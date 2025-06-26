# CrystalClock
Re-creation of PS2's display clock in Raylib
# Controls
For Desktop: 'k' to show/hide time, 'j' to enable/disable 'orbs only' mode.\
For Mobile: 'Swipe up' to show/hide time, 'Swipe down' to enable/disable 'orbs only' mode
# Build
Install and configure CMake and then run the following from the root directory of the project:
``` bash
cmake -B build
cmake --build build --config Release
```
Binary will appear in bin/Release directory. Copy resources/ directory there or create a symlink:
## POSIX
```
ln -s ../../resources resources 
```
## Windows
```
mklink resources ../../resources
```
# Usage
Specify width and height of the window:
```
crystalclock.exe -width 1920 -height 1080
```
Specify which display the program should appear on:
```
crystalclock.exe -width 1920 -height 1080 -display 1
```
Shorter form:
```
crystalclock.exe -w 1920 -h 1080 -d 1
```
Additional flags:
```
-fullscreen -borderless -undecorated -nosound -nofadein
```
Parameters can also be specified in `config.ini` file, place it into resources/
```
# CrystalClock configuration file
# NOTE: Do not put comments on the same line with properties: width = 1920 # window width
# Boolean properties can also accept integer as value: fullscreen = 1, fullscreen = true
# Properties with invalid values will be ignored
[window]
width  = 1920
height = 1080

fullscreen  = 0
undecorated = 0
borderless  = 0
nofadein    = 0
nosound     = 0
```
# Screenshots
![Clock1](/images/clock1.png "Clock preview")
![Clock2](/images/clock2.png "Clock preview")
![Clock3](/images/clock3.png "Clock preview")
![Clock4](/images/clock4.jpg "Clock preview")