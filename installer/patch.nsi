Unicode true
ManifestDPIAware true

!define APPNAME "SakuraMoyu Zh_Hans Patch"
!define COMPANYNAME "樱花萌放"
!define DESCRIPTION "中文补丁"

!define PRODUCT_REGKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\Favo_GamesId_fvg_SAKURA_is1"
!define GAME_EXE_SHA1 "688CC80CDD358EFFD80E2BF8E28FE811FE3B436E"

!define VERSIONMAJOR 0
!define VERSIONMINOR 1
!define VERSIONBUILD 8

RequestExecutionLevel admin
InstallDir "$PROGRAMFILES\Favorite\さくら、もゆ。\"
InstallDirRegKey HKLM "${PRODUCT_REGKEY}" InstallLocation
Name "${COMPANYNAME}-${DESCRIPTION}-${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}"
Icon "icon\inst.ico"
UninstallIcon "icon\uninst.ico"
OutFile "patch_setup_v${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}.exe"

SetCompressor LZMA

!define MUI_ICON "icon\inst.ico"
!define MUI_UNICON "icon\uninst.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP "icon\welcome.bmp"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "icon\header.bmp"
!define MUI_HEADERIMAGE_RIGHT
!define MUI_ABORTWARNING
!include "MUI2.nsh"

!include "Sections.nsh"

Function checkDir
  IfFileExists "$INSTDIR\uninst_patch.exe" hasOldVer checkExec
  hasOldVer:
  MessageBox MB_OK|MB_ICONEXCLAMATION "检查到旧版本的安装，请先卸载旧版本"
  Exec "$INSTDIR\uninst_patch.exe"
  Abort
  checkExec:
  IfFileExists "$INSTDIR\Sakura.exe" checkHash noExec
  noExec:
  MessageBox MB_OK|MB_ICONSTOP|MB_YESNO "目录下没有发现游戏可执行文件，继续执行安装（警告：这很可能导致图形补丁无法正确应用）？" IDYES checkOk
  Abort
  checkHash:
  Crypto::HashFile "SHA1" "$INSTDIR\Sakura.exe"
  Pop $0
  StrCmp $0 "${GAME_EXE_SHA1}" checkOk hashFail
  hashFail:
  MessageBox MB_OK|MB_ICONEXCLAMATION "可执行文件校验和错误，可能使用了不兼容的游戏版本"
  checkOk:
FunctionEnd

Var StartMenuFolder

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE checkDir
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "樱花萌放"
!define MUI_STARTMENUPAGE_REGISTRY_ROOT HKLM
!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\SakuraMoyu_Patch"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "StartMenuFolder"
!insertmacro MUI_PAGE_STARTMENU "PatchStartMenuPage" $StartMenuFolder
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "SimpChinese"

InstType "全部补丁"

Section "二进制" Section0
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
  SectionIn 1 RO
  SetOutPath "$INSTDIR\"
  Delete "$INSTDIR\patch.bin"
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

SectionGroup /e "字幕" Section3

Section "ASS字幕" Section3All
  SectionIn 1 RO
  SetOutPath "$INSTDIR\subtitle"
  File /r "static\subtitle\*.*"
SectionEnd

Section "OP1翻译-五言+白话" Section30
  Rename "$INSTDIR\subtitle\vd1_v0.ass" "$INSTDIR\subtitle\vd1.ass"
SectionEnd

Section "OP1翻译-七言" Section31
  Rename "$INSTDIR\subtitle\vd1_v1.ass" "$INSTDIR\subtitle\vd1.ass"
SectionEnd

Section "OP1翻译-白话" Section32
  Rename "$INSTDIR\subtitle\vd1_v2.ass" "$INSTDIR\subtitle\vd1.ass"
SectionEnd

SectionGroupEnd

Section "额外项目" Section4
  SectionIn 1
  SetOutPath "$INSTDIR\extra"
  File /r "extra\樱花摸鱼大事记.doc"
SectionEnd

Section "工作人员的留言" Section5
  SectionIn 1
  SetOutPath "$INSTDIR\extra"
  File /oname=$INSTDIR\extra\留言.txt "COMMENT.txt"
SectionEnd

Section -FinishSection
  WriteUninstaller "$INSTDIR\uninst_patch.exe"
  SetOutPath "$INSTDIR"
  
  !insertmacro MUI_STARTMENU_WRITE_BEGIN "PatchStartMenuPage"
    
    CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
    CreateShortCut "$SMPROGRAMS\$StartMenuFolder\启动.lnk" "$INSTDIR\SakuraChs.exe"
    CreateShortCut "$SMPROGRAMS\$StartMenuFolder\卸载补丁.lnk" "$INSTDIR\uninst_patch.exe"
  
  !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Function .onInit
  Push $0
  Push $1
  SectionGetFlags ${Section3} $0
  IntOp $0 $0 | ${SF_RO}
  SectionSetFlags ${Section3} $0
  IntOp $1 ${SF_SELECTED} ~
  StrCpy $R9 ${Section30}
  SectionGetFlags ${Section30} $0
  IntOp $0 $0 | ${SF_SELECTED}
  SectionSetFlags ${Section30} $0
  SectionGetFlags ${Section31} $0
  IntOp $0 $0 & $1
  SectionSetFlags ${Section31} $0
  SectionGetFlags ${Section32} $0
  IntOp $0 $0 & $1
  SectionSetFlags ${Section32} $0
  Pop $1
  Pop $0
FunctionEnd

Function un.onInit
FunctionEnd

Function .onSelChange
  Push $0
  Push $1
  Push $2
  IntOp $1 ${SF_SELECTED} ~
  
  StrCmp $R9 ${Section30} sec1
  SectionGetFlags ${Section30} $0
  IntOp $0 $0 & ${SF_SELECTED}
  StrCpy $2 ${Section30}
  IntCmp $0 ${SF_SELECTED} deselCur
sec1:
  StrCmp $R9 ${Section31} sec2
  SectionGetFlags ${Section31} $0
  IntOp $0 $0 & ${SF_SELECTED}
  StrCpy $2 ${Section31}
  IntCmp $0 ${SF_SELECTED} deselCur
sec2:
  StrCmp $R9 ${Section32} sec3
  SectionGetFlags ${Section32} $0
  IntOp $0 $0 & ${SF_SELECTED}
  StrCpy $2 ${Section32}
  IntCmp $0 ${SF_SELECTED} deselCur
sec3:
  Goto end
deselCur:
  SectionGetFlags $R9 $0
  IntOp $0 $0 & $1
  SectionSetFlags $R9 $0
  StrCpy $R9 $2
end:
  Pop $2
  Pop $1
  Pop $0
FunctionEnd

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${Section0} "补丁核心"
  !insertmacro MUI_DESCRIPTION_TEXT ${Section1} "游戏文本和对话的翻译（不包括H部分）"
  !insertmacro MUI_DESCRIPTION_TEXT ${Section2} "图形文本翻译，包括界面、菜单、立绘名字和部分CG"
  !insertmacro MUI_DESCRIPTION_TEXT ${Section3All} "给开幕视频和末尾插入歌嵌入ASS字幕"
  !insertmacro MUI_DESCRIPTION_TEXT ${Section30} "使用五言和白话混合的版本"
  !insertmacro MUI_DESCRIPTION_TEXT ${Section31} "使用七言版本"
  !insertmacro MUI_DESCRIPTION_TEXT ${Section32} "使用白话版本"
  !insertmacro MUI_DESCRIPTION_TEXT ${Section4} "《樱花萌放》剧情梳理"
  !insertmacro MUI_DESCRIPTION_TEXT ${Section5} "想看看大家的感想吗？合作的乐趣就在于有各种各样的人"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Section Uninstall  
  !insertmacro MUI_STARTMENU_GETFOLDER "PatchStartMenuPage" $R0
  Delete "$SMPROGRAMS\$R0\卸载补丁.lnk"
  Delete "$SMPROGRAMS\$R0\启动.lnk"
  RMDir "$SMPROGRAMS\$R0"
  DeleteRegKey /ifempty HKLM "Software\SakuraMoyu_Patch"
  
  Delete "$INSTDIR\extra\留言.txt"
  Delete "$INSTDIR\extra\樱花摸鱼大事记.doc"
  RMDir "$INSTDIR\extra"
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
