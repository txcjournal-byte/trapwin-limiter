; Inno Setup script - TrapWin Limiter Windows installer
; Build:  iscc /DAppVersion=1.0.0 /DSrcDir=<staging dir> /DOutDir=<output dir> TrapWinLimiter.iss
; <staging dir> must contain: TrapLimit.vst3\, EULA.txt,
;                             THIRD-PARTY-LICENSES.txt, TrapWin Limiter - Manual.pdf

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef SrcDir
  #define SrcDir "..\..\out\stage-win"
#endif
#ifndef OutDir
  #define OutDir "..\..\out"
#endif

[Setup]
; AppId identifies the product for upgrades/uninstall - never change it
AppId={{6F2B8C1E-4D7A-4E1B-9C3F-7A5D2E8B9C41}
AppName=TrapWin Limiter
AppVersion={#AppVersion}
AppVerName=TrapWin Limiter {#AppVersion}
AppPublisher=TrapWin
DefaultDirName={autopf}\TrapWin\TrapWin Limiter
DefaultGroupName=TrapWin
DisableProgramGroupPage=yes
DisableDirPage=yes
LicenseFile={#SrcDir}\EULA.txt
OutputDir={#OutDir}
OutputBaseFilename=TrapWin_Limiter_v{#AppVersion}_Windows_Setup
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
MinVersion=10.0
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=TrapWin Limiter
CloseApplications=yes

[Files]
Source: "{#SrcDir}\TrapLimit.vst3\*"; DestDir: "{commoncf64}\VST3\TrapLimit.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SrcDir}\TrapWin Limiter - Manual.pdf"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SrcDir}\EULA.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SrcDir}\THIRD-PARTY-LICENSES.txt"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\TrapWin Limiter Manual"; Filename: "{app}\TrapWin Limiter - Manual.pdf"
Name: "{group}\Uninstall TrapWin Limiter"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\TrapWin Limiter - Manual.pdf"; Description: "Open the user manual"; Flags: postinstall shellexec skipifsilent unchecked

[Messages]
FinishedLabel=TrapWin Limiter has been installed.%n%nOpen your DAW and rescan plugins (FL Studio: Options > Manage plugins > Find installed plugins). You will find it as "TrapWin Limiter" by TrapWin.
