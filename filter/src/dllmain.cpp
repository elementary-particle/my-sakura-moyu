
#include <cstdio>
#include <cstring>

#include <windows.h>

#include <detours.h>

#include "../../filelist.h"
#include "common.h"
#include "overlay/i_d3d9.h"
#include "strsub.h"
#include "subtitle.h"

#define NAME_SIZE 0x200

template <typename TYPE>
inline void FromPtr(TYPE &pFunc, PVOID pRaw) {
  *(PVOID *)(&pFunc) = pRaw;
}

template <typename TYPE>
inline PVOID ToPtr(TYPE pFunc) {
  TYPE pRaw = pFunc;
  return *(PVOID *)(&pRaw);
}

int (*StringMap::pInflateInit)(z_streamp, const char *, int);
int (*StringMap::pInflate)(z_streamp, int);
int (*StringMap::pInflateEnd)(z_streamp);

/** Executable Structures **/
class FileSys {
 public:
  typedef void (FileSys::*PFUNC_Open)(PCSTR pPath);
  static PFUNC_Open pOpen;
  void Open(PCSTR pPath) {
    char pNewPath[NAME_SIZE];
    UINT L, R;
    L = 0;
    R = defaultEntryList.size();
    while (L < R) {
      UINT M;
      int C;
      M = (L + R) >> 1;
      C = strcmp(pPath, defaultEntryList[M].originalName);
      if (C < 0) {
        R = M;
      } else if (C > 0) {
        L = M + 1;
      } else {
        if (strcpy_s(pNewPath, "patch/") ||
            strcat_s(pNewPath, defaultEntryList[M].patchName)) {
          break;
        }
        pPath = pNewPath;
        break;
      }
    }
    (this->*pOpen)(pPath);
  }
};
FileSys::PFUNC_Open FileSys::pOpen = NULL;

struct VMARG;
struct VMSTR;
struct VMENV;

class SakuraApp {
 private:
  static DWORD dwAudioSubChannel;
  static PCSTR AudioSub[4];

 public:
  static DWORD dwStringOffset[0x80];

  typedef void (SakuraApp::*FUNC_VOID)();
  static FUNC_VOID pRunStep;
  void RunStep() { (this->*pRunStep)(); }
  typedef void (SakuraApp::*FUNC_NATIVE)(VMARG *);
  static FUNC_NATIVE pSysMovie;
  void SysMovie(VMARG *);
  static FUNC_NATIVE pSysMovieStop;
  void SysMovieStop(VMARG *);
  static FUNC_NATIVE pAudioLoad;
  void AudioLoad(VMARG *);
  static FUNC_NATIVE pAudioPlay;
  void AudioPlay(VMARG *);
  static FUNC_NATIVE pAudioStop;
  void AudioStop(VMARG *);
  static FUNC_NATIVE pTextTest;
  void TextTest(VMARG *);
  inline VMENV *GetEnv();
  inline PVOID GetScript() { return *((PVOID *)this + 1718251); }
  inline VMSTR *GetStringByIndex(int iStrIndex) {
    return (VMSTR *)((PDWORD)this + 1741435 + iStrIndex);
  }
  PCSTR GetStringArg(VMARG *Arg);
};
DWORD SakuraApp::dwAudioSubChannel = 4;
PCSTR SakuraApp::AudioSub[4];
DWORD SakuraApp::dwStringOffset[0x80];
SakuraApp::FUNC_VOID SakuraApp::pRunStep = NULL;
SakuraApp::FUNC_NATIVE SakuraApp::pSysMovie = NULL;
SakuraApp::FUNC_NATIVE SakuraApp::pSysMovieStop = NULL;
SakuraApp::FUNC_NATIVE SakuraApp::pAudioLoad = NULL;
SakuraApp::FUNC_NATIVE SakuraApp::pAudioPlay = NULL;
SakuraApp::FUNC_NATIVE SakuraApp::pAudioStop = NULL;
SakuraApp::FUNC_NATIVE SakuraApp::pTextTest = NULL;

struct HEAPBLOCK {
  static const size_t sVmSize0 = 35, sVmSize1 = sVmSize0 + 5;
  static const DWORD dwVmStart = 0x74daa;
  constexpr static const BYTE bVmBytes[] =
      "\x02\x50\x7e\x03\x00"
      "\x0b\xfa\x00\x0e\x08"
      "warning\x00\x0c\x05\x08"
      "\x02\xa5\x73\x03\x00"  // sub_000373a5(250, "patch/warning", 5, 0b)
      "\x0b\xb8\x0b\x03\x89\x00"
      "\x03\x85\x00"
      "\x06\x00\x00\x00\x00";
  typedef void (HEAPBLOCK::*PFUNC_ReadFile)(PCSTR pPath);
  static PFUNC_ReadFile pReadFile;
  void ReadFile(PCSTR pPath) {
    (this->*pReadFile)(pPath);
    if (!strcmp(pPath, "Sakura.hcb")) {
      DWORD dwScriptSize;
      dwScriptSize = dwSize;
      (this->*pSetSize)(dwSize + sVmSize1);
      memcpy((PBYTE)pBuf + dwScriptSize, bVmBytes, sVmSize1);
      *((PBYTE)pBuf + dwVmStart) = 0x06;
      *(PDWORD)((PBYTE)pBuf + dwVmStart + 1) = dwScriptSize;
      *(PDWORD)((PBYTE)pBuf + dwScriptSize + sVmSize0 + 1) = dwVmStart + 5;
    }
  }
  typedef void (HEAPBLOCK::*PFUNC_SetSize)(DWORD dwSize_);
  static PFUNC_SetSize pSetSize;

  void *pBuf;
  DWORD dwSize;
};
HEAPBLOCK::PFUNC_ReadFile HEAPBLOCK::pReadFile = NULL;
HEAPBLOCK::PFUNC_SetSize HEAPBLOCK::pSetSize = NULL;

struct VMARG {
  BYTE bType, bPad0[3];
  DWORD dwData;
};

struct VMSTR {
 public:
  typedef void (VMSTR::*PFUNC_SetString)(PCSTR pString);
  static PFUNC_SetString pSetString;
};
VMSTR::PFUNC_SetString VMSTR::pSetString = NULL;

WCHAR hTextBufW[0x400];
CHAR hTextBuf[0x800];
struct VMENV {
  SakuraApp *pApp;
  PVOID pScript;
  VMARG pStack[0x100];
  DWORD dwFlags;
  DWORD dwWaitTimeLeft;
  BYTE bYield, bPad0[3];
  VMARG argReturnValue;
  DWORD dwPtrCode;
  DWORD dwPtrStack;
  DWORD dwPtrFrame;
  WORD wRefCount0[0x80];
  WORD wRefCount1[0x100];

 public:
  static StringMap *pSubMap;
  typedef int (VMENV::*PFUNC_GetIndex)();
  static PFUNC_GetIndex pGetIndex;
  typedef void (VMENV::*PFUNC_LoadText)();
  static PFUNC_LoadText pLoadText;
  void LoadText() {
    (this->*pLoadText)();
    VMARG &ArgTop = this->pStack[this->dwPtrStack - 1];
    if (pSubMap) {
      StringMap::ENTRY *pEntry;
      pEntry = pSubMap->GetEntry(ArgTop.dwData);
      if (pEntry) {
        PCSTR pOrgText;
        int iStrIndex;
        VMSTR *pVmStr;
        pOrgText = (PCSTR)this->pScript + ArgTop.dwData;
        iStrIndex = (this->*pGetIndex)();
        SakuraApp::dwStringOffset[iStrIndex] = pEntry->dwOffset;
        pVmStr = this->pApp->GetStringByIndex(iStrIndex);
        if (pEntry->pText) {
          (pVmStr->*(VMSTR::pSetString))(pEntry->pText);
#ifdef DEBUG_LOG
          if (168 <= pEntry->dwIndex && pEntry->dwIndex < 53909) {
            MultiByteToWideChar(932, 0, pOrgText, -1, hTextBufW, 0x400);
            WideCharToMultiByte(65001, 0, hTextBufW, -1, hTextBuf, 0x800, NULL,
                                NULL);
            DebugLog(0, "%6d %s\n", pEntry->dwIndex, hTextBuf);
            MultiByteToWideChar(936, 0, pEntry->pText, -1, hTextBufW, 0x400);
            WideCharToMultiByte(65001, 0, hTextBufW, -1, hTextBuf, 0x800, NULL,
                                NULL);
            DebugLog(0, "       %s\n\n", hTextBuf);
          }
#endif
        } else {
          int Size;
          Size = MultiByteToWideChar(932, 0, pOrgText, -1, hTextBufW, 0x400);
          if (Size) {
            Size = WideCharToMultiByte(936, 0, hTextBufW, -1, hTextBuf, 0x800,
                                       NULL, NULL);
            if (Size) {
              (pVmStr->*(VMSTR::pSetString))(hTextBuf);
            } else {
              (pVmStr->*(VMSTR::pSetString))("<无法转换文本>");
            }
          } else {
            (pVmStr->*(VMSTR::pSetString))("<无法转换文本>");
          }
        }
        ArgTop.bType = 5;
        ArgTop.dwData = iStrIndex;
      }
    }
  }
};
StringMap *VMENV::pSubMap = NULL;
VMENV::PFUNC_GetIndex VMENV::pGetIndex = NULL;
VMENV::PFUNC_LoadText VMENV::pLoadText = NULL;

VMENV *SakuraApp::GetEnv() {
  return (VMENV *)((PDWORD)this + 1718256) + *((PDWORD)this + 1743284);
}

PCSTR SakuraApp::GetStringArg(VMARG *Arg) {
  if (Arg->bType == 4) {
    return (PCSTR)GetEnv()->pScript + Arg->dwData;
  } else if (Arg->bType == 5) {
    return *(PCSTR *)GetStringByIndex(Arg->dwData);
  } else {
    return NULL;
  }
}

/** Native VM Functions **/
void SakuraApp::SysMovie(VMARG *Arg) {
  PCSTR pMoviePath;
  pMoviePath = GetStringArg(Arg);
  // DebugLog(0, "Movie Load: %s\n", pMoviePath);
  (this->*pSysMovie)(Arg);
  if (pMoviePath) {
    PCSTR pAssSubPath;
    if (!strcmp(pMoviePath, "movie/01.wmv")) {
      pAssSubPath = PATH_SUB VD1_NAME;
      GameApp.AttachSub(new Subtitle((char *)pAssSubPath));
    } else if (!strcmp(pMoviePath, "movie/02.wmv")) {
      pAssSubPath = PATH_SUB VD2_NAME;
      GameApp.AttachSub(new Subtitle((char *)pAssSubPath));
    }
  }
}
void SakuraApp::SysMovieStop(VMARG *Arg) {
  // DebugLog(0, "Movie Stop\n");
  (this->*pSysMovieStop)(Arg);
  GameApp.DetachSub();
}
void SakuraApp::AudioLoad(VMARG *Arg) {
  PCSTR pAudioPath;
  (this->*pAudioLoad)(Arg);
  if (Arg->bType == 2 && 0 <= Arg->dwData && Arg->dwData < 4) {
    pAudioPath = GetStringArg(Arg + 1);
    // DebugLog(0, "Audio Load: %d %s\n", Arg->dwData, pAudioPath);
    if (pAudioPath) {
      if (!strcmp(pAudioPath, "BGM/073")) {  // ED1
        AudioSub[Arg->dwData] = PATH_SUB ED1_NAME;
      } else if (!strcmp(pAudioPath, "BGM/074")) {  // ED2
        AudioSub[Arg->dwData] = PATH_SUB ED2_NAME;
      } else {
        AudioSub[Arg->dwData] = NULL;
      }
    }
  }
}
void SakuraApp::AudioPlay(VMARG *Arg) {
  (this->*pAudioPlay)(Arg);
  if (0 <= Arg->dwData && Arg->dwData < 4) {
    // DebugLog(0, "Audio Play: %d\n", Arg->dwData);
    if (AudioSub[Arg->dwData]) {
      dwAudioSubChannel = Arg->dwData;
      GameApp.AttachSub(new Subtitle((char *)AudioSub[Arg->dwData]));
    }
  }
}
void SakuraApp::AudioStop(VMARG *Arg) {
  (this->*pAudioStop)(Arg);
  if (0 <= Arg->dwData && Arg->dwData < 4) {
    // DebugLog(0, "Audio Stop: %d\n", Arg->dwData);
    if (AudioSub[Arg->dwData] && Arg->dwData == dwAudioSubChannel) {
      GameApp.DetachSub();
      dwAudioSubChannel = 4;
    }
  }
}
void SakuraApp::TextTest(VMARG *Arg) {
  // TODO: Hook the free function as well.
  // A bit more checking would make this more robust.
  // Assuming that the VM would not test unrelated entries.

  if (Arg->bType == 5 && Arg->dwData < 0x80) {
    VMARG OriArg;
    OriArg.bType = 4;
    OriArg.dwData = dwStringOffset[Arg->dwData];
    (this->*pTextTest)(&OriArg);
  } else {
    (this->*pTextTest)(Arg);
  }
}

/** Global Environment **/
HMODULE hSakuraExe;
BOOL doTextPatch, doImagePatch, doSubPatch;
CHAR lpDllPath[NAME_SIZE];
HMODULE hDirect3D9Library;

/** Global Functions **/
typedef int(WINAPI *PFUNC_MessageBoxA)(HWND hWnd, LPCSTR lpText,
                                       LPCSTR lpCaption, UINT uType);
typedef HWND(WINAPI *PFUNC_CreateWindowExA)(DWORD dwExStyle, LPCSTR lpClassName,
                                            LPCSTR lpWindowName, DWORD dwStyle,
                                            int X, int Y, int nWidth,
                                            int nHeight, HWND hWndParent,
                                            HMENU hMenu, HINSTANCE hInstance,
                                            LPVOID lpParam);
typedef INT(WINAPI *PFUNC_StrcmpIA)(LPCSTR lpString1, LPCSTR lpString2);
typedef INT(CDECL *PFUNC_IsLeadByte)(UINT Char);

PFUNC_MessageBoxA pMessageBoxA;
PFUNC_CreateWindowExA pCreateWindowExA;
PFUNC_StrcmpIA pStrcmpIA;
PFUNC_IsLeadByte pIsLeadByte;

DWORD PatchDoubleWord(PVOID pAddr, DWORD dwValue) {
  DWORD dwOldProtect, dwOldValue;

  VirtualProtect(pAddr, 4, PAGE_EXECUTE_READWRITE, &dwOldProtect);
  dwOldValue = *(PDWORD)pAddr;
  *(PDWORD)pAddr = dwValue;
  VirtualProtect(pAddr, 4, dwOldProtect, &dwOldProtect);
  return dwOldValue;
}

void InitGlobal() {
  HMODULE hUser32, hKernel32;
  hUser32 = GetModuleHandleW(L"USER32");
  hKernel32 = GetModuleHandleW(L"KERNEL32");
  pMessageBoxA = (PFUNC_MessageBoxA)GetProcAddress(hUser32, "MessageBoxA");
  pCreateWindowExA =
      (PFUNC_CreateWindowExA)GetProcAddress(hUser32, "CreateWindowExA");
  pStrcmpIA = (PFUNC_StrcmpIA)GetProcAddress(hKernel32, "lstrcmpiA");
  pIsLeadByte = (PFUNC_IsLeadByte)0x44844f;
  FromPtr(HEAPBLOCK::pReadFile, (PVOID)0x4344c0);
  FromPtr(HEAPBLOCK::pSetSize, (PVOID)0x434060);

  FromPtr(StringMap::pInflateInit, (PVOID)0x401cd0);
  FromPtr(StringMap::pInflate, (PVOID)0x401de0);
  FromPtr(StringMap::pInflateEnd, (PVOID)0x4033c0);
  FromPtr(FileSys::pOpen, (PVOID)0x4422d0);
  FromPtr(VMSTR::pSetString, (PVOID)0x433c60);
  FromPtr(VMENV::pGetIndex, (PVOID)0x4445e0);
  FromPtr(VMENV::pLoadText, (PVOID)0x445b50);
  FromPtr(SakuraApp::pRunStep, (PVOID)0x43f570);
  FromPtr(SakuraApp::pSysMovie, (PVOID)0x43b120);
  FromPtr(SakuraApp::pSysMovieStop, (PVOID)0x43b300);
  FromPtr(SakuraApp::pAudioLoad, (PVOID)0x439c80);
  FromPtr(SakuraApp::pAudioPlay, (PVOID)0x439ce0);
  FromPtr(SakuraApp::pAudioStop, (PVOID)0x439dc0);
  FromPtr(SakuraApp::pTextTest, (PVOID)0x437a60);
}

int WINAPI MessageBoxA_(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption,
                        UINT uType) {
  int Size;
  LPWSTR lpTextW, lpCaptionW;
  do {
    Size = MultiByteToWideChar(932, 0, lpText, -1, NULL, 0);
    if (!Size) {
      break;
    }
    lpTextW = new WCHAR[Size];
    Size = MultiByteToWideChar(932, 0, lpText, -1, lpTextW, Size);
    if (!Size) {
      delete[] lpTextW;
      break;
    }
    Size = MultiByteToWideChar(932, 0, lpCaption, -1, NULL, 0);
    if (!Size) {
      break;
    }
    lpCaptionW = new WCHAR[Size];
    Size = MultiByteToWideChar(932, 0, lpCaption, -1, lpCaptionW, Size);
    if (!Size) {
      delete[] lpCaptionW;
      delete[] lpTextW;
      break;
    }
    int Result;
    Result = MessageBoxW(hWnd, lpTextW, lpCaptionW, uType);
    delete[] lpCaptionW;
    delete[] lpTextW;
    return Result;
  } while (false);
  return (*pMessageBoxA)(hWnd, lpText, lpCaption, uType);
}

HWND WINAPI CreateWindowExA_(DWORD dwExStyle, LPCSTR lpClassName,
                             LPCSTR lpWindowName, DWORD dwStyle, int X, int Y,
                             int nWidth, int nHeight, HWND hWndParent,
                             HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
  if (!strcmp(lpWindowName,
              "\x82\xb3\x82\xad\x82\xe7\x81\x41\x82\xe0\x82\xe4"
              "\x81\x42 -as the Night's, Reincarnation-")) {
    WCHAR lpWindowNameW[NAME_SIZE], lpClassNameW[NAME_SIZE];
    MultiByteToWideChar(932, 0, lpClassName, -1, lpClassNameW, NAME_SIZE);
    if (VMENV::pSubMap) {
      wsprintfW(lpWindowNameW,
                L"樱花、萌放 -as the Night's, Reincarnation- 文本版本-"
                L"%4hd/%02hd/%02hd-%02hd",
                VMENV::pSubMap->Time.wYear, VMENV::pSubMap->Time.wMonth,
                VMENV::pSubMap->Time.wDay, VMENV::pSubMap->Time.wHour);
    } else {
      wcscpy_s(lpWindowNameW, L"樱花、萌放 -as the Night's, Reincarnation-");
    }
    return CreateWindowExW(dwExStyle, lpClassNameW, lpWindowNameW, dwStyle, X,
                           Y, nWidth, nHeight, hWndParent, hMenu, hInstance,
                           lpParam);
  } else {
    return (*pCreateWindowExA)(dwExStyle, lpClassName, lpWindowName, dwStyle, X,
                               Y, nWidth, nHeight, hWndParent, hMenu, hInstance,
                               lpParam);
  }
}

INT WINAPI StrcmpIA_(LPCSTR lpString1, LPCSTR lpString2) {
  return CompareStringA(0x411, NORM_IGNORECASE, lpString1, -1, lpString2, -1) -
         2;
}

int IsLeadByte(UINT Char) {
  if (doTextPatch) {
    return (0x80 < Char && Char < 0xff);
  } else {
    return (0x80 < Char && Char < 0xa0) || (0xe0 <= Char && Char < 0xef);
  }
}

void AttachGlobal() {
  DetourAttach(&((PVOID &)pMessageBoxA), ToPtr(&MessageBoxA_));
  DetourAttach(&((PVOID &)pCreateWindowExA), ToPtr(&CreateWindowExA_));
  DetourAttach(&((PVOID &)pStrcmpIA), ToPtr(&StrcmpIA_));
  DetourAttach(&((PVOID &)pIsLeadByte), ToPtr(&IsLeadByte));
  DetourAttach(&((PVOID &)HEAPBLOCK::pReadFile), ToPtr(&HEAPBLOCK::ReadFile));
}
void DetachGlobal() {
  DetourDetach(&((PVOID &)HEAPBLOCK::pReadFile), ToPtr(&HEAPBLOCK::ReadFile));
  DetourDetach(&((PVOID &)pIsLeadByte), ToPtr(&IsLeadByte));
  DetourDetach(&((PVOID &)pStrcmpIA), ToPtr(&StrcmpIA_));
  DetourDetach(&((PVOID &)pCreateWindowExA), ToPtr(&CreateWindowExA_));
  DetourDetach(&((PVOID &)pMessageBoxA), ToPtr(&MessageBoxA_));
}

static DWORD dwCharSet, pFaceName;
void AttachTextSub() {
  VMENV::pSubMap = new StringMap(L"" PATH_TEXT);
  dwCharSet = PatchDoubleWord((PVOID)0x444364, GB2312_CHARSET);
  PatchDoubleWord((PVOID)0x42d22e, GB2312_CHARSET);
  pFaceName = PatchDoubleWord((PVOID)0x443b3a, (DWORD) "CHINESE_GB2312");
  DetourAttach(&((PVOID &)VMENV::pLoadText), ToPtr(&VMENV::LoadText));
  DetourAttach(&((PVOID &)SakuraApp::pTextTest), ToPtr(&SakuraApp::TextTest));
}
void DetachTextSub() {
  PatchDoubleWord((PVOID)0x443b3a, pFaceName);
  PatchDoubleWord((PVOID)0x42d22e, dwCharSet);
  PatchDoubleWord((PVOID)0x444364, dwCharSet);
  DetourDetach(&((PVOID &)SakuraApp::pTextTest), ToPtr(&SakuraApp::TextTest));
  DetourDetach(&((PVOID &)VMENV::pLoadText), ToPtr(&VMENV::LoadText));
  delete VMENV::pSubMap;
  VMENV::pSubMap = NULL;
}

void AttachImage() {
  DetourAttach(&((PVOID &)FileSys::pOpen), ToPtr(&FileSys::Open));
}
void DetachImage() {
  DetourAttach(&((PVOID &)FileSys::pOpen), ToPtr(&FileSys::Open));
}

void AttachSub() {
  DetourAttach(&((PVOID &)pDirect3DCreate9), ToPtr(&OverlayD3DCreate));
  DetourAttach(&((PVOID &)SakuraApp::pSysMovie), ToPtr(&SakuraApp::SysMovie));
  DetourAttach(&((PVOID &)SakuraApp::pSysMovieStop),
               ToPtr(&SakuraApp::SysMovieStop));
  DetourAttach(&((PVOID &)SakuraApp::pAudioLoad), ToPtr(&SakuraApp::AudioLoad));
  DetourAttach(&((PVOID &)SakuraApp::pAudioPlay), ToPtr(&SakuraApp::AudioPlay));
  DetourAttach(&((PVOID &)SakuraApp::pAudioStop), ToPtr(&SakuraApp::AudioStop));
}
void DetachSub() {
  DetourDetach(&((PVOID &)SakuraApp::pAudioStop), ToPtr(&SakuraApp::AudioStop));
  DetourDetach(&((PVOID &)SakuraApp::pAudioPlay), ToPtr(&SakuraApp::AudioPlay));
  DetourDetach(&((PVOID &)SakuraApp::pAudioLoad), ToPtr(&SakuraApp::AudioLoad));
  DetourDetach(&((PVOID &)SakuraApp::pSysMovieStop),
               ToPtr(&SakuraApp::SysMovieStop));
  DetourDetach(&((PVOID &)SakuraApp::pSysMovie), ToPtr(&SakuraApp::SysMovie));
  DetourDetach(&((PVOID &)pDirect3DCreate9), ToPtr(&OverlayD3DCreate));
}

BOOL TestFile(LPCSTR lpPath) {
  HANDLE hTestFile;

  hTestFile = CreateFileA(lpPath, GENERIC_READ, FILE_SHARE_READ, NULL,
                          OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
  if (hTestFile != INVALID_HANDLE_VALUE) {
    CloseHandle(hTestFile);
    return TRUE;
  }
  return FALSE;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ulReason, LPVOID lpReserved) {
  UNREFERENCED_PARAMETER(lpReserved);
  if (DetourIsHelperProcess()) {
    return TRUE;
  }
  hSakuraExe = GetModuleHandleW(L"" PATH_EXEC);
  if (hSakuraExe == NULL) {
    hSakuraExe = GetModuleHandleW(L"SakuraF.exe");
  }
  if (hSakuraExe == NULL) {
    GetModuleFileNameA(hModule, lpDllPath, NAME_SIZE);
    return TRUE;
  }
  switch (ulReason) {
    case DLL_PROCESS_ATTACH:
#ifdef DEBUG_LOG
      AllocConsole();
      SetConsoleOutputCP(65001);
#endif
      doTextPatch = TestFile(PATH_TEXT);
      doImagePatch = TestFile(PATH_IMAGE);
      doSubPatch = TestFile(PATH_SUB VD1_NAME) && TestFile(PATH_SUB VD2_NAME) &&
                   TestFile(PATH_SUB ED1_NAME) && TestFile(PATH_SUB ED2_NAME);
      // DebugLog(0, "Configuration: Text %d, Image %d, Subtitle: %d\n",
      //          doTextPatch, doImagePatch, doSubPatch);

      if (doSubPatch) {
        hDirect3D9Library = LoadLibraryA("d3d9.dll");
        if (!hDirect3D9Library) {
          MessageBoxW(GetDesktopWindow(),
                      L"无法载入Direct3D9，请检查是否安装了相应的运行库",
                      L"错误", MB_ICONSTOP);
          return FALSE;
        }
        pDirect3DCreate9 = (PFUNC_Direct3DCreate9)GetProcAddress(
            hDirect3D9Library, "Direct3DCreate9");
      } else {
        hDirect3D9Library = NULL;
        pDirect3DCreate9 = NULL;
      }
      SubtitleInit();
      InitGlobal();

      DetourRestoreAfterWith();
      DetourTransactionBegin();
      DetourUpdateThread(GetCurrentThread());
      AttachGlobal();
      if (doTextPatch) {
        AttachTextSub();
        if (!VMENV::pSubMap || !VMENV::pSubMap->Exist()) {
          MessageBoxW(GetDesktopWindow(), L"文本载入失败", L"错误",
                      MB_ICONSTOP);
          ExitProcess(1);
        }
      }
      if (doImagePatch) {
        AttachImage();
      }
      if (doSubPatch) {
        AttachSub();
      }
      if (DetourTransactionCommit() != NO_ERROR) {
        MessageBoxW(GetDesktopWindow(),
                    L"无法挂载函数接口，或许您使用了错误的游戏版本", L"错误",
                    MB_OK | MB_ICONSTOP);
        return FALSE;
      }
      break;
    case DLL_PROCESS_DETACH:
      DetourTransactionBegin();
      DetourUpdateThread(GetCurrentThread());
      if (doSubPatch) {
        DetachSub();
      }
      if (doImagePatch) {
        DetachImage();
      }
      if (doTextPatch) {
        DetachTextSub();
      }

      DetachGlobal();
      DetourTransactionCommit();
      SubtitleFini();
      FreeLibrary(hDirect3D9Library);
#ifdef DEBUG_LOG
      FreeConsole();
#endif
      break;
    default:
      break;
  }
  return TRUE;
}

EXTERN_C
int CDECL StartExecutable() {
  STARTUPINFOA siSakura;
  PROCESS_INFORMATION piSakura;
  BOOL bOk;

  ZeroMemory(&siSakura, sizeof(siSakura));
  ZeroMemory(&piSakura, sizeof(piSakura));
  siSakura.cb = sizeof(siSakura);
  siSakura.dwFlags = STARTF_USESHOWWINDOW;
  siSakura.wShowWindow = SW_SHOW;

  bOk = DetourCreateProcessWithDllExA(PATH_EXEC, NULL, NULL, NULL, FALSE,
                                      CREATE_DEFAULT_ERROR_MODE, NULL, NULL,
                                      &siSakura, &piSakura, lpDllPath, NULL);
  if (!bOk) {
    MessageBoxW(GetDesktopWindow(), L"可执行文件加载失败", L"错误",
                MB_ICONSTOP);
    CloseHandle(&siSakura);
    CloseHandle(&piSakura);
    return 1;
  }
  CloseHandle(&siSakura);
  CloseHandle(&piSakura);
  return 0;
}
