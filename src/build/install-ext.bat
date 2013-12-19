REM install-ext.bat - this installs the mod into two destination directories

REM -- CONFIG START

set WETDIR1=C:\Users\Random\WETc\
set WETDIR2=C:\Users\Random\WETs\
set MODNAME=mymod

REM -- CONFIG END

7z.exe u -tzip z.zip cgame_mp_x86.dll ui_mp_x86.dll gfx scripts sound -r

rename z.zip z.pk3

copy z.pk3 %WETDIR1%\%MODNAME%
copy cgame_mp_x86.dll %WETDIR1%\%MODNAME%
copy qagame_mp_x86.dll %WETDIR1%\%MODNAME%
copy ui_mp_x86.dll %WETDIR1%\%MODNAME%

copy z.pk3 %WETDIR2%\%MODNAME%
copy cgame_mp_x86.dll %WETDIR2%\%MODNAME%
copy qagame_mp_x86.dll %WETDIR2%\%MODNAME%
copy ui_mp_x86.dll %WETDIR2%\%MODNAME%

del z.pk3
