REM ==================================================================================================================
REM ----------------------
REM Filename: acqu-ext.bat
REM ----------------------
REM
REM This windows batch script zips up your mod, renames it, and then copies all the required files for this mod into two destination directories.
REM All you need to do after you ran this script is start up your mod with '+set fs_game MODNAME'.
REM
REM Using this file assumes the following conditions are met on your system:
REM a) 7zip is installed and accessible via system path (http://7-zip.org/)
REM b) two destination directories exist, to which the files can be copied over (see WETDIR1, WETDIR2, MODNAME variable if you want to modify those)
REM c) the file is located in a folder where all the required files and directories for this mod are present. These (at this moment) include the following:
REM    - cgame_mp_x86.dll
REM    - qagame_mp_x86.dll
REM    - ui_mp_x86.dll
REM    - gfx
REM    - scripts
REM Usually, all the required files are already present in the .\src\Release directoy by default.
REM The current modname is simply 'z'. This, along with the path to your W:ET directories, is meant to be changed by you via script variables in this script.
REM Variables which should be changed: WETDIR1, WETDIR2, MODNAME
REM ==================================================================================================================
REM Starting script now...


REM variables
set WETDIR1=C:\Users\Derp\WET1
set WETDIR2=C:\Users\Derp\WET2
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
