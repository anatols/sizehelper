#include "stdafx.h"

#include <math.h>
#include <vector>

//#include <algorithm>
//#include <locale>
//#include <functional>

#include <shlwapi.h>
#include <atlstr.h>

#include "sizehelper.h"

bool g_attachedToWindow=false;
HHOOK g_hWndHook;
DWORD g_currentProcessID;
HWND g_hImageSizeWindow;
HWND g_hImageSizePSViewC;
HWND g_hwndWidth;
HWND g_hwndWidthCombo;
HWND g_hwndHeight;
HWND g_hwndHeightCombo;
HWND g_hwndPresetButton;
HWND g_hwndConstrainRadio;
TCHAR g_ImageSizeText[50]=_T("");
HBITMAP g_bitmapButton;
HINSTANCE g_hInstance;

HMENU g_popupMenu=NULL;

int g_origWidth=0;
int g_origHeight=0;

//=== counters for IsImageSizeWindow
int g_staticCount;
int g_editCount;
int g_comboCount;
int g_buttonCount;

enum EPresetType{
	PT_PIXELS,
	PT_LONG_EDGE,
	PT_WIDTH,
	PT_HEIGHT,
	PT_RESET,
	PT_SEPARATOR,
};

struct SSizePreset{
	int		type;
	int		value;
};

typedef std::vector<SSizePreset> TSizePresets;
TSizePresets g_sizePresets;

BOOL WINAPI DllMain(
  HANDLE hinstDLL, 
  DWORD dwReason, 
  LPVOID lpvReserved
){
	switch (dwReason){
		case DLL_PROCESS_ATTACH:{
			g_hInstance = (HINSTANCE) hinstDLL;
		}; break;
	}
	return TRUE;
}

void SetNewSize(int sizePreset){
	if (g_origWidth==0 || g_origHeight==0){
		return;
	}
	int newWidth = g_origWidth;
	int newHeight = g_origHeight;
	
	int type = g_sizePresets[sizePreset].type;
	if (type == PT_LONG_EDGE){
		type = g_origWidth > g_origHeight ? PT_WIDTH : PT_HEIGHT;
	}
	

	bool setWidth=true;
	bool setHeight=true;
	bool constrainProp = false;
	if (g_hwndConstrainRadio!=NULL){
		int state=(int)SendMessage(g_hwndConstrainRadio, (UINT) BM_GETSTATE, 0, 0);
		constrainProp = (state & BST_CHECKED);
	}
	
	switch (type){
		case PT_PIXELS:{
			float factor = sqrtf(((float)g_sizePresets[sizePreset].value) / (g_origWidth*g_origHeight));
			newWidth = (int)ceilf(g_origWidth * factor);
			setWidth = true;
			
			//=== correct result to adapt to photoshops flooring
			if (constrainProp){
				float aspect = (float)g_origWidth / (float)g_origHeight;
				newHeight = (int)floorf(newWidth / aspect);
				while (newWidth * newHeight < g_sizePresets[sizePreset].value){
					newWidth++;
					newHeight = (int)floorf(newWidth / aspect);
				}
				
				setHeight = false;
			}
			else{
				newHeight = (int)ceilf(g_origHeight * factor);
				setHeight = true;
			}
		}; break;
		case PT_WIDTH:{
			float factor = ((float)g_sizePresets[sizePreset].value) / g_origWidth;
			newWidth = g_sizePresets[sizePreset].value;
			newHeight = (int)ceilf(g_origHeight * factor);
			setWidth = true;
			setHeight = !constrainProp;
		}; break;
		case PT_HEIGHT:{
			float factor = ((float)g_sizePresets[sizePreset].value) / g_origHeight;
			newHeight = g_sizePresets[sizePreset].value;
			newWidth = (int)ceilf(g_origWidth * factor);
			setHeight = true;
			setWidth = !constrainProp;
		}; break;
	}
	
	TCHAR buf[50];
	
	//=== set new width
	if (setWidth){
		_itow(newWidth, buf, 10);
		SetWindowText(g_hwndWidth, buf);
		SendMessage(g_hwndWidth, EM_SETMODIFY, TRUE, 0);
		SendMessage(g_hwndWidth, WM_CHAR, ' ', 0);
		SendMessage(g_hwndWidth, WM_CHAR, 8, 0);
	}
	
	//=== set new height
	if (setHeight){
		_itow(newHeight, buf, 10);
		SetWindowText(g_hwndHeight, buf);
		SendMessage(g_hwndHeight, EM_SETMODIFY, TRUE, 0);
		SendMessage(g_hwndHeight, WM_CHAR, ' ', 0);
		SendMessage(g_hwndHeight, WM_CHAR, 8, 0);
	}
}

LRESULT CALLBACK ButtonWindowProc(HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam
){
	switch (uMsg){
		case WM_PAINT:{
			PAINTSTRUCT ps;
			BeginPaint(hwnd, &ps);
			HDC bitmapDC = CreateCompatibleDC(ps.hdc);
			HGDIOBJ hbmOld = SelectObject (bitmapDC, g_bitmapButton);
			BitBlt(ps.hdc, 0, 0, 20, 15, bitmapDC, 0, 0, SRCCOPY);
			SelectObject(bitmapDC, hbmOld);
			DeleteDC(bitmapDC);

			EndPaint(hwnd, &ps);
		}; break;
		case WM_LBUTTONUP:{
			POINT pt;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);
			ClientToScreen(hwnd, &pt);
			int val = TrackPopupMenu(g_popupMenu, 0 , pt.x, pt.y, 0, hwnd, NULL);
			
		}; break;
		case WM_COMMAND:{
			//=== menu item selected
			if (HIWORD(wParam)==0){
				SetNewSize(LOWORD(wParam));
			}
		}; break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

BOOL CALLBACK IsImageSizeWindowEnumChildProc(HWND hwnd, LPARAM lParam ){
	TCHAR className[MAX_CLASS_NAME];
	if (GetClassName(hwnd,className,MAX_CLASS_NAME)==0) return FALSE;
	
	if (!_tcscmp(className, _T("Button"))) g_buttonCount++;
	else
	if (!_tcscmp(className, _T("Edit"))) g_editCount++;
	else
	if (!_tcscmp(className, _T("ComboBox"))) g_comboCount++;
	else
	if (!_tcscmp(className, _T("Static"))) g_staticCount++;
	
	return TRUE;
}

bool IsImageSizeWindow(HWND hwnd){
	TCHAR className[MAX_CLASS_NAME];
	if (GetClassName(hwnd,className,MAX_CLASS_NAME)==0) return false;

	//=== is it PSFloatC
	if (_tcscmp(className,_T("PSFloatC"))!=0) return false;
	
	//=== one step down should be PSViewC
	HWND hwnd_psviewc = FindWindowEx(hwnd, NULL, _T("PSViewC"), NULL);
	if (hwnd_psviewc==NULL) return false;
	
	//TODO do not check for name -- will not work in localized PS
	//TCHAR name[512];
	//GetWindowText(hwnd, name, 512);
	//if (_tcscmp(name, _T("Image Size"))!=0) return false;
	
	//=== count elements to make sure this is the window that looks like what we need
	g_staticCount=0; //11
	g_editCount=0; //5
	g_comboCount=0; //6
	g_buttonCount=0; //6
	EnumChildWindows(hwnd_psviewc, &IsImageSizeWindowEnumChildProc, 0);
	if (g_staticCount!=11 || g_editCount!=5 || g_comboCount!=6 || g_buttonCount!=6) return false;

	return true;
}

void UpdatePixelCount(){
	if (!g_attachedToWindow) return;

	//=== get width and height
	TCHAR buf[512];
	GetWindowText(g_hwndWidth, buf, 512);
	int width = _wtoi(buf);
	
	GetWindowText(g_hwndHeight, buf, 512);
	int height = _wtoi(buf);
	
	static int lastWidth = 0;
	static int lastHeight = 0;
	
		
	//=== check if pixels are used
	int widthSel = (int)SendMessage(g_hwndWidthCombo, CB_GETCURSEL, 0, 0);
	int heightSel = (int)SendMessage(g_hwndHeightCombo, CB_GETCURSEL, 0, 0);
	if (g_origWidth!=0 && g_origHeight!=0 && (widthSel!=1 || heightSel!=1)) {
		lastWidth=lastHeight = 0;
		SetWindowText(g_hImageSizeWindow, g_ImageSizeText);
		ShowWindow(g_hwndPresetButton, SW_HIDE);
		return;
	}
	else{
		//TODO calc size as % of original image size
	}

	if (g_origWidth==0 || g_origHeight==0){
		g_origWidth = width;
		g_origHeight = height;
		lastWidth=lastHeight = 0;
	}

	//=== check if values changed
	if (lastWidth == width && lastHeight == height){
		return;
	}

	ShowWindow(g_hwndPresetButton, SW_SHOW);

	lastWidth = width;
	lastHeight = height;
	
	//=== calculate size
	int imageSize = width * height;
	
	wcscpy(buf, g_ImageSizeText);
	wcscat(buf, _T(": "));
	
	TCHAR sizeBuf[50];
	sizeBuf[49]=0;
	int sizePtr=49;
	int i=0;
	while (imageSize > 0){
		int c = imageSize % 10;
		imageSize /= 10;
		sizeBuf[--sizePtr] = c + _T('0');
		i++;
		if ((i % 3) == 0){
			sizeBuf[--sizePtr] = _T(' ');
		}
	}
	
	wcscat(buf, sizeBuf + sizePtr);
	wcscat(buf, _T(" px"));
	
	SetWindowText(g_hImageSizeWindow, buf);
}

void AttachToWindow(HWND hwnd){
	HWND child;

	//=== one step down should be PSViewC
	HWND hwnd_psviewc = FindWindowEx(hwnd, NULL, _T("PSViewC"), NULL);
	if (hwnd_psviewc==NULL) return;

	//=== finding editboxes
	g_hwndWidth=NULL;
	g_hwndHeight=NULL;
	
	child=NULL;
	while ((child=FindWindowEx(hwnd_psviewc, child, _T("Edit"), NULL))!=NULL){
		if (g_hwndWidth==NULL) {
			g_hwndWidth = child;
			continue;
		}
		if (g_hwndHeight==NULL){
			g_hwndHeight = child;
			break;
		}
	}

	//=== finding comboboxes
	g_hwndWidthCombo=NULL;
	g_hwndHeightCombo=NULL;

	child=NULL;
	while ((child=FindWindowEx(hwnd_psviewc, child, _T("ComboBox"), NULL))!=NULL){
		if (g_hwndWidthCombo==NULL) {
			g_hwndWidthCombo = child;
			continue;
		}
		if (g_hwndHeightCombo==NULL){
			g_hwndHeightCombo = child;
			break;
		}
	}

	//=== finding constrain combobox
	g_hwndConstrainRadio=NULL;
	child=NULL;
	child=FindWindowEx(hwnd_psviewc, child, _T("Button"), NULL);
	if (child!=NULL){
		g_hwndConstrainRadio=FindWindowEx(hwnd_psviewc, child, _T("Button"), NULL);
	}

	GetWindowText(hwnd, g_ImageSizeText, 50);

	g_hwndPresetButton=CreateWindowEx(WS_EX_NOPARENTNOTIFY, _T("Static"),_T(">"), WS_CHILD | WS_VISIBLE, 0, 0, 20, 15, hwnd_psviewc, NULL, NULL, NULL);
	SetWindowLongPtr(g_hwndPresetButton, GWLP_WNDPROC, (LONG)(LONG_PTR)&ButtonWindowProc);

	g_hImageSizePSViewC = hwnd_psviewc;
	g_hImageSizeWindow = hwnd;
	g_attachedToWindow = true;
}

LRESULT CALLBACK WndHook(int nCode, WPARAM wParam, LPARAM lParam) 
{ 
    if (nCode < 0) {
		//=== пропускаем хук
        return CallNextHookEx(g_hWndHook, nCode, wParam, lParam); 
    }
    
    CWPSTRUCT *info = (CWPSTRUCT *)lParam;
    
    if (!g_attachedToWindow){
		//=== we're not attached, check if we need to attach to this window
		if (info->message == WM_ACTIVATE){
			if (IsImageSizeWindow(info->hwnd)){
				//=== attach to this window
				AttachToWindow(info->hwnd);
				UpdatePixelCount();
			}
		}
	}
	else{
		if (info->message == WM_DESTROY && info->hwnd==g_hImageSizeWindow){
			g_attachedToWindow = false;
			g_hImageSizeWindow = NULL;
			g_origWidth=g_origHeight=0;
		}
		else
		if (info->message == EM_GETMODIFY && (info->hwnd == g_hwndWidth || info->hwnd == g_hwndHeight)){
			UpdatePixelCount();
		}
		else
		if (info->message == CB_GETCOUNT && (info->hwnd == g_hwndWidthCombo || info->hwnd == g_hwndHeightCombo)){
			UpdatePixelCount();
		}
		else
		if (info->message == WM_SYSCOMMAND && info->hwnd==g_hImageSizeWindow && info->wParam == SC_KEYMENU){
			POINT pt;
			pt.x = 0;
			pt.y = 0;
			ClientToScreen(info->hwnd, &pt);
			TrackPopupMenu(g_popupMenu, 0 , pt.x, pt.y, 0, g_hwndPresetButton, NULL);
		}
	}

    return CallNextHookEx(g_hWndHook, nCode, wParam, lParam);
}

void SIZEHELPER_API sihInit(){
	g_currentProcessID=GetCurrentProcessId();

	//=== bitmap button
	g_bitmapButton = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_BUTTON1));

	//=== loading size presets
	TCHAR dllPath[MAX_PATH];
	GetModuleFileName(g_hInstance, dllPath, MAX_PATH);
	PathRemoveFileSpec(dllPath);
	
	wcscat(dllPath,_T("\\sizehelper.cfg"));
	
	FILE *fcfg = _wfopen(dllPath, _T("rt"));
	if (fcfg!=NULL){
		while (!feof(fcfg)){
			TCHAR line[1024];
			if (fgetws(line, 1024, fcfg) == NULL) break;
			
			SSizePreset preset;
			
			CAtlString str(line);
			str = str.Trim();
			
			if (str.IsEmpty()){
				continue;
			}
			
			CAtlString first = str.Left(1);
			CAtlString digits = str.Mid(str.FindOneOf(_T("0123456789"))).SpanIncluding(_T("0123456789"));
			
			if (first == _T("#")){
				continue;
			}
			if (first == _T("l")){
				preset.type = PT_LONG_EDGE;
				preset.value = _ttoi(digits);
				if (preset.value == 0) continue;
			}
			else
			if (first == _T("w")){
				preset.type = PT_WIDTH;
				preset.value = _ttoi(digits);
				if (preset.value == 0) continue;
			}
			else
			if (first == _T("h")){
				preset.type = PT_HEIGHT;
				preset.value = _ttoi(digits);
				if (preset.value == 0) continue;
			}
			else
			if (first == _T("-")){
				preset.type = PT_SEPARATOR;
			}
			else
			if (first == _T("r")){
				preset.type = PT_RESET;
			}
			else{
				preset.type = PT_PIXELS;
				preset.value = _ttoi(digits);
				if (preset.value == 0) continue;
			}
			
			g_sizePresets.push_back(preset);
		}
	}

	//=== creating menu
	g_popupMenu = CreatePopupMenu();
	for (int i=0; i< (int)g_sizePresets.size(); i++){
		switch (g_sizePresets[i].type){
			case PT_SEPARATOR:{
				//=== separate different types
				InsertMenu(g_popupMenu, -1, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
			}; break;
			case PT_PIXELS:{
				TCHAR buf[50];
				int val = g_sizePresets[i].value;
				if ((val % 1000000) == 0){
					_itot(val / 1000000, buf, 10);
					_tcscat(buf, _T(" Mpx"));
				}
				else{
					_itot(val, buf, 10);
					_tcscat(buf, _T(" px"));
				}
				InsertMenu(g_popupMenu, -1, MF_ENABLED | MF_STRING | MF_BYPOSITION, i, buf);
			}; break;
			case PT_LONG_EDGE:{
				TCHAR buf[50];
				_tcscpy(buf, _T("Long edge: "));
				_itot(g_sizePresets[i].value, buf + _tcslen(buf), 10);
				InsertMenu(g_popupMenu, -1, MF_ENABLED | MF_STRING | MF_BYPOSITION, i, buf);
			}; break;
			case PT_WIDTH:{
				TCHAR buf[50];
				_tcscpy(buf, _T("Width: "));
				_itot(g_sizePresets[i].value, buf + _tcslen(buf), 10);
				InsertMenu(g_popupMenu, -1, MF_ENABLED | MF_STRING | MF_BYPOSITION, i, buf);
			}; break;
			case PT_HEIGHT:{
				TCHAR buf[50];
				_tcscpy(buf, _T("Height: "));
				_itot(g_sizePresets[i].value, buf + _tcslen(buf), 10);
				InsertMenu(g_popupMenu, -1, MF_ENABLED | MF_STRING | MF_BYPOSITION, i, buf);
			}; break;
			case PT_RESET:{
				InsertMenu(g_popupMenu, -1, MF_ENABLED | MF_STRING | MF_BYPOSITION, i, _T("Reset"));
			}; break;
		}
	}
	
	//=== ставим хук на события
	g_hWndHook=SetWindowsHookEx(WH_CALLWNDPROC, &WndHook, NULL, GetCurrentThreadId());
	
}

void SIZEHELPER_API sihDeinit(){
	//=== снимаем хуки
	UnhookWindowsHookEx(g_hWndHook);
	
	//=== destroying menu
	DestroyMenu(g_popupMenu);
	
	//=== destroying bitmap
	DeleteObject(g_bitmapButton);
}
