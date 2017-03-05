
CXX       ?= g++
CXXFLAGS  ?= -pipe -ggdb -O2 -Wall -W -D_REENTRANT -fPIC
CXXFLAGS  += -Wno-deprecated-declarations
LFLAGS    = -Wl,--no-undefined
LIBS      = -lpthread -ljpeg -lX11
LIBS      += $(shell imlib2-config --libs)
AR        = ar

PKGCFG = $(if $(VDRDIR),$(shell pkg-config --variable=$(1) $(VDRDIR)/vdr.pc),$(shell PKG_CONFIG_PATH="$$PKG_CONFIG_PATH:../../.." pkg-config --variable=$(1) vdr))
BINDIR = $(call PKGCFG,bindir)

TARGET = graphtft-fe

OBJECTS = fecommon.o \
		    comthread.o \
			 graphtft.o \
			 main.o \
			 tcpchannel.o \
			 thread.o

all: 
	@$(MAKE) $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LFLAGS) $(OBJECTS) $(LIBS) -o $(TARGET)
install:
	@cp -v --remove-destination graphtft-fe $(DESTDIR)$(BINDIR)

clean:
	rm -f *.o $(TARGET) *~

.cc.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

fecommon.o : ../common.c ../common.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"
