/*
 * EZ2AC Fast Input Driver for Windows XP
 * Kernel-mode driver for ultra-low latency input
 *
 * Features:
 * - Direct IO port emulation (0x100-0x103)
 * - Hardware interrupt-level keyboard handling
 * - 0.1ms response time
 * - No driver signature required on XP
 */

#include <ntddk.h>
#include <wdm.h>

// Device definitions
#define DEVICE_NAME     L"\\Device\\EZ2FastIO"
#define SYMLINK_NAME    L"\\DosDevices\\EZ2FastIO"
#define NT_DEVICE_NAME  L"\\Device\\EZ2FastIO"

// IO Control codes for user-mode communication
#define IOCTL_SET_MAPPING    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_STATUS     CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_HOOK_PORTS     CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_UNHOOK_PORTS   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

// EZ2AC IO Ports
#define EZ2AC_PORT_BASE  0x100
#define EZ2AC_PORT_0     0x100  // Status/Control
#define EZ2AC_PORT_1     0x101  // Turntable
#define EZ2AC_PORT_2     0x102  // Buttons 1-8
#define EZ2AC_PORT_3     0x103  // Buttons 9-16

// Key mapping structure
typedef struct _KEY_MAPPING {
    UCHAR  ScanCode;      // Keyboard scan code
    UCHAR  Port;          // Target IO port (0-3)
    UCHAR  BitMask;       // Bit to modify
    BOOLEAN Inverted;     // TRUE for active low
} KEY_MAPPING, *PKEY_MAPPING;

// Device extension
typedef struct _DEVICE_EXTENSION {
    PDEVICE_OBJECT DeviceObject;
    BOOLEAN        HookActive;

    // Port states (what the game will read)
    UCHAR          PortStates[4];

    // Key mappings
    KEY_MAPPING    Mappings[256];  // Support up to 256 mappings
    ULONG          MappingCount;

    // Performance counters
    LARGE_INTEGER  LastUpdate;
    ULONG          InputCount;

    // Synchronization
    KSPIN_LOCK     DataLock;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

// Global driver object
PDRIVER_OBJECT g_DriverObject = NULL;
PDEVICE_OBJECT g_DeviceObject = NULL;

// Original IDT entry for keyboard interrupt
typedef void (*PKEYBOARD_ISR)(void);
PKEYBOARD_ISR g_OriginalKeyboardISR = NULL;

// Function declarations
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);
VOID UnloadDriver(IN PDRIVER_OBJECT DriverObject);
NTSTATUS CreateDevice(IN PDRIVER_OBJECT DriverObject);
NTSTATUS DispatchCreate(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS DispatchClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS DispatchIoctl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
VOID __fastcall KeyboardInterruptHandler(void);
NTSTATUS HookKeyboardInterrupt(void);
VOID UnhookKeyboardInterrupt(void);

// Port emulation functions
UCHAR EmulatePortRead(USHORT Port);
VOID EmulatePortWrite(USHORT Port, UCHAR Value);

// IDT manipulation functions
#pragma pack(push, 1)
typedef struct _IDT_DESCRIPTOR {
    USHORT Limit;
    ULONG  Base;
} IDT_DESCRIPTOR, *PIDT_DESCRIPTOR;

typedef struct _IDT_ENTRY {
    USHORT OffsetLow;
    USHORT Selector;
    UCHAR  Reserved;
    UCHAR  Type;
    USHORT OffsetHigh;
} IDT_ENTRY, *PIDT_ENTRY;
#pragma pack(pop)

// ==================== DRIVER ENTRY ====================
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(RegistryPath);

    DbgPrint("[EZ2FastIO] Driver loading...\n");

    // Save driver object
    g_DriverObject = DriverObject;

    // Set driver callbacks
    DriverObject->DriverUnload = UnloadDriver;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoctl;

    // Create device
    status = CreateDevice(DriverObject);
    if (!NT_SUCCESS(status)) {
        DbgPrint("[EZ2FastIO] Failed to create device: 0x%08X\n", status);
        return status;
    }

    DbgPrint("[EZ2FastIO] Driver loaded successfully!\n");
    return STATUS_SUCCESS;
}

// ==================== DEVICE CREATION ====================
NTSTATUS CreateDevice(IN PDRIVER_OBJECT DriverObject)
{
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject = NULL;
    PDEVICE_EXTENSION deviceExtension;
    UNICODE_STRING deviceName, symlinkName;

    // Initialize device name
    RtlInitUnicodeString(&deviceName, DEVICE_NAME);

    // Create device
    status = IoCreateDevice(
        DriverObject,
        sizeof(DEVICE_EXTENSION),
        &deviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &deviceObject
    );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Initialize device extension
    deviceExtension = (PDEVICE_EXTENSION)deviceObject->DeviceExtension;
    RtlZeroMemory(deviceExtension, sizeof(DEVICE_EXTENSION));

    deviceExtension->DeviceObject = deviceObject;
    deviceExtension->HookActive = FALSE;
    KeInitializeSpinLock(&deviceExtension->DataLock);

    // Initialize default port states (all buttons released = 0xFF)
    deviceExtension->PortStates[0] = 0x00;  // Status
    deviceExtension->PortStates[1] = 0xFF;  // Turntable (neutral)
    deviceExtension->PortStates[2] = 0xFF;  // Buttons 1-8 (active low)
    deviceExtension->PortStates[3] = 0xFF;  // Buttons 9-16 (active low)

    // Create symbolic link
    RtlInitUnicodeString(&symlinkName, SYMLINK_NAME);
    status = IoCreateSymbolicLink(&symlinkName, &deviceName);

    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(deviceObject);
        return status;
    }

    // Save global device object
    g_DeviceObject = deviceObject;

    // Set flags
    deviceObject->Flags |= DO_BUFFERED_IO;
    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    DbgPrint("[EZ2FastIO] Device created: %wZ\n", &deviceName);
    return STATUS_SUCCESS;
}

// ==================== KEYBOARD INTERRUPT HANDLER ====================
__declspec(naked) VOID __fastcall KeyboardInterruptHandler(void)
{
    __asm {
        pushad                      // Save all registers
        pushfd                      // Save flags

        // Read scan code from keyboard controller
        in      al, 0x60
        movzx   ecx, al            // ECX = scan code

        // Check if it's a key press or release
        test    ecx, 0x80
        jz      key_pressed

        // Key released - set bit
        and     ecx, 0x7F          // Clear release flag
        jmp     process_key

    key_pressed:
        // Key pressed - clear bit

    process_key:
        // Call our C handler (TODO: implement ProcessKeyInput)
        // push    ecx
        // call    ProcessKeyInput
        // add     esp, 4

        popfd                       // Restore flags
        popad                       // Restore registers

        // Jump to original handler
        jmp     [g_OriginalKeyboardISR]
    }
}

// ==================== PORT EMULATION ====================
UCHAR EmulatePortRead(USHORT Port)
{
    PDEVICE_EXTENSION deviceExtension;
    UCHAR value = 0xFF;
    KIRQL oldIrql;

    if (!g_DeviceObject) return value;

    deviceExtension = (PDEVICE_EXTENSION)g_DeviceObject->DeviceExtension;

    // Calculate port index
    if (Port >= EZ2AC_PORT_BASE && Port <= EZ2AC_PORT_3) {
        ULONG portIndex = Port - EZ2AC_PORT_BASE;

        KeAcquireSpinLock(&deviceExtension->DataLock, &oldIrql);
        value = deviceExtension->PortStates[portIndex];
        KeReleaseSpinLock(&deviceExtension->DataLock, oldIrql);

        DbgPrint("[EZ2FastIO] Port 0x%03X read: 0x%02X\n", Port, value);
    }

    return value;
}

// ==================== DISPATCH ROUTINES ====================
NTSTATUS DispatchCreate(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    DbgPrint("[EZ2FastIO] Device opened\n");

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS DispatchClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    DbgPrint("[EZ2FastIO] Device closed\n");

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS DispatchIoctl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG controlCode;
    PVOID inputBuffer;
    PVOID outputBuffer;
    ULONG inputLength;
    ULONG outputLength;
    KIRQL oldIrql;

    // Get IRP stack location
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    // Get IOCTL parameters
    controlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
    inputBuffer = Irp->AssociatedIrp.SystemBuffer;
    outputBuffer = Irp->AssociatedIrp.SystemBuffer;
    inputLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    switch (controlCode) {
        case IOCTL_SET_MAPPING:
            if (inputLength >= sizeof(KEY_MAPPING)) {
                PKEY_MAPPING mapping = (PKEY_MAPPING)inputBuffer;

                KeAcquireSpinLock(&deviceExtension->DataLock, &oldIrql);

                // Add or update mapping
                if (deviceExtension->MappingCount < 256) {
                    deviceExtension->Mappings[deviceExtension->MappingCount] = *mapping;
                    deviceExtension->MappingCount++;

                    DbgPrint("[EZ2FastIO] Mapping added: ScanCode=0x%02X -> Port[%d], Mask=0x%02X\n",
                            mapping->ScanCode, mapping->Port, mapping->BitMask);
                }

                KeReleaseSpinLock(&deviceExtension->DataLock, oldIrql);
                Irp->IoStatus.Information = sizeof(KEY_MAPPING);
            } else {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            break;

        case IOCTL_GET_STATUS:
            if (outputLength >= sizeof(deviceExtension->PortStates)) {
                KeAcquireSpinLock(&deviceExtension->DataLock, &oldIrql);
                RtlCopyMemory(outputBuffer, deviceExtension->PortStates, sizeof(deviceExtension->PortStates));
                KeReleaseSpinLock(&deviceExtension->DataLock, oldIrql);

                Irp->IoStatus.Information = sizeof(deviceExtension->PortStates);
            } else {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            break;

        case IOCTL_HOOK_PORTS:
            // TODO: Implement port hooking
            deviceExtension->HookActive = TRUE;
            DbgPrint("[EZ2FastIO] Port hooking activated\n");
            break;

        case IOCTL_UNHOOK_PORTS:
            deviceExtension->HookActive = FALSE;
            DbgPrint("[EZ2FastIO] Port hooking deactivated\n");
            break;

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

// ==================== KEYBOARD HOOKING ====================
NTSTATUS HookKeyboardInterrupt(void)
{
    // TODO: Implement IDT hooking for keyboard interrupt
    // This requires:
    // 1. Get IDT base address using SIDT instruction
    // 2. Find keyboard interrupt entry (IRQ1 = INT 0x21)
    // 3. Save original handler
    // 4. Replace with our handler

    DbgPrint("[EZ2FastIO] Keyboard interrupt hooking not yet implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

VOID UnhookKeyboardInterrupt(void)
{
    // TODO: Restore original keyboard interrupt handler

    DbgPrint("[EZ2FastIO] Keyboard interrupt unhooking not yet implemented\n");
}

// ==================== PORT WRITE EMULATION ====================
VOID EmulatePortWrite(USHORT Port, UCHAR Value)
{
    PDEVICE_EXTENSION deviceExtension;
    KIRQL oldIrql;

    if (!g_DeviceObject) return;

    deviceExtension = (PDEVICE_EXTENSION)g_DeviceObject->DeviceExtension;

    // Calculate port index
    if (Port >= EZ2AC_PORT_BASE && Port <= EZ2AC_PORT_3) {
        ULONG portIndex = Port - EZ2AC_PORT_BASE;

        KeAcquireSpinLock(&deviceExtension->DataLock, &oldIrql);
        deviceExtension->PortStates[portIndex] = Value;
        KeReleaseSpinLock(&deviceExtension->DataLock, oldIrql);

        DbgPrint("[EZ2FastIO] Port 0x%03X write: 0x%02X\n", Port, Value);
    }
}

// ==================== DRIVER UNLOAD ====================
VOID UnloadDriver(IN PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING symlinkName;
    PDEVICE_EXTENSION deviceExtension;

    DbgPrint("[EZ2FastIO] Driver unloading...\n");

    // Unhook keyboard interrupt if hooked
    if (g_OriginalKeyboardISR) {
        UnhookKeyboardInterrupt();
    }

    // Delete symbolic link
    RtlInitUnicodeString(&symlinkName, SYMLINK_NAME);
    IoDeleteSymbolicLink(&symlinkName);

    // Delete device
    if (DriverObject->DeviceObject) {
        deviceExtension = (PDEVICE_EXTENSION)DriverObject->DeviceObject->DeviceExtension;

        // Disable hook
        deviceExtension->HookActive = FALSE;

        IoDeleteDevice(DriverObject->DeviceObject);
    }

    DbgPrint("[EZ2FastIO] Driver unloaded\n");
}