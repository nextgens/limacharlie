!include x64.nsh
!include "Registry.nsh"
!include "Sections.nsh"
!include "FileFunc.nsh"

!ifndef RPHCPDIR
    !error "Please define the path to the directory containing HCP builds with the -DRPHCPDIR=<path> parameter."
!endif
!ifndef OUTPUT
    !error "Please specify the path to the output directory where to write the installer with the -DOUTPUT=<path> parameter."
!endif
!ifndef INSTNAME
    !error "Please specify the name of the installer file to write with the -DINSTNAME=<name> parameter."
!endif
!ifndef LICENSEKEY
    !error "Please specify the license key of the client with the -DLICENSEKEY=<key> parameter."
!endif
!define RPHCP32  "${RPHCPDIR}\32\rphcp.dll"
!define RPHCP64  "${RPHCPDIR}\64\rphcp.dll"
!define OUTINST  "${OUTPUT}\${INSTNAME}"

Name "RP HCP Install"
OutFile "${OUTINST}"

RequestExecutionLevel admin

SilentInstall silent

Function .onInit
    ${If} ${RunningX64}
    ;${EnableX64FSRedirection} ;- only if needed
    StrCpy "$INSTDIR" "$WINDIR\Sysnative"
    ${Else}
    StrCpy "$INSTDIR" "$WINDIR\System32"
    ${EndIf}
FunctionEnd

Section "RP HCP"
  SectionIn RO
  ; Set output path to the installation directory
  SetOutPath $INSTDIR

  Delete ".\rphcp.log"

  ; Set up an installation log. If no problem crop up, it will be deleted.
  ${GetParameters} $0
  ClearErrors
  ${GetOptions} $0 /LICENSEKEY= $1
  StrCmp "$1" "${LICENSEKEY}" license_key_ok
  FileOpen $2 ".\rphcp.log" w
  FileWrite $2 "Input license key `$1' does not match the one you were supplied. Installation aborted.$\n"
  FileClose $2
  Quit

license_key_ok:

  ; Put file there
  ${If} ${RunningX64}
  File "/oname=rphcp.dll" "${RPHCP64}"
  ${Else}
  File "/oname=rphcp.dll" "${RPHCP32}"
  ${EndIf}

  ${If} ${RunningX64}
    ${DisableX64FSRedirection}
    SetRegView 64
  ${EndIf}
  
  ExecWait 'sc create RPHCP binPath= "%SystemRoot%\system32\svchost.exe -k as" type= share start= auto'
  Sleep 1000 

  ${registry::Write} "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Svchost" "as" "RPHCP" "REG_MULTI_SZ" $R0
  ${registry::Write} "HKLM\System\CurrentControlSet\Services\rphcp" "DependOnService" "Tcpip" "REG_MULTI_SZ" $R0
  ${registry::Write} "HKLM\System\CurrentControlSet\Services\rphcp" "Description" "RPHCP" "REG_SZ" $R0
  ${registry::Write} "HKLM\System\CurrentControlSet\Services\rphcp" "DisplayName" "RPHCP" "REG_SZ" $R0
  ${registry::Write} "HKLM\System\CurrentControlSet\Services\rphcp" "ErrorControl" "0" "REG_DWORD" $R0
  ${registry::Write} "HKLM\System\CurrentControlSet\Services\rphcp" "FailureActions" "80510100000000000000000003000000140000000100000060EA00000100000060EA00000000000000000000" "REG_BINARY" $R0
  ${registry::Write} "HKLM\System\CurrentControlSet\Services\rphcp" "ImagePath" "%SystemRoot%\system32\svchost.exe -k as" "REG_EXPAND_SZ" $R0
  ${registry::Write} "HKLM\System\CurrentControlSet\Services\rphcp" "ObjectName" "LocalSystem" "REG_SZ" $R0
  ${registry::Write} "HKLM\System\CurrentControlSet\Services\rphcp" "Start" "2" "REG_DWORD" $R0
  ${registry::Write} "HKLM\System\CurrentControlSet\Services\rphcp" "Type" "272" "REG_DWORD" $R0
  ${registry::Write} "HKLM\System\CurrentControlSet\Services\rphcp\parameters" "ServiceDll" "%SystemRoot%\system32\rphcp.dll" "REG_EXPAND_SZ" $R0

  ExecWait 'sc start RPHCP'

SectionEnd

