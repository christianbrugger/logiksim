; Script for generating Windows Installation Package.
;
; Run the script with Inno Setup from 
;   https://jrsoftware.org/isinfo.php
;

#define MyAppName "LogikSim"
#define MyAppVersion "2.2.0"
#define MyAppPublisher "Christian Brugger"
#define MyAppExeName "logiksim.exe"
#define MyAppAssocName MyAppName + " Circuit"
#define MyAppAssocExt ".ls2"
#define MyAppAssocKey StringChange(MyAppAssocName, " ", "") + MyAppAssocExt
#define MyAppIconFile "{app}/resources/icons/derivative/app_icon_256.ico"

[Setup]

AppId={{2C447801-6DB7-43CE-8641-DAD122B2A15F}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
UninstallDisplayName={#MyAppName}
UninstallDisplayIcon={#MyAppIconFile}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DisableDirPage=no
DisableReadyPage=yes
DisableProgramGroupPage=yes
ChangesAssociations=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; lowest | admin
PrivilegesRequired=lowest
; commandline | dialog
PrivilegesRequiredOverridesAllowed=dialog
OutputDir=temp
OutputBaseFilename={#MyAppName}_{#MyAppVersion}_win_x64
SetupIconFile="temp/deploy/resources/icons/derivative/app_package_256.ico"
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "temp\deploy\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Registry]
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocExt}\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppAssocKey}"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocKey}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppAssocName}"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocKey}\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{#MyAppIconFile},0"
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocKey}\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: "{#MyAppAssocExt}"; ValueData: ""

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{#MyAppIconFile}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{#MyAppIconFile}"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

