/*
 * Port Test Utility for EZ2AC
 * Tests direct IO port access and timing
 */

#include <windows.h>
#include <stdio.h>
#include <conio.h>

// Test if we can access IO ports directly
bool TestDirectPortAccess() {
    printf("Testing direct port access...\n");

    __try {
        __asm {
            mov dx, 0x102
            in al, dx
        }
        printf("[OK] Direct port access works! (Should not happen on NT/2000/XP)\n");
        return true;
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        printf("[Expected] Access denied - Need kernel driver\n");
        return false;
    }
}

// Measure exception handling overhead
void MeasureExceptionOverhead() {
    printf("\nMeasuring exception handling overhead...\n");

    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);

    const int iterations = 100;
    double totalTime = 0;

    for (int i = 0; i < iterations; i++) {
        QueryPerformanceCounter(&start);

        __try {
            __asm {
                mov dx, 0x102
                in al, dx
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            // Exception caught
        }

        QueryPerformanceCounter(&end);

        double elapsed = (double)(end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart;
        totalTime += elapsed;
    }

    printf("Average exception overhead: %.3f ms per access\n", totalTime / iterations);
    printf("This is the latency WITHOUT kernel driver!\n");
}

// Test keyboard scan codes
void TestKeyboardScanCodes() {
    printf("\n=== Keyboard Scan Code Test ===\n");
    printf("Press keys to see their scan codes. Press ESC to exit.\n\n");

    while (true) {
        if (_kbhit()) {
            int key = _getch();

            if (key == 27) {  // ESC
                break;
            }

            // Special keys return 0 or 224 first
            if (key == 0 || key == 224) {
                key = _getch();  // Get the actual scan code
                printf("Special key - Scan code: 0x%02X\n", key);
            } else {
                printf("Key: '%c' - ASCII: 0x%02X\n", key, key);
            }

            // Try to get Windows virtual key code
            SHORT vk = GetAsyncKeyState(VK_SPACE) & 0x8000;
            if (vk) {
                printf("  (Space bar detected via GetAsyncKeyState)\n");
            }
        }
        Sleep(10);
    }
}

// Performance comparison
void PerformanceComparison() {
    printf("\n=== Performance Comparison ===\n\n");

    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);

    // Test 1: GetAsyncKeyState
    printf("Method 1: GetAsyncKeyState\n");
    QueryPerformanceCounter(&start);
    for (int i = 0; i < 10000; i++) {
        GetAsyncKeyState(VK_SPACE);
    }
    QueryPerformanceCounter(&end);
    double time1 = (double)(end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart;
    printf("  10,000 calls: %.2f ms\n", time1);
    printf("  Average: %.4f ms per call\n\n", time1 / 10000);

    // Test 2: Direct port with exception (simulated)
    printf("Method 2: Port access with exception\n");
    QueryPerformanceCounter(&start);
    for (int i = 0; i < 100; i++) {  // Less iterations due to overhead
        __try {
            __asm {
                mov dx, 0x102
                in al, dx
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
        }
    }
    QueryPerformanceCounter(&end);
    double time2 = (double)(end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart;
    printf("  100 calls: %.2f ms\n", time2);
    printf("  Average: %.4f ms per call\n\n", time2 / 100);

    // Test 3: Kernel driver (simulated - just a function call)
    printf("Method 3: Kernel driver (simulated)\n");
    QueryPerformanceCounter(&start);
    for (int i = 0; i < 100000; i++) {
        // Simulate kernel driver - just a simple memory read
        volatile BYTE dummy = *(volatile BYTE*)&time1;
    }
    QueryPerformanceCounter(&end);
    double time3 = (double)(end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart;
    printf("  100,000 calls: %.2f ms\n", time3);
    printf("  Average: %.6f ms per call\n\n", time3 / 100000);

    // Summary
    printf("=== Summary ===\n");
    printf("GetAsyncKeyState:     %.4f ms\n", time1 / 10000);
    printf("Exception handler:    %.4f ms (%.0fx slower)\n", time2 / 100, (time2 / 100) / (time1 / 10000));
    printf("Kernel driver:        %.6f ms (%.0fx faster!)\n", time3 / 100000, (time1 / 10000) / (time3 / 100000));
}

// Main
int main() {
    printf("=====================================\n");
    printf("    EZ2AC Port Test Utility\n");
    printf("=====================================\n\n");

    // Check OS version
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);

    printf("OS Version: Windows %d.%d\n", osvi.dwMajorVersion, osvi.dwMinorVersion);

    if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1) {
        printf("Running on Windows XP - Perfect for kernel driver!\n");
    }

    while (true) {
        printf("\n=== Menu ===\n");
        printf("1. Test direct port access\n");
        printf("2. Measure exception overhead\n");
        printf("3. Test keyboard scan codes\n");
        printf("4. Performance comparison\n");
        printf("5. Exit\n");
        printf("\nChoice: ");

        char choice = _getch();
        printf("%c\n\n", choice);

        switch (choice) {
            case '1':
                TestDirectPortAccess();
                break;
            case '2':
                MeasureExceptionOverhead();
                break;
            case '3':
                TestKeyboardScanCodes();
                break;
            case '4':
                PerformanceComparison();
                break;
            case '5':
            case 27:  // ESC
                return 0;
            default:
                printf("Invalid choice\n");
        }
    }

    return 0;
}