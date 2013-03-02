#!/bin/sh

# install-ext.sh
# this installs the mod into two destination directories

WETDIR1='/usr/local/games/enemy-territory/'
WETDIR2='/usr/local/games/enemy-territory2/'
MODNAME='z'

strip -s *.so

zip -r temp cgame.mp.i386.so qagame.mp.i386.so ui.mp.i386.so gfx scripts sound

cp ./temp.zip $WETDIR1/$MODNAME/z.pk3
cp ./cgame.mp.i386.so $WETDIR1/$MODNAME/cgame.mp.i386.so
cp ./qagame.mp.i386.so $WETDIR1/$MODNAME/qagame.mp.i386.so
cp ./ui.mp.i386.so $WETDIR1/$MODNAME/ui.mp.i386.so

mv ./temp.zip $WETDIR2/$MODNAME/z.pk3
cp ./cgame.mp.i386.so $WETDIR2/$MODNAME/cgame.mp.i386.so
cp ./qagame.mp.i386.so $WETDIR2/$MODNAME/qagame.mp.i386.so
cp ./ui.mp.i386.so $WETDIR2/$MODNAME/ui.mp.i386.so

echo "Done!"
