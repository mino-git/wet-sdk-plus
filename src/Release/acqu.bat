REM ==================================================================================================================
REM ----------------------
REM Filename: acqu.bat
REM ----------------------
REM
REM This windows batch script zips up your mod, renames it, and then copies all the required files for this mod into one destination directory.
REM All you need to do after you ran this script is start up your mod with '+set fs_game MODNAME'.
REM
REM Using this file assumes the following conditions are met on your system:
REM a) 7zip is installed and accessible via system path (http://7-zip.org/)
REM b) one destination directory exists, to which the files can be copied over (see WETDIR1, MODNAME variable if you want to do modification)
REM c) the file is located in a folder where all the required files and directories for this mod are present. These (at this moment) include the following:
REM    - cgame_mp_x86.dll
REM    - qagame_mp_x86.dll
REM    - ui_mp_x86.dll
REM    - gfx
REM    - scripts
REM Usually, all the required files are already present in the .\src\Release directoy by default.
REM The current modname is simply 'z'. This, along with the path to your W:ET directory, is meant to be changed by you via script variables in this script.
REM Variables which should be changed: WETDIR1, MODNAME
REM ==================================================================================================================
REM Starting script now...


REM variables
set WETDIR1=C:\Users\Derp\WET1
set MODNAME=z

7z.exe u -tzip z.zip cgame_mp_x86.dll ui_mp_x86.dll gfx scripts sound -r

rename z.zip z.pk3

copy z.pk3 %WETDIR1%\%MODNAME%
copy cgame_mp_x86.dll %WETDIR1%\%MODNAME%
copy qagame_mp_x86.dll %WETDIR1%\%MODNAME%
copy ui_mp_x86.dll %WETDIR1%\%MODNAME%

del z.pk3
