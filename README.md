# REminiCRT - Flashback with CRT emulation shader

![Screenshot](https://raw.github.com/bni/REminiCRT/master/screenshot.png)
_From the intro_

## Credits
Original [REminiscence](http://cyxdown.free.fr/reminiscence/) author Gregory Montoir.

Delphine Software, makers of the 1992 game [Flashback](https://en.wikipedia.org/wiki/Flashback_(1992_video_game)).

## Added features
* CRT shader based on the one used in [TIC-80](https://github.com/nesbox/TIC-80)
* Fullscreen by default.
* Always preserving original aspect ratio.
* 4x scale in windowed mode.
* Mac app bundle.

## Removed features
* Internal and external scalers.
* Tremor/Vorbis support.
* ClassicMac asset support.
* Widescreen modes.

## Build
Get dependencies using macports or preferred way.

* SDL2
* [sdl-gpu](https://github.com/grimfang4/sdl-gpu)
* [libmodplug](http://modplug-xmms.sourceforge.net)
* [macdylibbundler](https://github.com/auriamg/macdylibbundler)

Modify the Makefile for your circumstances, then type:

`make`

To build Flashback.app (optional):

`make app`

## Assets/DATA
Requires DATA files from the original game, either DOS (Recommended) or Amiga (Not tested).
DOS files from GOG probably work. Filenames should be uppercase.

Get Amiga MOD music files from here: [UnExoticA](https://www.exotica.org.uk/wiki/Flashback).
Put files directly in the DATA/ folder.

## Run
1440p or 4K monitor preferred.

The Mac .app is standalone, with all dependencies inside the bundle.
