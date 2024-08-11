symbosvm v1.0
a fast Z80 virtual machine for the SYmbiosis Multitasking Based Operating System

written by insane of Rabenauge^.tSCc.

usage: symbosvm [z80program.bin]

standard loaded program: symbosvm.bin

configure using symbosvm.ini

global keys:
  LCTRL+LALT+C: force quit
  LCTRL+LALT+ENTER: fullscreen

compiling:

linux:
  make -j

linux to windows 32bit:
  make CROSS=w32 -j

linux to windows 64bit:
  make CROSS=w64 -j

