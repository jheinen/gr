ifeq ($(strip $(THIRDPARTYDIR)),)
override THIRDPARTYDIR = $(abspath $(CURDIR)/../../3rdparty/build)
endif

       GRDIR = /usr/local/gr
      LIBDIR = $(DESTDIR)$(GRDIR)/lib
      INCDIR = $(DESTDIR)$(GRDIR)/include

UNAME := $(shell uname)

    INCLUDES = -I$(THIRDPARTYDIR)/include -I../gr
      GRLIBS = -L../gr -lGR
     PNGLIBS = $(THIRDPARTYDIR)/lib/libpng.a
    JPEGLIBS = $(THIRDPARTYDIR)/lib/libjpeg.a
       ZLIBS = $(THIRDPARTYDIR)/lib/libz.a
      CFLAGS = -O3 -Wall -Wextra -pedantic -fPIC -pthread -DGRDIR=\"$(GRDIR)\" $(EXTRA_CFLAGS)
        OBJS = gr3.o gr3_convenience.o gr3_html.o gr3_povray.o gr3_png.o \
               gr3_jpeg.o gr3_gr.o gr3_mc.o gr3_slices.o gr3_sr.o

ifeq ($(UNAME), Darwin)
GR_SHARED_LIBRARY_SUFFIX ?= .dylib
      CFLAGS+= -mmacosx-version-min=10.11
      GLLIBS = -framework OpenGL -framework Cocoa
        OBJS+= gr3_cgl.o
INSTALL_NAME = -Wl,-install_name,@rpath/libGR3${GR_SHARED_LIBRARY_SUFFIX}
     LDFLAGS = $(EXTRA_LDFLAGS) -Wl,-rpath,@loader_path/. -mmacosx-version-min=10.11
     SOFLAGS = -dynamiclib
        LIBS =
else
GR_SHARED_LIBRARY_SUFFIX ?= .so
      GLLIBS =
PLATFORMLIBS = -lX11 -lGL
        OBJS+= gr3_glx.o
INSTALL_NAME =
     LDFLAGS = $(EXTRA_LDFLAGS) -Wl,-rpath,'$$ORIGIN/.'
     SOFLAGS = -shared
        LIBS = -ldl
endif

default:
	$(MAKE) -C ../.. Makedefs
	$(MAKE) `cat ../../Makedefs`

all: targets

targets: prerequisites libGR3${GR_SHARED_LIBRARY_SUFFIX}

prerequisites:
	$(MAKE) -C ../gr

ifeq ($(UNAME), Linux)
targets: libGR3platform.so
install: install_platform
install_platform:
	@if [ ! -d $(DESTDIR)$(GRDIR) ]; then mkdir -m 755 $(DESTDIR)$(GRDIR); fi
	@if [ ! -d $(LIBDIR) ]; then mkdir -m 755 $(LIBDIR); fi
	cp -r libGR3platform.so $(LIBDIR)
libGR3platform.so: gr3_platform_glx.c
	${CC} -o $@ $^ $(CFLAGS) $(INCLUDES) $(PLATFORMLIBS) $(SOFLAGS) $(LDFLAGS) $(GLDEFS)
endif

.c.o:
	${CC} -c $< $(CFLAGS) $(INCLUDES) $(GLDEFS)

libGR3${GR_SHARED_LIBRARY_SUFFIX}: $(OBJS)
	${CC} -o $@ $^ $(CFLAGS) $(JPEGLIBS) $(GLLIBS) $(GRLIBS) $(PNGLIBS) $(ZLIBS) $(LIBS) \
	$(SOFLAGS) $(LDFLAGS) $(INSTALL_NAME)

gr3.c: gr3.h gr3_internals.h gr3_sr.h
gr3_internals.h: gr3.h gr3_glx.h gr3_cgl.h
gr3_glx.c: gr3_glx.h
gr3_cgl.c: gr3_cgl.h
gr3_convenience.c: gr3_internals.h
gr3_html.c: gr3_internals.h
gr3_povray.c: gr3_internals.h
gr3_png.c: gr3_internals.h
gr3_jpeg.c: gr3_internals.h
gr3_gr.c: gr3_internals.h gr3_sr.h
gr3_mc.c: gr3.h gr3_mc_data.h
gr3_slices.c: gr3.h
gr3_sr.c: gr3_sr.h

install: targets
	@if [ ! -d $(DESTDIR)$(GRDIR) ]; then mkdir -m 755 $(DESTDIR)$(GRDIR); fi
	@if [ ! -d $(LIBDIR) ]; then mkdir -m 755 $(LIBDIR); fi
	cp -p libGR3${GR_SHARED_LIBRARY_SUFFIX} $(LIBDIR)
	@if [ ! -d $(INCDIR) ]; then mkdir -m 755 $(INCDIR); fi
	cp -p gr3.h $(INCDIR)

clean:
	$(MAKE) -C ../gr clean
	rm -rf libGR3${GR_SHARED_LIBRARY_SUFFIX}*
	rm -rf libGR3platform.so*
	rm -f *.o
