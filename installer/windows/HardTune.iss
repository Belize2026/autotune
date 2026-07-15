; Inno Setup script for the AUTISMIDOL AUTOTUNE Windows installer.
; Built in CI with:  ISCC.exe /O"dist-installer" installer\windows\HardTune.iss
; (paths below are relative to this file's directory)

#define AppVersion "0.8.0"

[Setup]
AppId={{D4C0FFEE-2026-4A11-B0B0-00C0FFEE2026}
AppName=AUTISMIDOL AUTOTUNE
AppVersion={#AppVersion}
AppPublisher=AutismIdol
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

[InstallDelete]
; Wipe previously installed copies (including the legacy "HardTune" name)
; before installing, so upgrades never leave stale versions behind.
Type: filesandordirs; Name: "{commoncf64}\VST3\HardTune.vst3"
Type: filesandordirs; Name: "{commoncf64}\VST3\AUTISMIDOL AUTOTUNE.vst3"

[Files]
Source: "..\..\build\HardTune_artefacts\Release\VST3\AUTISMIDOL AUTOTUNE.vst3\*"; \
    DestDir: "{commoncf64}\VST3\AUTISMIDOL AUTOTUNE.vst3"; \
    Flags: recursesubdirs ignoreversion

[Messages]
SetupWindowTitle=AUTISMIDOL AUTOTUNE Setup
FinishedLabelNoIcons=AUTISMIDOL AUTOTUNE has been installed. Rescan your plugins in your DAW and it will show up as "AUTISMIDOL AUTOTUNE" under Belize2026.
