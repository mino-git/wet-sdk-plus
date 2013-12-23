#!/bin/sh

# install.sh
# this installs the mod into one destination directory

ET_HOMEPATH='/home/random/.etwolf/'
MODNAME='mymod'

strip -s *.so

zip -r temp cgame.mp.i386.so qagame.mp.i386.so ui.mp.i386.so gfx scripts sound

mv ./temp.zip $ET_HOMEPATH/$MODNAME/z.pk3
cp ./cgame.mp.i386.so $ET_HOMEPATH/$MODNAME/cgame.mp.i386.so
cp ./qagame.mp.i386.so $ET_HOMEPATH/$MODNAME/qagame.mp.i386.so
cp ./ui.mp.i386.so $ET_HOMEPATH/$MODNAME/ui.mp.i386.so

echo "Done!"
