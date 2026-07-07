; ======================================================================
; VectorGraphicsEditor — NSIS Installer Script
; ======================================================================
; Runs from the portable directory (alongside the .exe and Qt DLLs).
; Usage: cd dist/portable && makensis installer.nsi -DVERSION=x.y.z
; ======================================================================

!include "MUI2.nsh"
!include "FileFunc.nsh"

!ifndef APP_VERSION
  !define APP_VERSION "0.0.0"
!endif
!ifndef OUTFILE
  !define OUTFILE "VectorGraphicsEditor-Setup.exe"
!endif

Name "VectorGraphicsEditor ${APP_VERSION}"
OutFile "${OUTFILE}"
InstallDir "$PROGRAMFILES64\VectorGraphicsEditor"
RequestExecutionLevel admin

!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

Section "Install"
  SetOutPath "$INSTDIR"

  ; Exclude the NSIS script itself and the generated installer from packaging
  File /r /x "installer.nsi" /x "${OUTFILE}" "*"

  WriteUninstaller "$INSTDIR\Uninstall.exe"

  CreateDirectory "$SMPROGRAMS\VectorGraphicsEditor"
  CreateShortcut "$SMPROGRAMS\VectorGraphicsEditor\VectorGraphicsEditor.lnk" "$INSTDIR\VectorGraphicsEditor.exe"
  CreateShortcut "$SMPROGRAMS\VectorGraphicsEditor\Uninstall.lnk" "$INSTDIR\Uninstall.exe"

  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\VectorGraphicsEditor" \
    "DisplayName" "VectorGraphicsEditor"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\VectorGraphicsEditor" \
    "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\VectorGraphicsEditor" \
    "DisplayVersion" "${APP_VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\VectorGraphicsEditor" \
    "Publisher" "CourseProject"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\VectorGraphicsEditor" \
    "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\VectorGraphicsEditor" \
    "NoRepair" 1
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\VectorGraphicsEditor" \
    "EstimatedSize" "$0"
SectionEnd

Section "Uninstall"
  RMDir /r "$INSTDIR"
  RMDir /r "$SMPROGRAMS\VectorGraphicsEditor"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\VectorGraphicsEditor"
SectionEnd
