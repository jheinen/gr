ifeq ($(strip $(THIRDPARTYDIR)),)
override THIRDPARTYDIR = $(abspath $(CURDIR)/../../3rdparty/build)
endif

       GRDIR = /usr/local/gr
      BINDIR = $(DESTDIR)$(GRDIR)/bin
      LIBDIR = $(DESTDIR)$(GRDIR)/lib
      INCDIR = $(DESTDIR)$(GRDIR)/include

UNAME := $(shell uname)

      GROBJS = gr.o text.o contour.o spline.o gridit.o strlib.o io.o image.o \
               delaunay.o interp2.o md5.o import.o shade.o grforbnd.o \
               contourf.o boundary.o mathtex2.o mathtex2.tab.o threadpool.o
      GSDEFS =
     DEFINES = $(GSDEFS)
    INCLUDES = -I../gks -I$(THIRDPARTYDIR)/include
          CC = cc
      CFLAGS = $(DEFINES) -O3 -Wall -fPIC $(EXTRA_CFLAGS)
          AR = ar
      RANLIB = ar ts
ifeq ($(UNAME), Darwin)
GR_SHARED_LIBRARY_SUFFIX ?= .dylib
	 CFLAGS += -mmacosx-version-min=10.11
INSTALL_NAME = -Wl,-install_name,@rpath/libGR${GR_SHARED_LIBRARY_SUFFIX}
     LDFLAGS = $(EXTRA_LDFLAGS) -Wl,-rpath,@loader_path/. -mmacosx-version-min=10.11
     SOFLAGS = -dynamiclib
else
GR_SHARED_LIBRARY_SUFFIX ?= .so
INSTALL_NAME =
     LDFLAGS = $(EXTRA_LDFLAGS) -Wl,-rpath,'$$ORIGIN/.'
     SOFLAGS = -shared
endif
        LIBS = -lpthread -ldl -lc -lm
    JPEGLIBS = $(THIRDPARTYDIR)/lib/libjpeg.a
     BZ2LIBS = $(THIRDPARTYDIR)/lib/libbz2.a
     PNGLIBS = $(THIRDPARTYDIR)/lib/libpng.a
     GKSLIBS = ../gks/libGKS.a
      FTLIBS = $(THIRDPARTYDIR)/lib/libfreetype.a
       ZLIBS = $(THIRDPARTYDIR)/lib/libz.a
      QHLIBS = $(THIRDPARTYDIR)/lib/libqhull.a

# Only update gr_version.h if it will result in an actual change
GR_VERSION_H_GENERATED := $(shell ../../lib/Version)
GR_VERSION_H_CURRENT := $(shell cat gr_version.h 2>/dev/null || echo "")
ifneq ($(GR_VERSION_H_CURRENT),$(GR_VERSION_H_GENERATED))
GR_VERSION_H_DEPS := .FORCE
else
GR_VERSION_H_DEPS :=
endif

.SUFFIXES:
.SUFFIXES: .c .o

.c.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $<

default:
	$(MAKE) -C ../.. Makedefs
	$(MAKE) `cat ../../Makedefs`

all: targets

targets: prerequisites libGR.a libGR${GR_SHARED_LIBRARY_SUFFIX} demo libqtgr

prerequisites:
	$(MAKE) -C ../../3rdparty/ PREFIX=$(THIRDPARTYDIR)

../gks/libGKS.a:
	$(MAKE) -C ../gks libGKS.a

libGR.a: $(GROBJS)
	$(AR) crv $@ $?
	$(RANLIB) $@

libGR${GR_SHARED_LIBRARY_SUFFIX}: $(GROBJS) $(GKSLIBS)
	$(CC) -o $@ $(SOFLAGS) $(GROBJS) $(PNGLIBS) $(GKSLIBS) \
	$(FTLIBS) $(JPEGLIBS) $(ZLIBS) $(QHLIBS) $(LIBS) \
	$(LDFLAGS) $(INSTALL_NAME)

demo: demo.c libGR.a $(GKSLIBS)
	$(CC) -o demo $(INCLUDES) $(CFLAGS) demo.c libGR.a \
	$(PNGLIBS) $(GKSLIBS) $(FTLIBS) $(JPEGLIBS) \
	$(ZLIBS) $(QHLIBS) $(LIBS) $(LDFLAGS)

libqtgr:
	$(MAKE) -C qtgr

gr_version.h: $(GR_VERSION_H_DEPS)
	../../lib/Version > $@
	@chmod 644 $@

gr.o: gr_version.h

install:
	$(MAKE) -C ../gks install
	@if [ ! -d $(DESTDIR)$(GRDIR) ]; then mkdir -m 755 $(DESTDIR)$(GRDIR); fi
	@if [ ! -d $(BINDIR) ]; then mkdir -m 755 $(BINDIR); fi
	@if [ ! -d $(LIBDIR) ]; then mkdir -m 755 $(LIBDIR); fi
	cp -p libGR.* $(LIBDIR)
	@if [ ! -d $(INCDIR) ]; then mkdir -m 755 $(INCDIR); fi
	cp -p gr.h $(INCDIR)
	$(MAKE) -C qtgr install

clean:
	$(MAKE) -C qtgr clean
	$(MAKE) -C ../gks clean
	$(MAKE) -C ../gks/plugin clean
	$(MAKE) -C ../../3rdparty/ clean
	rm -f so_locations demo *.so *.a *.o *.bak *~ gr_version.h *${GR_SHARED_LIBRARY_SUFFIX}

depend:
	makedepend -Y -- gr.c text.c contour.c spline.c gridit.c strlib.c io.c \
	image.c delaunay.c interp2.c md5.c import.c shade.c grforbnd.c \
    threadpool.c    2> /dev/null

.FORCE:

.PHONY: all targets install clean depend prerequisites .FORCE

# DO NOT DELETE THIS LINE -- make depend depends on it.

gr.o: gr.h text.h spline.h gridit.h contour.h strlib.h io.h md5.h cm.h
contour.o: gr.h contour.h
contourf.o: gr.h contourf.h
spline.o: spline.h
gridit.o: gridit.h
strlib.o: strlib.h
io.o: gr.h io.h
image.o: gr.h
delaunay.o: gr.h
interp2.o: gr.h
md5.o: md5.h
import.o: gr.h
shade.o: gr.h
grforbnd.o: gr.h
boundary.o: boundary.h
mathtex2.o: mathtex2.h tempbuffer.inl
mathtex2.tab.o: mathtex2.h
threadpool.o: threadpool.h
