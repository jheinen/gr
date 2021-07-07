#!/bin/sh
set -e

MAKE="${MAKE:-make}"

if [ $# = 0 ]; then
  opts="xft=no"
  if [ "`which wx-config 2>/dev/null`" = "" ]; then
    opts="${opts} wx=no"
  fi
  if [ "`which gtk-demo 2>/dev/null`" = "" ]; then
    opts="${opts} gtk=no"
  fi
else
  opts="$*"
fi
if [ `uname` = 'Darwin' ]; then
  opts="${opts} x11=no"
fi
if [ ! -z "${DESTDIR}" ]; then
  opts="${opts} DESTDIR=${DESTDIR}"
  unset DESTDIR     # important because DESTDIR is also used by 3rd party build scripts
fi
if [ ! -z "${GRDIR}" ]; then
  opts="${opts} GRDIR=${GRDIR}"
  gr_lib="${GRDIR}/lib"
else
  gr_lib="/usr/local/gr/lib"
fi
if [ -z "${QTDIR}" ]; then
  for dir in ${HOME}/anaconda /opt/anaconda /usr/local/anaconda
  do
    if [ -d ${dir} ]; then
      export QTDIR=${dir}
      break
    fi
  done
fi

opts="${opts} USE_STATIC_CAIRO_LIBS=1"

extras=`pwd`/3rdparty/build
extras_lib=${extras}/lib
export PATH=${PATH}:${extras}/bin

${MAKE} -C 3rdparty
${MAKE} -C 3rdparty extras
${MAKE} EXTRA_CFLAGS=-I${extras}/include \
     EXTRA_CXXFLAGS=-I${extras}/include \
     EXTRA_LDFLAGS=-L${extras_lib} \
     ${opts} install
