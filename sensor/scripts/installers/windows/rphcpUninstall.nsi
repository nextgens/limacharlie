!include x64.nsh
!include "Registry.nsh"
!include "Sections.nsh"

!ifndef OUTPUT
  !error "Please specify the path to the output uninstaller with the -DOUTPUT=<path> parameter."
!endif

Name "SilentRPHCPUninst"
OutFile "${OUTPUT}"

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


; Uninstaller

Section "SilentRPHCPUninst"

  ; stop RPHCP
  nsExec::Exec /TIMEOUT=5000 "net stop RPHCP"

  ${If} ${RunningX64}
    ${DisableX64FSRedirection}
    SetRegView 64
  ${EndIf}  

  ; Delete the service
  ExecWait 'sc delete RPHCP'
  Sleep 1000

  ; Remove registry keys
  ${registry::DeleteValue} "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Svchost" "as" $R0
  ${registry::DeleteKey} "HKLM\System\CurrentControlSet\Services\rphcp" $R0
  
  ; Remove files and uninstaller
  Delete "$INSTDIR\rphcp.dll"

SectionEnd
