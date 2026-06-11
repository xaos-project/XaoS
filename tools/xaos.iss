#define XaoS_VERSION "4.3.5"

[Setup]
AppName=XaoS
AppVersion={#XaoS_VERSION}
WizardStyle=modern
DefaultDirName={commonpf32}\XaoS
DefaultGroupName=XaoS
UninstallDisplayIcon={app}\xaos.ico
Compression=lzma2
SolidCompression=yes
OutputBaseFilename=xaos-setup

[Files]
Source: "xaos-{#XaoS_VERSION}\*.*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs

[Icons]
Name: "{group}\XaoS"; Filename: "{app}\XaoS.exe"; IconFilename: "{app}\xaos.ico"
Name: "{group}\Uninstall XaoS"; Filename: "{uninstallexe}"; IconFilename: "{app}\xaos.ico"
Name: "{autoprograms}\XaoS"; Filename: "{app}\XaoS.exe"; IconFilename: "{app}\xaos.ico"
Name: "{autodesktop}\XaoS"; Filename: "{app}\XaoS.exe"; IconFilename: "{app}\xaos.ico"

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "de"; MessagesFile: "compiler:Languages\German.isl"
Name: "hu"; MessagesFile: "compiler:Languages\Hungarian.isl"
Name: "es"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "fr"; MessagesFile: "compiler:Languages\French.isl"

[Run]
Filename: {app}\XaoS.exe; Description: Start XaoS now; Flags: postinstall nowait skipifsilent
