REM install-ext.bat - this installs the mod into two destination directories

set WETDIR1=C:\Users\herb28\WETc
set WETDIR2=C:\Users\herb28\WETs
set MODNAME=z

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
