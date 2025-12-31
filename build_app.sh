#!/bin/sh
rm -f Flashback.app/Contents/MacOS/rs
cp cmake-build-debug/rs Flashback.app/Contents/MacOS/
cp rs.cfg Flashback.app/Contents/Resources/
cp vertex.shader Flashback.app/Contents/Resources/
cp pixel.shader Flashback.app/Contents/Resources/
cp -r DATA Flashback.app/Contents/Resources/
../macdylibbundler/dylibbundler -od -b -x Flashback.app/Contents/MacOS/rs -d ./Flashback.app/Contents/libs/
otool -L Flashback.app/Contents/MacOS/rs
