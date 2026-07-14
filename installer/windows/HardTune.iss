; Inno Setup script for the AUTISMIDOL AUTOTUNE Windows installer.
; Built in CI with:  ISCC.exe /O"dist-installer" installer\windows\HardTune.iss
; (paths below are relative to this file's directory)

#define AppVersion "0.2.1"

[Setup]
AppId={{D4C0FFEE-2026-4A11-B0B0-00C0FFEE2026}
AppName=AUTISMIDOL AUTOTUNE
AppVersion={#AppVersion}
AppPublisher=Belize2026
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
OutputBaseFilename=AUTISMIDOL-AUTOTUNE-Installer-Windows
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
UninstallDisplayName=AUTISMIDOL AUTOTUNE (VST3 plugin)

[Files]
Source: "..\..\build\HardTune_artefacts\Release\VST3\AUTISMIDOL AUTOTUNE.vst3\*"; \
    DestDir: "{commoncf64}\VST3\AUTISMIDOL AUTOTUNE.vst3"; \
    Flags: recursesubdirs ignoreversion

[Messages]
SetupWindowTitle=AUTISMIDOL AUTOTUNE Setup
FinishedLabelNoIcons=AUTISMIDOL AUTOTUNE has been installed. Rescan your plugins in your DAW and it will show up as "AUTISMIDOL AUTOTUNE" under Belize2026.
