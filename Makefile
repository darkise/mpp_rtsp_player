#############################################################################
# Makefile for building: mpp_rtsp
# Project:  mpp_rtsp.pro
# Template: app
#############################################################################

####### Compiler, tools and options
CC            = gcc
CXX           = g++
DEFINES       = -DDEBUG=1
CFLAGS        = -pipe -O2 -Wall -W -fPIC $(DEFINES)
CXXFLAGS      = -pipe -O2 -std=c++0x -Wall -W -fPIC $(DEFINES)
INCPATH       = -I../../mpp_rtsp -I. -I/usr/lib/aarch64-linux-gnu/qt5/mkspecs/linux-g++
QMAKE         = /usr/lib/aarch64-linux-gnu/qt5/bin/qmake
DEL_FILE      = rm -f
CHK_DIR_EXISTS= test -d
MKDIR         = mkdir -p
COPY          = cp -f
COPY_FILE     = cp -f
COPY_DIR      = cp -f -R
INSTALL_FILE  = install -m 644 -p
INSTALL_PROGRAM = install -m 755 -p
INSTALL_DIR   = cp -f -R
DEL_FILE      = rm -f
SYMLINK       = ln -f -s
DEL_DIR       = rmdir
MOVE          = mv -f
TAR           = tar -cf
COMPRESS      = gzip -9f
DISTNAME      = mpp_rtsp1.0.0
DISTDIR = /home/firefly/workspaces/mpp_rtsp/build/.tmp/mpp_rtsp1.0.0
LINK          = gcc
LFLAGS        = -Wl,-O1
LDFLAGS       = -lGLESv2 -lEGL -lm -lX11 -pthread -lrockchip_mpp
LIBS          = $(SUBLIBS)  
AR            = ar cqs
RANLIB        = 
SED           = sed
STRIP         = strip

####### Output directory

OBJECTS_DIR   = ./

####### Files

SOURCES       = main.c \
		mppdecoder.c \
		rtspprotocolutil.c \
		yuvdisplay.c 
OBJECTS       = main.o \
		mppdecoder.o \
		rtspprotocolutil.o \
		yuvdisplay.o

TARGET        = mpp_rtsp


first: all
####### Implicit rules

.SUFFIXES: .o .c .cpp .cc .cxx .C

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.cc.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.cxx.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.C.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o "$@" "$<"

####### Build rules

$(TARGET):  $(OBJECTS)  
	$(LINK) $(LFLAGS) -o $(TARGET) $(OBJECTS) $(OBJCOMP) $(LIBS) $(LDFLAGS)


all: $(TARGET)

dist: distdir 
	(cd `dirname $(DISTDIR)` && $(TAR) $(DISTNAME).tar $(DISTNAME) && $(COMPRESS) $(DISTNAME).tar) && $(MOVE) `dirname $(DISTDIR)`/$(DISTNAME).tar.gz . && $(DEL_FILE) -r $(DISTDIR)

distdir: FORCE
	@test -d $(DISTDIR) || mkdir -p $(DISTDIR)
	$(COPY_FILE) --parents $(DIST) $(DISTDIR)/


clean: compiler_clean 
	-$(DEL_FILE) $(OBJECTS)
	-$(DEL_FILE) *~ core *.core


distclean: clean 
	-$(DEL_FILE) $(TARGET) 
	-$(DEL_FILE) Makefile


####### Sub-libraries

check: first

compiler_yacc_decl_make_all:
compiler_yacc_decl_clean:
compiler_yacc_impl_make_all:
compiler_yacc_impl_clean:
compiler_lex_make_all:
compiler_lex_clean:
compiler_clean: 

####### Compile

main.o: main.c rtspprotocolutil.h \
		mppdecoder.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o main.o main.c

mppdecoder.o: mppdecoder.c mppdecoder.h \
		tools.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o mppdecoder.o mppdecoder.c

rtspprotocolutil.o: rtspprotocolutil.c rtspprotocolutil.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o rtspprotocolutil.o rtspprotocolutil.c

yuvdisplay.o: yuvdisplay.c yuvdisplay.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o yuvdisplay.o yuvdisplay.c

####### Install

install:  FORCE

uninstall:  FORCE

FORCE:

