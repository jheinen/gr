ifeq ($(strip $(PREFIX)),)
override PREFIX = $(abspath $(CURDIR)/../build)
endif

VERSION = 4.3.4
ZEROMQ_EXTRA_CONFIGURE_FLAGS ?=

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

$(PREFIX)/src/zeromq-$(VERSION).tar.gz:
	mkdir -p $(PREFIX)/src
	cd $(PREFIX)/src/ && $(DOWNLOAD_CMD) https://gr-framework.org/downloads/3rdparty/zeromq-$(VERSION).tar.gz

$(PREFIX)/src/zeromq-$(VERSION)/configure: $(PREFIX)/src/zeromq-$(VERSION).tar.gz
	cd $(PREFIX)/src/ && tar -xf zeromq-$(VERSION).tar.gz
	cd $(PREFIX)/src/zeromq-$(VERSION) && patch -p1 < $(CURDIR)/tests_includes.patch
	touch $@

$(PREFIX)/src/zeromq-$(VERSION)/Makefile: $(PREFIX)/src/zeromq-$(VERSION)/configure
	cd $(PREFIX)/src/zeromq-$(VERSION) && \
	./configure \
		--prefix=$(PREFIX) \
		--libdir=$(PREFIX)/lib \
		--enable-static \
		--disable-shared \
		--with-pic \
		--disable-dependency-tracking \
		--enable-curve=no \
		$(ZEROMQ_EXTRA_CONFIGURE_FLAGS)

$(PREFIX)/lib/libzmq.a: $(PREFIX)/src/zeromq-$(VERSION)/Makefile
	MACOSX_DEPLOYMENT_TARGET="10.11" $(MAKE) -C $(PREFIX)/src/zeromq-$(VERSION) -j4
	MACOSX_DEPLOYMENT_TARGET="10.11" $(MAKE) -C $(PREFIX)/src/zeromq-$(VERSION) install

install: $(PREFIX)/lib/libzmq.a

.PHONY: default install
