OPTFLAGS=-O3 -fomit-frame-pointer -funroll-loops -Wall
#OPTFLAGS=-O3 -fomit-frame-pointer -funroll-loops -march=i686 -mcpu=i686
#OPTFLAGS=-xM -O3

CC=gcc
#CC=icc
CXX=g++
#CXX=icpc
NASM=nasm

ifeq ($(P),unix)
	CFLAGS= $(OPTFLAGS) $(shell sdl-config --cflags) -Imast -Idoze -D__cdecl=
else ifeq ($(P),win)
	CFLAGS= $(OPTFLAGS) -mno-cygwin -Imast -Idoze -Imaster -Iextra -Izlib
endif

CXXFLAGS= $(CFLAGS) -fno-exceptions

DOZEOBJ = doze/doze.o doze/dozea.o
DAMOBJ = doze/dam.o doze/dama.o doze/damc.o doze/dame.o doze/damf.o doze/damj.o doze/damm.o doze/damo.o doze/damt.o
MASTOBJ = mast/area.o mast/dpsg.o mast/draw.o mast/emu2413.o mast/frame.o mast/load.o mast/map.o mast/mast.o mast/mem.o mast/samp.o mast/snd.o mast/vgm.o mast/video.o mast/osd.o

ifeq ($(P),unix)
	PLATOBJ = sdl/main.o
else ifeq ($(P),win)
	PLATOBJ = master/app.o master/conf.o master/dinp.o master/disp.o master/dsound.o master/emu.o master/frame.o master/input.o master/load.o master/loop.o master/main.o master/misc.o master/render.o master/run.o master/shot.o master/state.o master/video.o master/zipfn.o master/keymap.o
endif

ifeq ($(P),unix)

all: dega degavi

dega: $(PLATOBJ) $(DOZEOBJ) $(MASTOBJ)
	$(CC) -o dega $(PLATOBJ) $(DOZEOBJ) $(MASTOBJ) $(shell sdl-config --libs)

degavi: tools/degavi.o $(DOZEOBJ) $(MASTOBJ)
	$(CC) -o degavi tools/degavi.o $(DOZEOBJ) $(MASTOBJ) -lm

doze/dozea.o: doze/dozea.asm
	nasm -f elf doze/dozea.asm

doze/dozea.asm: doze/dam
	cd doze ; ./dam
	sed -f doze/doze.cmd.sed <doze/dozea.asm >doze/dozea.asm.new
	mv doze/dozea.asm.new doze/dozea.asm

doze/dam: $(DAMOBJ)
	$(CC) -o doze/dam $(DAMOBJ)

else ifeq ($(P),win)

all: dega.exe

dega.exe: $(PLATOBJ) $(DOZEOBJ) $(MASTOBJ) zlib/libz.a
	$(CC) -mno-cygwin -Wl,--subsystem,windows -o dega.exe $(PLATOBJ) $(DOZEOBJ) $(MASTOBJ) -Lzlib -ldsound -ldinput -lddraw -ldxguid -lcomdlg32 -lcomctl32 -luser32 -lwinmm -lz

master/app.o: master/app.rc
	cd master && $(WINDRES) -o app.o app.rc

doze/dozea.o: doze/dozea.asm
	$(NASM) -f win32 -o doze/dozea.o doze/dozea.asm

doze/dozea.asm: doze/dam.exe
	cd doze ; $(WINE) ./dam

doze/dam.exe: $(DAMOBJ)
	$(CC) -mno-cygwin -o doze/dam.exe $(DAMOBJ)

zlib/libz.a:
	make -Czlib libz.a

else

all:
	@echo Please give a parameter P=unix or P=win
	@false

endif

clean:
	rm -f $(DOZEOBJ) $(DAMOBJ) $(MASTOBJ) $(PLATOBJ) tools/degavi.o doze/dozea.asm* doze/dam doze/dam.exe dega dega.exe degavi degavi.exe
	make -Czlib clean

distclean: clean
	rm -f *~ */*~
