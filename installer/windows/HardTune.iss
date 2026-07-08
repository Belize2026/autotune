; Inno Setup script for the HardTune Windows installer.
; Built in CI with:  ISCC.exe /O"dist-installer" installer\windows\HardTune.iss
; (paths below are relative to this file's directory)

#define AppVersion "0.1.0"

[Setup]
AppId={{D4C0FFEE-2026-4A11-B0B0-00C0FFEE2026}
AppName=HardTune
AppVersion={#AppVersion}
AppPublisher=Belize2026
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
OutputBaseFilename=HardTune-Installer-Windows
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
UninstallDisplayName=HardTune (VST3 plugin)

[Files]
Source: "..\..\build\HardTune_artefacts\Release\VST3\HardTune.vst3\*"; \
    DestDir: "{commoncf64}\VST3\HardTune.vst3"; \
    Flags: recursesubdirs ignoreversion

[Messages]
SetupWindowTitle=HardTune Setup
FinishedLabelNoIcons=HardTune has been installed. Rescan your plugins in your DAW and it will show up as "HardTune" under Belize2026.
