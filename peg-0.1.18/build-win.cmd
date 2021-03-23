@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\vcvars32.bat"
msbuild peg.sln /p:Configuration=Release

xcopy /Y /D Release\*.exe .\
