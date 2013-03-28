#!/bin/sh

# install.sh
# this installs the mod into one destination directory

WETDIR1='/home/linux/.etwolf/'
MODNAME='z'

strip -s *.so

zip -r temp cgame.mp.i386.so qagame.mp.i386.so ui.mp.i386.so gfx scripts sound

mv ./temp.zip $WETDIR1/$MODNAME/z.pk3
cp ./cgame.mp.i386.so $WETDIR1/$MODNAME/cgame.mp.i386.so
cp ./qagame.mp.i386.so $WETDIR1/$MODNAME/qagame.mp.i386.so
cp ./ui.mp.i386.so $WETDIR1/$MODNAME/ui.mp.i386.so

echo "Done!"
