#define IMGUI_DISABLE_SSE2
#define OPENSSL_NO_ASM

#pragma once
namespace Injector
{
    int Inject(char* exename);
    int Inject2(char* exename);
    int InjectWithName(const char* exename);
}
