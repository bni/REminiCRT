#!/bin/sh
rm -f Flashback.app/Contents/MacOS/rs
cp build/rs Flashback.app/Contents/MacOS/
cp build/rs.cfg Flashback.app/Contents/Resources/
cp build/vertex.shader Flashback.app/Contents/Resources/
cp build/pixel.shader Flashback.app/Contents/Resources/
cp -r build/DATA Flashback.app/Contents/Resources/
../macdylibbundler/dylibbundler -od -b -x Flashback.app/Contents/MacOS/rs -d ./Flashback.app/Contents/libs/
otool -L Flashback.app/Contents/MacOS/rs
