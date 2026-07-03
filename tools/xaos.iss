#define XaoS_VERSION "4.3.6"

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
Name: "it"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "pt"; MessagesFile: "compiler:Languages\Portuguese.isl"
Name: "tr"; MessagesFile: "compiler:Languages\Turkish.isl"
Name: "hy"; MessagesFile: "compiler:Languages\Armenian.isl"
Name: "cz"; MessagesFile: "compiler:Languages\Czech.isl"
Name: "he"; MessagesFile: "compiler:Languages\Hebrew.isl"
Name: "sv"; MessagesFile: "compiler:Languages\Swedish.isl"
Name: "ru"; MessagesFile: "compiler:Languages\Russian.isl"
; Name: "rs"; MessagesFile: "compiler:Languages\Unofficial\SerbianLatin.isl"
; Name: "ro"; MessagesFile: "compiler:Languages\Unofficial\Romanian.isl"
; Name: "zh"; MessagesFile: "compiler:Languages\Unofficial\ChineseSimplified.isl"
; Name: "hi"; MessagesFile: "compiler:Languages\Unofficial\Hindi.islu"
; Name: "is"; MessagesFile: "compiler:Languages\Unofficial\Icelandic.isl"
; Name: "vi"; MessagesFile: "compiler:Languages\Unofficial\Vietnamese.isl"

[Run]
Filename: {app}\XaoS.exe; Description: Start XaoS now; Flags: postinstall nowait skipifsilent
