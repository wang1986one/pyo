; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{4F72B6EF-CFA0-434F-A2B4-B130F94F54EB}
AppName=Pyo
AppVersion=0.7.1
AppPublisher=iACT.umontreal.ca
AppPublisherURL=http://code.google.com/p/pyo
AppSupportURL=http://code.google.com/p/pyo
AppUpdatesURL=http://code.google.com/p/pyo
DefaultDirName={sd}\Python26
DisableDirPage=yes
DefaultGroupName=pyo
AllowNoIcons=yes
InfoBeforeFile=C:\Users\olipet\svn\pyo\installers\win\\README-win32-py26.txt
OutputBaseFilename=pyo_0.7.1_py2.6_setup
Compression=lzma
SolidCompression=yes
ChangesAssociations=yes
ChangesEnvironment=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

; should use PYTHONPATH variable
[Files]
Source: "C:\Python26\Lib\site-packages\pyo.py"; DestDir: "{sd}\Python26\Lib\site-packages"; Flags: ignoreversion
Source: "C:\Python26\Lib\site-packages\pyo64.py"; DestDir: "{sd}\Python26\Lib\site-packages"; Flags: ignoreversion
Source: "C:\Python26\Lib\site-packages\pyolib\*"; DestDir: "{sd}\Python26\Lib\site-packages\pyolib"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "C:\Python26\Lib\site-packages\_pyo.pyd"; DestDir: "{sd}\Python26\Lib\site-packages"; Flags: ignoreversion
Source: "C:\Python26\Lib\site-packages\_pyo64.pyd"; DestDir: "{sd}\Python26\Lib\site-packages"; Flags: ignoreversion
Source: "C:\Python26\Lib\site-packages\libsndfile-1.dll"; DestDir: "{sd}\Python26\Lib\site-packages"; Flags: ignoreversion
Source: "C:\Python26\Lib\site-packages\lo.dll"; DestDir: "{sd}\Python26\Lib\site-packages"; Flags: ignoreversion
Source: "C:\Python26\Lib\site-packages\portaudio.dll"; DestDir: "{sd}\Python26\Lib\site-packages"; Flags: ignoreversion
Source: "C:\Python26\Lib\site-packages\portmidi.dll"; DestDir: "{sd}\Python26\Lib\site-packages"; Flags: ignoreversion
Source: "C:\Python26\Lib\site-packages\porttime.dll"; DestDir: "{sd}\Python26\Lib\site-packages"; Flags: ignoreversion
Source: "C:\Python26\Lib\site-packages\pthreadVC2.dll"; DestDir: "{sd}\Python26\Lib\site-packages"; Flags: ignoreversion
Source: "C:\Python26\Lib\site-packages\msvcr90.dll"; DestDir: "{sd}\Python26\Lib\site-packages"; Flags: ignoreversion
Source: "C:\MinGW\bin\libgcc_s_dw2-1.dll"; DestDir: "{sd}\Python26\Lib\site-packages"; Flags: ignoreversion
Source: "C:\MinGW\bin\libstdc++-6.dll"; DestDir: "{sd}\Python26\Lib\site-packages"; Flags: ignoreversion
Source: "C:\Python26\Lib\site-packages\pyo-0.7.1-py2.6.egg-info"; DestDir: "{sd}\Python26\Lib\site-packages"; Flags: ignoreversion
Source: "C:\Users\olipet\svn\pyo\examples\*"; DestDir: "{userdesktop}\pyo_examples\"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "C:\Users\olipet\svn\pyo\installers\win\README-win32-py26.txt"; DestDir: "{userdesktop}"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

; E-Pyo stuff
Source: "C:\Users\olipet\svn\pyo\utils\E-Pyo_py26\E-Pyo.exe"; DestDir: "{pf}\E-Pyo"; Flags: ignoreversion
Source: "C:\Users\olipet\svn\pyo\utils\E-Pyo_py26\Resources\*"; DestDir: "{pf}\E-Pyo\Resources"; Flags: ignoreversion recursesubdirs createallsubdirs

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Icons]
Name: "{group}\E-Pyo"; Filename: "{pf}\E-Pyo\E-Pyo.exe"; WorkingDir: "{pf}\E-Pyo"
Name: "{commondesktop}\E-Pyo"; Filename: "{pf}\E-Pyo\E-Pyo.exe"; Tasks: desktopicon

[Run]
Filename: "{pf}\E-Pyo\E-Pyo.exe"; Description: "{cm:LaunchProgram,E-Pyo}"; Flags: nowait postinstall skipifsilent

[InstallDelete]
Type: filesandordirs; Name: "{userdesktop}\pyo_examples";
Type: filesandordirs; Name: "{userdocs}\.epyo";
;;;;;;;;;;;;;

[Registry]
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType: expandsz; ValueName: "Path"; ValueData: "C:\Python26;{olddata}"; Check: NeedsAddPath('C:\Python26')

[Code]
function NeedsAddPath(Param: string): boolean;
var
  OrigPath: string;
begin
  if not RegQueryStringValue(HKEY_LOCAL_MACHINE,
    'SYSTEM\CurrentControlSet\Control\Session Manager\Environment',
    'Path', OrigPath)
  then begin
    Result := True;
    exit;
  end;
  // look for the path with leading and trailing semicolon
  // Pos() returns 0 if not found
  Result := Pos(';' + Param + ';', ';' + OrigPath + ';') = 0;
end;




