# dmc5perfmod
Devil May Cry 5 graphic mods for low spec PCs
# Installing
Place MFReadWrite.dll and performance.ini in the root of the game folder (where DevilMayCry5.exe is located).
# Settings
Can be toggled in *performance.ini* file or at runtime, but be aware that disabling something, then enabling it back will not work until a level reload (restart from checkpoint for example).
CTRL + F1 - Toggle Light Probes.
CTRL + F2 - Toggle LOD overwrite.
CTRL + F3 - Toggle Shadows.
CTRL + F4 - Toggle AO.
CTRL + F5 - Toggle Cubemaps.
CTRL + F6 - Toggle Fog.
CTRL + F7 - Toggle IBL.
# Uninstalling
Remove MFReadWrite.dll, performance.ini
# Building
Can be build with either Intel Compiler or Clang 10.0 rc1 (at the time https://llvm.org/builds/). Run corresponding batfiles to generate VS solution and build.
