#
# Makefile for the text2skin plugin of the Video Disk Recorder
#

# External image library: graphicsmagick (default), imagemagick, imlib2 or none.
# imagemagick needs ImageMagick 6, the Magick++ API of ImageMagick 7 differs.
IMAGELIB ?= graphicsmagick

# DEBUG=1: debug output, no inlining. BENCH=1: timing output.
#DEBUG = 1
#BENCH = 1

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.

PLUGIN = text2skin

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'const char \*cText2SkinPlugin::VERSION *=' $(PLUGIN).c | awk '{ print $$5 }' | sed -e 's/[";]//g')

### The directory environment:

# Use package data if installed...otherwise assume we're under the VDR source directory:
PKG_CONFIG ?= pkg-config
PKGCFG = $(if $(VDRDIR),$(shell $(PKG_CONFIG) --variable=$(1) $(VDRDIR)/vdr.pc),$(shell PKG_CONFIG_PATH="$$PKG_CONFIG_PATH:../../.." $(PKG_CONFIG) --variable=$(1) vdr))
LIBDIR = $(call PKGCFG,libdir)
LOCDIR = $(call PKGCFG,locdir)
PLGCFG = $(call PKGCFG,plgcfg)
#
TMPDIR ?= /tmp

### The compiler options:

export CFLAGS   = $(call PKGCFG,cflags) -fPIC
export CXXFLAGS = $(call PKGCFG,cxxflags) -fPIC

### The version number of VDR's plugin API:

APIVERSION = $(call PKGCFG,apiversion)

### Allow user defined options to overwrite defaults:

-include $(PLGCFG)

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### The name of the shared object file:

SOFILE = libvdr-$(PLUGIN).so

### The object files (add further files here):

OBJS = $(PLUGIN).o loader.o display.o render.o common.o bitmap.o \
       file.o i18n.o theme.o cache.o setup.o status.o scroller.o screen.o \
       menu.o font.o \
       \
       xml/skin.o xml/parser.o xml/string.o xml/object.o xml/function.o \
       xml/type.o xml/display.o xml/xml.o

### Includes and Defines (add further entries here):

INCLUDES += -I.

DEFINES += -DPLUGIN_NAME_I18N='"$(PLUGIN)"'

ifeq ($(IMAGELIB), imagemagick)
DEFINES  += -DHAVE_IMAGEMAGICK
INCLUDES += $(shell $(PKG_CONFIG) --cflags ImageMagick++)
LIBS     += $(shell $(PKG_CONFIG) --libs ImageMagick++)
else ifeq ($(IMAGELIB), graphicsmagick)
DEFINES  += -DHAVE_IMAGEMAGICK # the Magick++ code serves both libraries
INCLUDES += $(shell $(PKG_CONFIG) --cflags GraphicsMagick++)
LIBS     += $(shell $(PKG_CONFIG) --libs GraphicsMagick++)
else ifeq ($(IMAGELIB), imlib2)
DEFINES  += -DHAVE_IMLIB2
INCLUDES += $(shell $(PKG_CONFIG) --cflags imlib2)
LIBS     += $(shell $(PKG_CONFIG) --libs imlib2)
OBJS     += quantize.o
endif

ifdef DEBUG
CXXFLAGS += -g -fno-inline
DEFINES  += -DDEBUG
endif
ifdef BENCH
DEFINES  += -DBENCH
endif

### The main target:

all: $(SOFILE) i18n

### Implicit rules:

%.o: %.c
	@echo CC $@
	$(Q)$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) -o $@ $<

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
	@echo MO $@
	$(Q)msgfmt -c -o $@ $<

$(I18Npot): $(wildcard *.c)
	@echo GT $@
	$(Q)xgettext -C -cTRANSLATORS --no-wrap --no-location -k -ktr -ktrNOOP --package-name=vdr-$(PLUGIN) --package-version=$(VERSION) --msgid-bugs-address='<https://github.com/unf/vdr-plugin-text2skin/issues>' -o $@ $^

%.po: $(I18Npot)
	@echo PO $@
	$(Q)msgmerge -U --no-wrap --no-location --backup=none -q -N $@ $<
	@touch $@

$(I18Nmsgs): $(DESTDIR)$(LOCDIR)/%/LC_MESSAGES/vdr-$(PLUGIN).mo: $(PODIR)/%.mo
	install -D -m644 $< $@

.PHONY: i18n
i18n: $(I18Nmo) $(I18Npot)

install-i18n: $(I18Nmsgs)

### Targets:

$(SOFILE): $(OBJS)
	@echo LD $@
	$(Q)$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared $(OBJS) $(LIBS) -o $@

install-lib: $(SOFILE)
	install -D $^ $(DESTDIR)$(LIBDIR)/$^.$(APIVERSION)

install: install-lib install-i18n

dist: $(I18Npo) clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(PODIR)/*.mo $(PODIR)/*.pot
	@-rm -f $(OBJS) $(DEPFILE) *.so *.tgz core* *~ xml/*~
