ifeq ($(strip $(PREFIX)),)
override PREFIX = $(abspath $(CURDIR)/../build)
endif

VERSION = 4.2.1
FFMPEG_EXTRA_CONFIGURE_FLAGS ?=
ifeq ($(shell uname),Darwin)
FFMPEG_EXTRA_CONFIGURE_FLAGS += --extra-cflags=-mmacosx-version-min=10.11
endif

ifeq ($(DOWNLOAD_CMD),)
ifneq ($(shell curl --version 2>/dev/null),)
DOWNLOAD_CMD := curl -k -OL
endif
endif
ifeq ($(DOWNLOAD_CMD),)
ifneq ($(shell wget --version 2>/dev/null),)
DOWNLOAD_CMD := wget --no-check-certificate
endif
endif
ifeq ($(DOWNLOAD_CMD),)
DOWNLOAD_CMD := echo "Error: Unable to find curl or wget."; exit 1; \#
endif

default: install

$(PREFIX)/src/ffmpeg-$(VERSION).tar.gz:
	mkdir -p $(PREFIX)/src
	cd $(PREFIX)/src/ && $(DOWNLOAD_CMD) https://gr-framework.org/downloads/3rdparty/ffmpeg-$(VERSION).tar.gz

$(PREFIX)/src/ffmpeg-$(VERSION)/configure: $(PREFIX)/src/ffmpeg-$(VERSION).tar.gz
	cd $(PREFIX)/src/ && tar -xf ffmpeg-$(VERSION).tar.gz
	sed 's/    \(bcrypt\|clock_gettime\)//' $(PREFIX)/src/ffmpeg-$(VERSION)/configure > $(PREFIX)/src/ffmpeg-$(VERSION)/configure.tmp
	mv $(PREFIX)/src/ffmpeg-$(VERSION)/configure.tmp $(PREFIX)/src/ffmpeg-$(VERSION)/configure
	cd $(PREFIX)/src/ffmpeg-$(VERSION)/libavformat && patch -p1 < $(CURDIR)/mov_pixeldensity.patch
	chmod +x $(PREFIX)/src/ffmpeg-$(VERSION)/configure

$(PREFIX)/src/ffmpeg-$(VERSION)/Makefile: $(PREFIX)/src/ffmpeg-$(VERSION)/configure $(PREFIX)/lib/libtheora.a $(PREFIX)/lib/libvpx.a $(PREFIX)/lib/libopenh264.a
	cd $(PREFIX)/src/ffmpeg-$(VERSION) && \
	PKG_CONFIG_PATH=$(PREFIX)/lib/pkgconfig ./configure \
		--prefix=$(PREFIX) \
		--disable-x86asm \
		--disable-asm \
		--enable-static \
		--disable-shared \
		--enable-pic \
		--enable-libvpx \
		--enable-libtheora \
		--enable-libopenh264 \
		--disable-iconv \
		--extra-cflags=-I$(PREFIX)/include \
		--extra-cxxflags=-I$(PREFIX)/include \
		--extra-ldflags=-L$(PREFIX)/lib \
		--disable-doc \
		--disable-vaapi \
		--disable-postproc \
		--disable-network \
		--disable-programs \
		--disable-encoders \
		--disable-decoders \
		--disable-muxers \
		--disable-demuxers \
		--disable-parsers \
		--disable-bsfs \
		--disable-protocols \
		--disable-devices \
		--disable-filters \
		--disable-vdpau \
		--disable-dxva2 \
		--enable-encoder=libvpx_vp8 \
		--enable-encoder=libtheora \
		--enable-encoder=libopenh264 \
		--enable-encoder=gif \
		--enable-muxer=webm \
		--enable-muxer=mov \
		--enable-muxer=mp4 \
		--enable-muxer=ogg \
		--enable-muxer=gif \
		--enable-protocol=file \
		$(FFMPEG_EXTRA_CONFIGURE_FLAGS)
	# configure doesn't update the Makefile mtime correctly
	touch $@

$(PREFIX)/lib/libavformat.a: $(PREFIX)/src/ffmpeg-$(VERSION)/Makefile
	$(MAKE) -C $(PREFIX)/src/ffmpeg-$(VERSION) -j4
	$(MAKE) -C $(PREFIX)/src/ffmpeg-$(VERSION) install

install: $(PREFIX)/lib/libavformat.a


$(PREFIX)/lib/libtheora.a:
	$(MAKE) -C ../theora PREFIX="$(PREFIX)" install

$(PREFIX)/lib/libvpx.a:
	$(MAKE) -C ../vpx PREFIX="$(PREFIX)" install

$(PREFIX)/lib/libopenh264.a:
	$(MAKE) -C ../openh264 PREFIX="$(PREFIX)" install

.PHONY: default install
