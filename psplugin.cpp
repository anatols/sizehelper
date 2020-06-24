#include "stdafx.h"
#include "sizehelper.h"
#include "psplugin.h"

void SIZEHELPER_API ENTRYPOINT1 (
	short selector,
	void* pluginParamBlock,
	long* pluginData,
	short* result)
{
	switch (selector){
		case 0: { //=== about box
			//TODO
			//TCHAR fileName[MAX_PATH+1];
			//if (GetModuleFileName((HMODULE)g_hModule,fileName,MAX_PATH)!=0){
			//	DWORD dummy;
			//	DWORD versionSize=GetFileVersionInfoSize(fileName,&dummy);
			//	if (versionSize!=0){
			//		char *versionData=new char[versionSize];
			//		if (GetFileVersionInfo(fileName,0,versionSize,versionData)){
			//			TCHAR *productName;
			//			UINT productNameLen;
			//			TCHAR *version;
			//			UINT versionLen;
			//			VerQueryValue(versionData,_T("StringFileInfo\\040904b0\\ProductName"),(LPVOID *)&productName,&productNameLen);
			//			VerQueryValue(versionData,_T("StringFileInfo\\040904b0\\FileVersion"),(LPVOID *)&version,&versionLen);
			//			MessageBox(NULL, version, productName, MB_ICONINFORMATION);
			//		}
			//		delete [] versionData;
			//	}
			//}
		}; break;
		case 1: {
			//=== photoshop start
			sihInit();
		}; break;
		case 2: {
			//=== photoshop exit
			sihDeinit();
		}; break;
	}
	*result=0;
}