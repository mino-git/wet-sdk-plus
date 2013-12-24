REM install.bat - this installs the mod into one destination directory

REM -- CONFIG START

set PATH_TO_WETEXE_1=C:\Users\Random\WETc\
set MODNAME=mymod
set COMPILED_WITH_LUA_SUPPORT=0
set COMPILED_WITH_XPSAVE_SUPPORT=0
set PATH_TO_LUA_LIB=..\add-ons\lua5.1\bin\lua5.1.dll
set PATH_TO_SQLITE3_LIB=..\add-ons\sqlite3\bin\sqlite3.dll

REM -- CONFIG END

7z.exe u -tzip z.zip cgame_mp_x86.dll ui_mp_x86.dll gfx scripts sound -r

rename z.zip z.pk3

copy z.pk3 %PATH_TO_WETEXE_1%\%MODNAME%
copy cgame_mp_x86.dll %PATH_TO_WETEXE_1%\%MODNAME%
copy qagame_mp_x86.dll %PATH_TO_WETEXE_1%\%MODNAME%
copy ui_mp_x86.dll %PATH_TO_WETEXE_1%\%MODNAME%

if %COMPILED_WITH_LUA_SUPPORT%==1 copy %PATH_TO_LUA_LIB% %PATH_TO_WETEXE_1%
if %COMPILED_WITH_XPSAVE_SUPPORT%==1 copy %PATH_TO_SQLITE3_LIB% %PATH_TO_WETEXE_1%

del z.pk3
