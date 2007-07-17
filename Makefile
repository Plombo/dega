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
MASTOBJ = mast/area.o mast/dpsg.o mast/draw.o mast/emu2413.o mast/frame.o mast/load.o mast/map.o mast/mast.o mast/mem.o mast/samp.o mast/snd.o mast/vgm.o mast/video.o mast/osd.o mast/md5.o
PYOBJ = python/pydega.o python/stdalone.o
PYEMBOBJ = python/pydega.emb.o

ifeq ($(P),unix)
	NASM_FORMAT = elf
	EXEEXT =
	SOEXT = .so
	PLATOBJ =
	PLATPYOBJ = sdl/main.o
	PLATPYOBJCXX =
	EXTRA_LIBS = $(shell sdl-config --libs)
	DOZE_FIXUP = sed -f doze/doze.cmd.sed <doze/dozea.asm >doze/dozea.asm.new && mv doze/dozea.asm.new doze/dozea.asm
	EXTRA_LDFLAGS =
	GUI_LDFLAGS =
	SPECS =
	PYTHON_CFLAGS = $(shell python-config --cflags) $(CFLAGS)
	PYTHON_CXXFLAGS = $(shell python-config --cflags) $(CXXFLAGS)
	PYTHON_LDFLAGS = $(shell python-config --ldflags) -lm
else ifeq ($(P),win)
	NASM_FORMAT = win32
	EXEEXT = .exe
	SOEXT = .dll
	PLATOBJ = master/app.o master/conf.o master/dinp.o master/disp.o master/dsound.o master/emu.o master/frame.o master/input.o master/load.o master/loop.o master/main.o master/misc.o master/render.o master/run.o master/shot.o master/state.o master/video.o master/zipfn.o master/keymap.o zlib/libz.a
	PLATPYOBJ =
	PLATPYOBJCXX = master/python.o
	EXTRA_LIBS = -ldsound -ldinput -lddraw -ldxguid -lcomdlg32 -lcomctl32 -luser32 -lwinmm
	DOZE_FIXUP =
	EXTRA_LDFLAGS = -specs=$(shell pwd)/specs -mno-cygwin
	GUI_LDFLAGS = -Wl,--subsystem,windows
	SPECS = specs
	PYTHON_PREFIX = /home/peter/pytest/winpython
	PYTHON_CFLAGS = -I$(PYTHON_PREFIX)/include $(CFLAGS)
	PYTHON_CXXFLAGS = -I$(PYTHON_PREFIX)/include $(CFLAGS)
	PYTHON_LDFLAGS = $(PYTHON_PREFIX)/libs/python24.dll
endif

ifeq ($(P),unix)

all: dega$(EXEEXT) degavi$(EXEEXT) mmvconv$(EXEEXT) pydega$(SOEXT)

release:
	darcs dist --dist-name dega-$(R)

else ifeq ($(P),win)

all: dega$(EXEEXT) mmvconv$(EXEEXT) pydega$(SOEXT)

zlib/libz.a:
	make -Czlib libz.a

release: all
	rm -rf dega-$(R)-win32
	mkdir dega-$(R)-win32
	cp dega.exe mmvconv.exe dega.txt python/scripts/*.py dega-$(R)-win32/
	$(STRIP) dega-$(R)-win32/dega.exe dega-$(R)-win32/mmvconv.exe
	cd dega-$(R)-win32 && zip -9 ../dega-$(R)-win32.zip dega.exe mmvconv.exe dega.txt *.py

else

all:
	@echo Please give a parameter P=unix or P=win
	@false

endif

dega$(EXEEXT): $(PLATOBJ) $(PLATPYOBJ) $(PLATPYOBJCXX) $(DOZEOBJ) $(MASTOBJ) $(PYEMBOBJ) $(SPECS)
	$(CC) $(EXTRA_LDFLAGS) $(GUI_LDFLAGS) -o dega$(EXEEXT) $(PLATOBJ) $(PLATPYOBJ) $(PLATPYOBJCXX) $(DOZEOBJ) $(MASTOBJ) $(PYEMBOBJ) $(EXTRA_LIBS)

degavi$(EXEEXT): tools/degavi.o $(DOZEOBJ) $(MASTOBJ)
	$(CC) $(EXTRA_LDFLAGS) -o degavi$(EXEEXT) tools/degavi.o $(DOZEOBJ) $(MASTOBJ) -lm

mmvconv$(EXEEXT): tools/mmvconv.o $(SPECS)
	$(CC) $(EXTRA_LDFLAGS) -o mmvconv$(EXEEXT) tools/mmvconv.o

pydega$(SOEXT): $(PYOBJ) $(DOZEOBJ) $(MASTOBJ) $(SPECS)
	$(CC) -shared -o pydega$(SOEXT) $(PYOBJ) $(DOZEOBJ) $(MASTOBJ) $(EXTRA_LDFLAGS) $(PYTHON_LDFLAGS)

doze/dozea.o: doze/dozea.asm
	nasm -f $(NASM_FORMAT) -o doze/dozea.o doze/dozea.asm

doze/dozea.asm: doze/dam$(EXEEXT)
	cd doze ; $(WINE) ./dam
	$(DOZE_FIXUP)

doze/dam$(EXEEXT): $(DAMOBJ)
	$(CC) -o doze/dam$(EXEEXT) $(DAMOBJ)

master/app.o: master/app.rc
	cd master && $(WINDRES) -o app.o app.rc

specs:
	sed -e 's/-lmsvcrt/-lmsvcr71/g' $(shell $(CC) -v 2>&1 | head -1 | cut -d' ' -f4) > specs

$(PYOBJ): %.o: %.c
	$(CC) -c -o $@ $< $(PYTHON_CFLAGS)

$(PLATPYOBJ): %.o: %.c
	$(CC) -c -o $@ $< -DEMBEDDED $(PYTHON_CFLAGS)

$(PLATPYOBJCXX): %.o: %.cpp
	$(CXX) -c -o $@ $< -DEMBEDDED $(PYTHON_CXXFLAGS)

%.emb.o: %.c
	$(CC) -c -o $@ $< -DEMBEDDED $(PYTHON_CFLAGS)

clean:
	rm -f $(DOZEOBJ) $(DAMOBJ) $(MASTOBJ) $(PLATOBJ) $(PYOBJ) $(PYEMBOBJ) $(PLATPYOBJ) $(PLATPYOBJCXX) tools/degavi.o tools/mmvconv.o doze/dozea.asm* doze/dam doze/dam.exe dega dega.exe degavi degavi.exe mmvconv mmvconv.exe specs
	make -Czlib clean

distclean: clean
	rm -f *~ */*~
