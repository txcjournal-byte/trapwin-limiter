; Inno Setup script for BIT ERA by TrapWin (Windows, VST3 + Standalone)
; Build: ISCC.exe /DAppVersion=0.1.0 bitera\installer\win\bitera.iss
#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif

[Setup]
AppId={{9B2F4E61-3C7A-4D8E-A1B5-7E0C2D9F4A31}
AppName=Bit Era by TrapWin
AppVersion={#AppVersion}
AppPublisher=TrapWin
AppPublisherURL=https://trapwin.com
DefaultDirName={commonpf64}\TrapWin\Bit Era
DisableDirPage=yes
DisableProgramGroupPage=yes
Uninstallable=yes
LicenseFile=..\EULA.txt
OutputDir=..\..\build-bitera\installer
OutputBaseFilename=BIT-ERA-{#AppVersion}-Windows-Setup
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes

[Files]
Source: "..\..\build-bitera\BitEra_artefacts\Release\VST3\Bit Era.vst3\*"; DestDir: "{commoncf64}\VST3\Bit Era.vst3"; Excludes: "*.pdb,*.ilk"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\..\build-bitera\BitEra_artefacts\Release\Standalone\Bit Era.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\EULA.txt"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{commonprograms}\Bit Era"; Filename: "{app}\Bit Era.exe"

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Bit Era.vst3"
