#***************************************************************************
# Group VDR/GraphTFTng
# File Makefile
# Date 31.10.06
# This code is distributed under the terms and conditions of the
# GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
# (c) 2006-2013 Jörg Wendel
#***************************************************************************

FFMDIR = /usr/include

# ----------------------------------------------------------------------------
# User settings

# like to support a touch screen ?
#WITH_TOUCH = 1

# like the X renderer to display directly on local xorg (grpahtft-fe not needed) 
#WITH_X = 1

# compile the graphtft-fe an the therefor tcp communication
WITH_TCPCOM = 1

# compile wit old patch version (depricated)
#WITH_OLD_PATCH = 1

# compile with epg2vdr timer support
#WITH_EPG2VDR = 1

# Name of the plugin

PLUGIN    = graphtftng
HISTFILE  = "HISTORY.h"

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'define _VERSION ' $(HISTFILE) | awk '{ print $$3 }' | sed -e 's/[";]//g')
LASTHIST    = $(shell grep '^\#[0-9][0-9]' $(HISTFILE) | head -1)
LASTCOMMENT = $(subst |,\n,$(shell sed -n '/$(LASTHIST)/,/^ *$$/p' $(HISTFILE) | tr '\n' '|'))
LASTTAG     = $(shell git describe --tags --abbrev=0)
BRANCH      = $(shell git rev-parse --abbrev-ref HEAD)
GIT_REV     = $(shell git describe --always 2>/dev/null)

# The directory environment

# Use package data if installed...otherwise assume we're under the VDR source directory:
PKGCFG = $(if $(VDRDIR),$(shell pkg-config --variable=$(1) $(VDRDIR)/vdr.pc),$(shell pkg-config --variable=$(1) vdr || pkg-config --variable=$(1) ../../../vdr.pc))
LIBDIR = $(call PKGCFG,libdir)
LOCDIR = $(call PKGCFG,locdir)
PLGCFG = $(call PKGCFG,plgcfg)
#
TMPDIR ?= /tmp

### The compiler options:

export CFLAGS   = $(call PKGCFG,cflags)
export CXXFLAGS = $(call PKGCFG,cxxflags) -ggdb -fPIC -Wall -Wunused-variable -Wunused-label -Wunused-value -Wunused-function -Wno-unused-result

### The version number of VDR's plugin API:

APIVERSION = $(call PKGCFG,apiversion)

#### Allow user defined options to overwrite defaults:

-include $(PLGCFG)

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### The name of the shared object file:

SOFILE = libvdr-$(PLUGIN).so

### Includes and Defines (add further entries here):

INCLUDES += -I$(VDRDIR)/include -I. \
	         -I./imlibrenderer \
				-I./imlibrenderer/fbrenderer \
				-I./imlibrenderer/xrenderer \
				-I./imlibrenderer/dmyrenderer

INCLUDES += $(shell pkg-config libgtop-2.0 --cflags 2>/dev/null)
INCLUDES += $(shell pkg-config libexif --cflags 2>/dev/null)

DEFINES += -DPLUGIN_NAME_I18N='"$(PLUGIN)"' -DVDR_PLUGIN
DEFINES += -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS 

ifdef GIT_REV
   DEFINES += -DGIT_REV='"$(GIT_REV)"'
endif

ifdef WITH_OLD_PATCH
	DEFINES += -D_OLD_PATCH
endif

ifdef WITH_TCPCOM
  DEFINES += -DWITH_TCPCOM
endif

ifdef	WITH_EPG2VDR
	DEFINES += -DWITH_EPG2VDR
endif

ifdef WITH_X
  DEFINES += -DWITH_X
endif

# deactivate VDRs swap definition since the plugin needs std::sort() !

DEFINES += -D__STL_CONFIG_H

LIBS += $(shell pkg-config libexif --libs 2>/dev/null)
LIBS += $(shell pkg-config imlib2 --libs 2>/dev/null)
LIBS += $(shell pkg-config libgtop-2.0 --libs 2>/dev/null)

ifdef WITH_TCPCOM
  LIBS += -ljpeg
endif

ifdef WITH_TOUCH
  DEFINES += -DWITH_TOUCH
endif

AVCODEC_INC = $(shell pkg-config libavcodec --cflags | sed -e 's/  //g')

ifeq ($(strip $(AVCODEC_INC)),)
  INCLUDES += -I$(FFMDIR) -I$(FFMDIR)/libavcodec
else
  INCLUDES += $(AVCODEC_INC)
endif

AVCODEC_LIBS = $(shell pkg-config libavcodec --libs)

ifeq ($(strip $(AVCODEC_LIBS)),)
  LIBS += -lavcodec
else
  LIBS += $(AVCODEC_LIBS)
endif

SWSCALE_INC = $(shell pkg-config libswscale --cflags 2>/dev/null)

ifeq ($(strip $(SWSCALE_INC)),)
  INCLUDES += -I$(FFMDIR) -I$(FFMDIR)/libswscale
else
  INCLUDES += $(SWSCALE_INC)
endif

SWSCALE_LIBS = $(shell pkg-config libswscale --libs 2>/dev/null)

ifeq ($(strip $(SWSCALE_LIBS)),)
  LIBS += -lswscale
else
  LIBS += $(SWSCALE_LIBS)
endif

### The object files (add further files here):

OBJS = $(PLUGIN).o dspitems.o vlookup.o status.o display.o \
	          setup.o scan.o theme.o common.o sysinfo.o \
	          touchthread.o \
	          imlibrenderer/imlibrenderer.o \
			 	 imlibrenderer/fbrenderer/fbrenderer.o \
				 renderer.o comthread.o tcpchannel.o \
				 scraper2vdr.o

ifdef WITH_X
  OBJS += imlibrenderer/xrenderer/xrenderer.o
  LIBS += -lX11
endif

### The main target:

ifdef WITH_X
  all: $(SOFILE) i18n fe
else
  all: $(SOFILE) i18n
endif

### Implicit rules:

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) -o $@ $<

### Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies

$(DEPFILE): Makefile
	@$(MAKEDEP) $(CXXFLAGS) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) > $@

-include $(DEPFILE)

### Internationalization (I18N):

PODIR     = po
I18Npo    = $(wildcard $(PODIR)/*.po)
I18Nmo    = $(addsuffix .mo, $(foreach file, $(I18Npo), $(basename $(file))))
I18Nmsgs  = $(addprefix $(DESTDIR)$(LOCDIR)/, $(addsuffix /LC_MESSAGES/vdr-$(PLUGIN).mo, $(notdir $(foreach file, $(I18Npo), $(basename $(file))))))
I18Npot   = $(PODIR)/$(PLUGIN).pot

%.mo: %.po
	msgfmt -c -o $@ $<

$(I18Npot): $(wildcard *.c)
	xgettext -C -cTRANSLATORS --no-wrap --no-location -k -ktr -ktrNOOP --package-name=vdr-$(PLUGIN) --package-version=$(VERSION) --msgid-bugs-address='<vdr@jwendel.de>' -o $@ `ls $^`

%.po: $(I18Npot)
	msgmerge -U --no-wrap --no-location --backup=none -q -N $@ $<
	@touch $@

$(I18Nmsgs): $(DESTDIR)$(LOCDIR)/%/LC_MESSAGES/vdr-$(PLUGIN).mo: $(PODIR)/%.mo
	install -D -m644 $< $@

.PHONY: i18n
i18n: $(I18Nmo) $(I18Npot)

install-i18n: $(I18Nmsgs)

### Targets:

# ----------------------------------------------------------------------------
# Check madatory environment
#

check-builddep:
	if ! pkg-config libavcodec; then \
		echo "Missing libavcodec, aborting build !!"; \
		exit 1; \
	fi
	if ! pkg-config libswscale; then \
		echo "Missing libswscale, aborting build !!"; \
		exit 1; \
	fi
	if ! pkg-config imlib2; then \
		echo "Missing imlib2, aborting build !!"; \
		exit 1; \
	fi
	if ! pkg-config libexif; then \
		echo "Missing libexif, aborting build !!"; \
		exit 1; \
	fi
	if ! pkg-config libgtop-2.0; then \
		echo "Missing libgtop-2.0, aborting build !!"; \
		exit 1; \
	fi

$(SOFILE): check-builddep $(COMMONOBJS) $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared $(OBJS) $(LIBS) -o $@

fe:
	$(MAKE) -C ./graphtft-fe all

install-fe:
	$(MAKE) -C ./graphtft-fe install

install-lib: $(SOFILE)
	install -D -m644 $^ $(DESTDIR)$(LIBDIR)/$^.$(APIVERSION)

install: install-lib

dist: clean clean-fe
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

ifdef WITH_TCPCOM
clean: clean-plug clean-fe
else
clean: clean-plug
endif

clean-plug:
	@-rm -f $(OBJS) $(DEPFILE) *.so *.tgz core*
	@-rm -f $(PODIR)/*.mo $(PODIR)/*.pot
	@-rm -f $(PACKAGE).tgz t1
	@-rm -f *~ */*~ */*/*~

clean-fe:
	@rm -rf plasma/build imlibrenderer/imlibtest
	$(MAKE) -C ./graphtft-fe clean

t1: test.c scan.c scan.h
	$(CXX) $(INCLUDES) -ggdb $(LIBS) scan.c test.c -lrt -o $@

# ------------------------------------------------------
# Git / Versioning / Tagging
# ------------------------------------------------------

vcheck:
	git fetch
	if test "$(LASTTAG)" = "$(VERSION)"; then \
		echo "Warning: tag/version '$(VERSION)' already exists, update HISTORY first. Aborting!"; \
		exit 1; \
	fi

push: vcheck
	echo "tagging git with $(VERSION)"
	git tag $(VERSION)
	git push --tags
	git push

commit: vcheck
	git commit -m "$(LASTCOMMENT)" -a

git: commit push

showv:
	@echo "Git ($(BRANCH)):\\n  Version: $(LASTTAG) (tag)"
	@echo "Local:"
	@echo "  Version: $(VERSION)"
	@echo "  Change:"
	@echo -n "   $(LASTCOMMENT)"
	echo
