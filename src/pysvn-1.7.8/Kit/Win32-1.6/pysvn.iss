;
; -- bemacs.iss --
;

[Code]
function InitializeSetup(): Boolean;
var
    install_path : string;
    rcb : Boolean;
begin
    rcb := RegQueryStringValue( HKLM,
        'SOFTWARE\Python\PythonCore\%(py_maj)d.%(py_min)d\InstallPath',
        '', install_path );
    if not rcb then
    begin
        rcb := RegQueryStringValue( HKCU,
            'SOFTWARE\Python\PythonCore\%(py_maj)d.%(py_min)d\InstallPath', '', install_path );
        if not rcb then
        begin
            MsgBox( 'pysvn requires Python %(py_maj)d.%(py_min)d to be installed.' #13 #13
                    'Quitting installation',
                 mbError, MB_OK );
        end;
    end;
    Result := rcb;
end;

function pythondir(Default: String): String; 
var
    install_path : string;
    rcb : Boolean;
begin
    rcb := RegQueryStringValue( HKLM,
        'SOFTWARE\Python\PythonCore\%(py_maj)d.%(py_min)d\InstallPath',
        '', install_path );
    if rcb then
    begin
        Result := install_path;
    end
    else
    begin
        rcb := RegQueryStringValue( HKCU,
            'SOFTWARE\Python\PythonCore\%(py_maj)d.%(py_min)d\InstallPath',
            '', install_path );
        if rcb then
        begin
            Result := install_path;
        end
        else
        begin
            Result := 'c:\python%(py_maj)d.%(py_min)d';
        end;
    end;
end;


[Setup]
AppName=Python %(py_maj)d.%(py_min)d PySVN
AppVerName=Python %(py_maj)d.%(py_min)d PySVN %(pysvn_version_string)s 
AppCopyright=Copyright (C) 2003-2008 Barry A. Scott
DefaultDirName={code:pythondir}\lib\site-packages\pysvn
DefaultGroupName=PySVN for Python %(py_maj)d.%(py_min)d
DisableStartupPrompt=yes
InfoBeforeFile=info_before.txt
Compression=bzip/9

[Files]

Source: "..\..\..\Source\pysvn\__init__.py"; DestDir: "{app}";
Source: "..\..\..\Source\pysvn\_pysvn_%(py_maj)d_%(py_min)d.pyd"; DestDir: "{app}"; Flags: ignoreversion;
Source: "..\..\..\Docs\pysvn.html"; DestDir: "{app}";
Source: "..\..\..\Docs\pysvn_prog_guide.html"; DestDir: "{app}";
Source: "..\..\..\Docs\pysvn_prog_ref.html"; DestDir: "{app}";
Source: "..\..\..\Docs\pysvn_prog_ref.js"; DestDir: "{app}";
Source: "LICENSE.txt"; DestDir: "{app}";

Source: "..\..\..\Examples\Client\svn_cmd.py"; DestDir: "{app}\Examples\Client";
Source: "..\..\..\Examples\Client\parse_datetime.py"; DestDir: "{app}\Examples\Client";

#include "system_files.iss"

[Icons]
Name: "{group}\PySVN Documentation"; Filename: "{app}\pysvn.html";
Name: "{group}\PySVN License"; Filename: "{app}\pysvn_LICENSE.txt";
Name: "{group}\PySVN Web Site"; Filename: "http://pysvn.tigris.org";
