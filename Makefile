OPTFLAGS=-O3 -fomit-frame-pointer -funroll-loops
#OPTFLAGS=-O3 -fomit-frame-pointer -funroll-loops -march=i686 -mcpu=i686
#OPTFLAGS=-xM -O3

CC=gcc
#CC=icc
CXX=g++
#CXX=icpc

CFLAGS= $(OPTFLAGS) $(shell sdl-config --cflags) -Imast -Idoze -D__cdecl=
CXXFLAGS= $(CFLAGS) -fno-exceptions

DOZEOBJ = doze/doze.o doze/dozea.o
DAMOBJ = doze/dam.o doze/dama.o doze/damc.o doze/dame.o doze/damf.o doze/damj.o doze/damm.o doze/damo.o doze/damt.o
MASTOBJ = mast/area.o mast/dpsg.o mast/draw.o mast/emu2413.o mast/frame.o mast/load.o mast/map.o mast/mast.o mast/mem.o mast/samp.o mast/snd.o mast/vgm.o
SDLOBJ = sdl/main.o

all: dega

dega: $(SDLOBJ) $(DOZEOBJ) $(MASTOBJ)
	$(CC) -o dega sdl/main.o $(DOZEOBJ) $(MASTOBJ) $(shell sdl-config --libs)

doze/dozea.o: doze/dozea.asm
	nasm -f elf doze/dozea.asm

doze/dozea.asm: doze/dam
	cd doze ; ./dam
	sed -f doze/doze.cmd.sed <doze/dozea.asm >doze/dozea.asm.new
	mv doze/dozea.asm.new doze/dozea.asm

doze/dam: $(DAMOBJ)
	$(CC) -o doze/dam $(DAMOBJ)

clean:
	rm -f $(DOZEOBJ) $(DAMOBJ) $(MASTOBJ) $(SDLOBJ) doze/dozea.asm* doze/dam dega

distclean: clean
	rm -f *~ */*~
