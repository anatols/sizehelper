// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the WHEELBRUSHHOOK_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// WHEELBRUSHHOOK_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef SIZEHELPER_EXPORTS
#define SIZEHELPER_API __declspec(dllexport)
#else
#define SIZEHELPER_API __declspec(dllimport)
#endif

extern HANDLE g_hModule;

extern "C"{
void SIZEHELPER_API sihInit();
void SIZEHELPER_API sihDeinit();
}
