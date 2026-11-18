Railway Reservation System - Qt GUI

1. Back up your existing .txt data files first.
2. Copy railway_gui.cpp and railway_gui.pro into your RRC folder.
3. Keep your existing main.cpp. Do not compile main.cpp together with railway_gui.cpp because both contain main().
4. In PowerShell, from the RRC folder, run:

$env:PATH = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\mingw1310_64\libexec\gcc\x86_64-w64-mingw32\13.1.0;C:\Qt\6.12.0\mingw_64\bin;$env:PATH"
& "C:\Qt\6.12.0\mingw_64\bin\qmake.exe" railway_gui.pro
if ($LASTEXITCODE -eq 0) { & "C:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe" }

5. Run .\release\railway_gui.exe

The GUI uses the same text-file formats as the console program.
