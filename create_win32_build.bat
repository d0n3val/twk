:: Compiles and creates a build .zip file

if not defined INCLUDE (
call set_dirs.bat
) else (
call make.bat
)

mkdir twk
xcopy *.dll twk
xcopy twk.exe twk
xcopy CREDITS twk
xcopy /i data twk\data
xcopy /i data\textures twk\data\textures
del /s /q twk\data\*.bin
del /s /q twk\data\*.*~
7za.exe a -r twk_%date:~-4%_%date:~3,2%_%date:~0,2%-win32.zip twk
del /s /q twk
rmdir twk\data
rmdir twk
