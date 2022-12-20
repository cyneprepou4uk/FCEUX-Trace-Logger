# Fork description

A hacked Trace Logger to produce special log files for my disassembler.

## Download latest Windows 32 version

https://ci.appveyor.com/project/cyneprepou4uk/fceux/build/artifacts

## Changed files
* [appveyor.yml](https://github.com/cyneprepou4uk/fceux/blob/master/appveyor.yml)
* [src\asm.cpp](https://github.com/cyneprepou4uk/fceux/blob/master/src/asm.cpp)
* [src\asm.h](https://github.com/cyneprepou4uk/fceux/blob/master/src/asm.h)
* [src\debug.cpp](https://github.com/cyneprepou4uk/fceux/blob/master/src/debug.cpp)
* [src\debug.h](https://github.com/cyneprepou4uk/fceux/blob/master/src/debug.h)
* [src\drivers\win\tracer.cpp](https://github.com/cyneprepou4uk/fceux/blob/master/src/drivers/win/tracer.cpp)
* [src\drivers\win\tracer.h](https://github.com/cyneprepou4uk/fceux/blob/master/src/drivers/win/tracer.h)



# --- Original README ---

# fceux [![Build status](https://ci.appveyor.com/api/projects/status/github/TASEmulators/fceux?branch=master&svg=true)](https://ci.appveyor.com/project/zeromus/fceux)

An open source NES Emulator for Windows and Unix that features solid emulation accuracy and state of the art tools for power users. For some reason casual gamers use it too.

## Builds and Releases

Interim builds:
* Win32: [fceux.zip](https://ci.appveyor.com/api/projects/zeromus/fceux/artifacts/fceux.zip?branch=master&job=Windows%2032)
* Win64: [fceux64.zip](https://ci.appveyor.com/api/projects/zeromus/fceux/artifacts/fceux64.zip?branch=master&job=Windows%2064)
* Win64 Qt/SDL: [qfceux64.zip](https://ci.appveyor.com/api/projects/zeromus/fceux/artifacts/qfceux64.zip?branch=master&job=Win64%20Qt)
* Ubuntu: [fceux-2.6.4-amd64.deb](https://ci.appveyor.com/api/projects/zeromus/fceux/artifacts/fceux-2.6.4-amd64.deb?branch=master&job=Ubuntu)
* MacOSX: [fceux-2.6.4-Darwin.dmg](https://ci.appveyor.com/api/projects/zeromus/fceux/artifacts/fceux-2.6.4-Darwin.dmg?branch=master&job=MacOS)
* Status: [Appveyor](https://ci.appveyor.com/project/zeromus/fceux/)

But you might like mesen more: https://github.com/SourMesen/Mesen 

You should get releases from here: https://sourceforge.net/projects/fceultra/files/

That's because github forces us to use tags we don't have for releases.

2.6.4 is the most recent release but most people are using the autobuilds.
