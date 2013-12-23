#!/bin/sh

# install-ext.sh
# this installs the mod into two destination directories

ET_HOMEPATH1='/home/random/.etwolf/'
ET_HOMEPATH2='/home/random/.etwolf2/'
MODNAME='mymod'

strip -s *.so

zip -r temp cgame.mp.i386.so qagame.mp.i386.so ui.mp.i386.so gfx scripts sound

cp ./temp.zip $ET_HOMEPATH1/$MODNAME/z.pk3
cp ./cgame.mp.i386.so $ET_HOMEPATH1/$MODNAME/cgame.mp.i386.so
cp ./qagame.mp.i386.so $ET_HOMEPATH1/$MODNAME/qagame.mp.i386.so
cp ./ui.mp.i386.so $ET_HOMEPATH1/$MODNAME/ui.mp.i386.so

mv ./temp.zip $ET_HOMEPATH2/$MODNAME/z.pk3
cp ./cgame.mp.i386.so $ET_HOMEPATH2/$MODNAME/cgame.mp.i386.so
cp ./qagame.mp.i386.so $ET_HOMEPATH2/$MODNAME/qagame.mp.i386.so
cp ./ui.mp.i386.so $ET_HOMEPATH2/$MODNAME/ui.mp.i386.so

echo "Done!"
