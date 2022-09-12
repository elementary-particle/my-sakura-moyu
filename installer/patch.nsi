Unicode true
ManifestDPIAware true

!define APPNAME "SakuraMoyu Zh_Hans Patch"
!define COMPANYNAME "樱花萌放"
!define DESCRIPTION "中文补丁"

!define PRODUCT_REGKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\Favo_GamesId_fvg_SAKURA_is1"
!define GAME_EXE_SHA1 "688CC80CDD358EFFD80E2BF8E28FE811FE3B436E"

!define VERSIONMAJOR 0
!define VERSIONMINOR 1
!define VERSIONBUILD 6

RequestExecutionLevel admin
InstallDir "$PROGRAMFILES\Favorite\さくら、もゆ。"
InstallDirRegKey HKLM "${PRODUCT_REGKEY}" InstallLocation
Name "${COMPANYNAME}-${DESCRIPTION}-${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}"
Icon "icon\inst.ico"
UninstallIcon "icon\uninst.ico"
OutFile "patch_setup_v${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}.exe"

SetCompressor LZMA

Function .onInit
FunctionEnd

Function un.onInit
FunctionEnd

!define MUI_ICON "icon\inst.ico"
!define MUI_UNICON "icon\uninst.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP "icon\welcome.bmp"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "icon\header.bmp"
!define MUI_HEADERIMAGE_RIGHT
!define MUI_ABORTWARNING
!include "MUI.nsh"

Function checkDir
  IfFileExists "$INSTDIR\uninst_patch.exe" hasOldVer checkExec
  hasOldVer:
  MessageBox MB_OK|MB_ICONEXCLAMATION "检查到旧版本的安装，请先卸载旧版本，并确保删除 UNINST_PATCH"
  Abort
  checkExec:
  IfFileExists "$INSTDIR\Sakura.exe" checkHash noExec
  noExec:
  MessageBox MB_OK|MB_ICONSTOP|MB_YESNO "目录下没有发现游戏可执行文件，继续执行安装？" IDYES checkOk
  Abort
  checkHash:
  Crypto::HashFile "SHA1" "$INSTDIR\Sakura.exe"
  Pop $0
  StrCmp $0 "${GAME_EXE_SHA1}" checkOk hashFail
  hashFail:
  MessageBox MB_OK|MB_ICONEXCLAMATION "可执行文件校验和错误，可能使用了不兼容的游戏版本"
  checkOk:
FunctionEnd

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE checkDir
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "SimpChinese"

InstType "全部补丁"

Section "二进制核心" Section0
  SectionIn 1 RO
  SetOutPath "$INSTDIR\"
  File /r "bin\libass-9.dll"
  File /r "bin\filter.dll"
  File /oname=$INSTDIR\SakuraChs.exe "bin\loader.exe"
  File /oname=$INSTDIR\PATCH_LICENSE.txt "LICENSE.txt"
  File /oname=$INSTDIR\PATCH_AUTHORS.txt "AUTHORS.txt"
SectionEnd

Section "文本" Section1
  SectionIn 1
  SetOutPath "$INSTDIR\"
  File /r "static\patch.dat"
SectionEnd

Section "图形" Section2
  SectionIn 1
  SetOutPath "$INSTDIR\"
  GetTempFileName $0
  GetTempFileName $1
  File /oname=$0 "util\fptool.exe"
  File /oname=$1 "static\diff.bin"
  DetailPrint "应用图形补丁......"
  nsExec::Exec '"$0" -p -i "$1" -o "$INSTDIR\patch.bin"'
  Pop $2
  StrCmp $2 0 fptoolOk fptoolFail
  fptoolFail:
  MessageBox MB_OK|MB_ICONEXCLAMATION "图形补丁失败，错误码 $2"
  fptoolOk:
  Delete $1
  Delete $0
SectionEnd

Section "中日特效字幕" Section3
  SectionIn 1
  SetOutPath "$INSTDIR\subtitle"
  File /r "static\subtitle\*.*"
SectionEnd

Section "贡献者提供的额外项目" Section4
  SectionIn 1
  SetOutPath "$INSTDIR\extra"
  File /r "extra\樱花摸鱼大事记.doc"
SectionEnd

Section -FinishSection
  WriteUninstaller "$INSTDIR\uninst_patch.exe"
SectionEnd

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${Section0} "补丁核心"
  !insertmacro MUI_DESCRIPTION_TEXT ${Section1} "安装文本翻译"
  !insertmacro MUI_DESCRIPTION_TEXT ${Section2} "安装日文图形翻译"
  !insertmacro MUI_DESCRIPTION_TEXT ${Section3} "给动画嵌入中日字幕"
  !insertmacro MUI_DESCRIPTION_TEXT ${Section4} "贡献者提供的额外项目"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Section Uninstall
  SectionIn 1
  Delete "$INSTDIR\extra\樱花摸鱼大事记.doc"
  Delete "$INSTDIR\subtitle\fonts\*.*"
  RMDir "$INSTDIR\subtitle\fonts"
  Delete "$INSTDIR\subtitle\*.*"
  RMDir "$INSTDIR\subtitle"
  Delete "$INSTDIR\patch.bin"
  Delete "$INSTDIR\patch.dat"
  Delete "$INSTDIR\PATCH_AUTHORS.txt"
  Delete "$INSTDIR\PATCH_LICENSE.txt"
  Delete "$INSTDIR\SakuraChs.exe"
  Delete "$INSTDIR\filter.dll"
  Delete "$INSTDIR\libass-9.dll"
  Delete "$INSTDIR\uninst_patch.exe"
SectionEnd
