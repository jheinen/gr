ifeq ($(strip $(THIRDPARTYDIR)),)
override THIRDPARTYDIR = $(abspath $(CURDIR)/../../../3rdparty/build)
endif

   UNAME := $(shell uname)

      GRDIR = /usr/local/gr
     LIBDIR = $(DESTDIR)$(GRDIR)/lib

     CFLAGS = -O -Wall -fPIC $(EXTRA_CFLAGS)
   CXXFLAGS = -O -Wall -fPIC $(EXTRA_CXXFLAGS)
   INCLUDES = -I.. -I$(THIRDPARTYDIR)/include
    SOFLAGS = -shared
    LDFLAGS = $(EXTRA_LDFLAGS)
    ASAN_FLAGS = #-fsanitize=address
    ASAN_LIBS  = #-shared-libasan -fsanitize=address
ifeq ($(UNAME),Darwin)
	 CFLAGS += -mmacosx-version-min=10.11
   CXXFLAGS += -mmacosx-version-min=10.11
	 LDFLAGS += -mmacosx-version-min=10.11
endif
  WX_CONFIG = false
     WXDEFS =
      WXINC = `$(WX_CONFIG) --cxxflags 2>/dev/null`
     WXLIBS = `$(WX_CONFIG) --libs 2>/dev/null`
ifneq ($(QT4_QMAKE),false)
     QT4DEFS =
   QT4INCDIR = `$(QT4_QMAKE) -query QT_INSTALL_HEADERS`
      QT4INC = -I$(QT4INCDIR) -I$(QT4INCDIR)/QtCore -I$(QT4INCDIR)/QtGui
   QT4LIBDIR = `$(QT4_QMAKE) -query QT_INSTALL_LIBS`
     QT4LIBS = -L$(QT4LIBDIR) -lQtGui -lQtCore
ifeq ($(UNAME),Darwin)
ifneq ($(QT4INC),)
     QT4INC += -mmacosx-version-min=10.11
endif
else
	EXTRA_LDFLAGS_QT4="-Wl,-rpath=$(QT4LIBDIR)"
endif
endif
ifneq ($(QT5_QMAKE),false)
     QT5DEFS =
   QT5INCDIR = `$(QT5_QMAKE) -query QT_INSTALL_HEADERS`
ifeq ($(UNAME),Darwin)
      QT5INC = -I$(QT5INCDIR) -I$(QT5LIBDIR)/QtCore.framework/Versions/5/Headers -I$(QT5LIBDIR)/QtGui.framework/Versions/5/Headers -I$(QT5LIBDIR)/QtWidgets.framework/Versions/5/Headers -F$(QT5LIBDIR)
else
      QT5INC = -I$(QT5INCDIR) -I$(QT5INCDIR)/QtCore -I$(QT5INCDIR)/QtGui -I$(QT5INCDIR)/QtWidgets
endif
   QT5INC += -std=c++11
   QT5LIBDIR = `$(QT5_QMAKE) -query QT_INSTALL_LIBS`
ifeq ($(UNAME),Darwin)
     QT5LIBS = -F$(QT5LIBDIR) -framework QtWidgets -framework QtGui -framework QtCore
else
     QT5LIBS = -L$(QT5LIBDIR) -lQt5Widgets -lQt5Gui -lQt5Core
	EXTRA_LDFLAGS_QT5="-Wl,-rpath=$(QT5LIBDIR)"
endif
endif
 GTK_CONFIG = false
    GTKDEFS =
     GTKINC = `$(GTK_CONFIG) gtk+-2.0 --cflags 2>/dev/null`
    GTKLIBS = `$(GTK_CONFIG) gtk+-2.0 --libs 2>/dev/null`
   JPEGLIBS = $(THIRDPARTYDIR)/lib/libjpeg.a
    PNGLIBS = $(THIRDPARTYDIR)/lib/libpng.a
    BZ2LIBS = $(THIRDPARTYDIR)/lib/libbz2.a
     GSDEFS =
      GSINC =
     GSLIBS = -lgs
   GLFWDEFS =
    GLFWINC =
    GLFWLIB = glfw
ifeq ($(UNAME),Darwin)
   GLFWLIBS = -l$(GLFWLIB) -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
else
   GLFWLIBS = -l$(GLFWLIB) -lGL
endif
    XFTDEFS =
     FTDEFS =
     FTLIBS = $(THIRDPARTYDIR)/lib/libfreetype.a
     AVDEFS =
     AVLIBS = -lavformat -lavcodec -lswscale -lavutil $(BZ2LIBS)
  CAIRODEFS =
ifdef USE_STATIC_CAIRO_LIBS
  CAIROLIBS = $(THIRDPARTYDIR)/lib/libcairo.a $(THIRDPARTYDIR)/lib/libpixman-1.a
else
  CAIROLIBS = -lcairo -lpixman-1 -lpthread
endif
    SSLLIBS = -lssl -lcrypto
    X11DEFS =
ifeq ($(UNAME), Darwin)
     X11INC = -I/opt/X11/include
else
     X11INC = -I/usr/X11R6/include
endif
    XFTLIBS = -lXft -lfontconfig
    X11LIBS = $(X11PATH) $(XFTLIBS) -lXt -lX11
    ZMQDEFS =
    ZMQLIBS = -L$(THIRDPARTYDIR)/lib/ -lzmq -lpthread
ifeq ($(UNAME), Darwin)
  EXTRALIBS = -framework VideoDecodeAcceleration -framework VideoToolbox -framework CoreVideo -framework CoreFoundation -framework CoreServices -framework CoreMedia
       LIBS = -liconv
else
  EXTRALIBS =
       LIBS = -lc -lm
endif
      ZLIBS = $(THIRDPARTYDIR)/lib/libz.a
   TIFFLIBS = -ltiff
    GKSLIBS = ../libGKS.a

.SUFFIXES: .o .m .cxx

.m.o:
	$(CXX) -c -DGRDIR=\"$(GRDIR)\" $(DEFINES) $(INCLUDES) $(CXXFLAGS) $<
.c.o:
	$(CC) -c -DGRDIR=\"$(GRDIR)\" $(DEFINES) $(CFLAGS) $(INCLUDES) $(GTKDEFS) $(GTKINC) $(GSDEFS) $(GSINC) $(GLFWDEFS) $(GLFWINC) $(ZMQDEFS) $(XFTDEFS) $(FTDEFS) $(FTINC) $(AVDEFS) $(CAIRODEFS) $(X11DEFS) $(X11INC) $(ZINC) $<
.cxx.o:
	$(CXX) -c -DGRDIR=\"$(GRDIR)\" $(DEFINES) $(CXXFLAGS) $(INCLUDES) $(WXDEFS) $(WXINC) $(QT4DEFS) $(QT4INC) $(GTKDEFS) $(GTKINC) $(GSDEFS) $(GSINC) $(GLFWDEFS) $(GLFWINC) $(ZMQDEFS) $(XFTDEFS) $(FTDEFS) $(FTINC) $(AVDEFS) $(CAIRODEFS) $(X11DEFS) $(X11INC) $(ZINC) $<

default:
	$(MAKE) -C ../../.. Makedefs
	$(MAKE) `cat ../../../Makedefs`

all: targets

targets: prerequisites \
         x11plugin.so wxplugin.so qtplugin.so qt5plugin.so svgplugin.so \
         gtkplugin.so gsplugin.so wmfplugin.so glplugin.so \
         zmqplugin.so pgfplugin.so cairoplugin.so videoplugin.so

ifeq ($(UNAME),Darwin)
targets: quartzplugin.so
endif

prerequisites:
	$(MAKE) -C ../../../3rdparty PREFIX=$(THIRDPARTYDIR)

$(GKSLIBS):
	$(MAKE) -C .. libGKS.a

qtplugin.o: qtplugin.cxx
	$(CXX) -o $@ -c -DGRDIR=\"$(GRDIR)\" $(DEFINES) $(WXDEFS) $(WXINC) $(QT4DEFS) $(QT4INC) $(GTKDEFS) $(GTKINC) $(GSDEFS) $(GSINC) $(GLFWDEFS) $(GLFWINC) $(ZMQDEFS) $(INCLUDES) $(XFTDEFS) $(FTDEFS) $(FTINC) $(AVDEFS) $(CAIRODEFS) $(X11DEFS) $(X11INC) $(CXXFLAGS) $(ZINC) $<

qt5plugin.o: qt5plugin.cxx
	$(CXX) -o $@ -c -DGRDIR=\"$(GRDIR)\" $(DEFINES) $(WXDEFS) $(WXINC) $(QT5DEFS) $(QT5INC) $(GTKDEFS) $(GTKINC) $(GSDEFS) $(GSINC) $(GLFWDEFS) $(GLFWINC) $(ZMQDEFS) $(INCLUDES) $(XFTDEFS) $(FTDEFS) $(FTINC) $(AVDEFS) $(CAIRODEFS) $(X11DEFS) $(X11INC) $(CXXFLAGS) $(ZINC) $<

x11plugin.so: x11plugin.o $(GKSLIBS)
	$(CC) -o $@ $(SOFLAGS) $(LDFLAGS) $^ $(FTLIBS) $(ZLIBS) $(X11LIBS) $(LIBS)

wxplugin.so: wxplugin.o $(GKSLIBS)
	$(CXX) -o $@ $(SOFLAGS) $(LDFLAGS) $^ $(WXLIBS) $(X11LIBS) $(LIBS)

qtplugin.so: qtplugin.o $(GKSLIBS)
	$(CXX) -o $@ $(SOFLAGS) $(LDFLAGS) $^ $(EXTRA_LDFLAGS_QT4) $(QT4LIBS) $(X11LIBS) $(LIBS)

qt5plugin.so: qt5plugin.o $(GKSLIBS)
	$(CXX) -o $@ $(SOFLAGS) $(LDFLAGS) $^ $(EXTRA_LDFLAGS_QT5) $(QT5LIBS) $(X11LIBS) $(LIBS)

svgplugin.so: svgplugin.o $(GKSLIBS) $(PNGLIBS)
	$(CC) -o $@ $(SOFLAGS) $(LDFLAGS) $^ $(PNGLIBS) $(ZLIBS) $(LIBS)

gtkplugin.so: gtkplugin.o $(GKSLIBS)
	$(CC) -o $@ $(SOFLAGS) $(LDFLAGS) $^ $(GTKLIBS) $(X11LIBS) $(LIBS)

gsplugin.so: gsplugin.o $(GKSLIBS)
	$(CC) -o $@ $(SOFLAGS) $(LDFLAGS) $^ $(GSLIBS) $(X11LIBS) $(LIBS)

wmfplugin.so: wmfplugin.o $(GKSLIBS)
	$(CC) -o $@ $(SOFLAGS) $(LDFLAGS) $^ $(LIBS)

#quartzplugin.so: quartzplugin.o networkbib/libconnection.a $(GKSLIBS) 
#	$(CXX) -o $@ $(SOFLAGS) $(ASAN_FLAGS) $^ $(LIBS) $(ASAN_LIBS) $(ZMQLIBS) \
#	-lobjc -framework Foundation -framework ApplicationServices -framework AppKit
quartzplugin.so: quartzplugin.o networkbib/libconnection.a $(GKSLIBS) 
	$(CXX) -o $@ $(SOFLAGS) $^ $(LIBS) $(ZMQLIBS) \
	-lobjc -framework Foundation -framework ApplicationServices -framework AppKit
glplugin.so: glplugin.o $(GKSLIBS)
	$(CC) -o $@ $(SOFLAGS) $(LDFLAGS) $^ $(GLFWLIBS) $(FTLIBS) $(ZLIBS) $(X11LIBS) $(LIBS)

zmqplugin.so: zmqplugin.o $(GKSLIBS)
	$(CXX) -o $@ $(SOFLAGS) $(LDFLAGS) $^ $(ZMQLIBS) $(LIBS)

pgfplugin.so: pgfplugin.o $(GKSLIBS) $(PNGLIBS)
	$(CC) -o $@ $(SOFLAGS) $(LDFLAGS) $^ $(PNGLIBS) $(ZLIBS) $(LIBS)

cairoplugin.o: cairoplugin.c
	$(CC) -c -DGRDIR=\"$(GRDIR)\" $(DEFINES) $(INCLUDES) $(FTDEFS) $(FTINC) $(JPEGDEFS) $(CAIRODEFS) $(TIFFDEFS) $(CFLAGS) $(ZINC) -DNO_X11 $<

cairoplugin.so: cairoplugin.o $(GKSLIBS) $(PNGLIBS)
	$(CC) -o $@ $(SOFLAGS) $(LDFLAGS) $^ $(CAIROLIBS) $(FTLIBS) $(JPEGLIBS) $(PNGLIBS) $(TIFFLIBS) $(ZLIBS) $(LIBS) -DNO_X11

videoplugin.so: videoplugin.o vc.o gif.o $(GKSLIBS)
	$(CXX) -o $@ $(SOFLAGS) $(LDFLAGS) $^ $(AVLIBS) $(FTLIBS) $(EXTRALIBS) $(ZLIBS) $(LIBS)

install: default $(TARGETS)
	cp -p *.so $(LIBDIR)

clean:
	rm -f so_locations *.so *.o *.bak *~ *.tmp

qtplugin.o: qtplugin_impl.cxx
qt5plugin.o: qtplugin_impl.cxx

.PHONY: clean install default gks
