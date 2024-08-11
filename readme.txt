symbosvm v1.0
a fast Z80 virtual machine for the SYmbiosis Multitasking Based Operating System

written by insane of Rabenauge^.tSCc.

usage: symbosvm [z80program.bin]

look at the optional symbosvm.ini for further configuration

global keys:
  LCTRL+LALT+C: force quit
  LCTRL+LALT+ENTER: fullscreen

compiling:
- symbosvm needs SDL2

linux:
  make -j

linux to windows 32bit:
  make CROSS=w32 -j

linux to windows 64bit:
  make CROSS=w64 -j

