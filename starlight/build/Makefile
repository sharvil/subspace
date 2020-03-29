.PHONY: all

SOURCES=$(wildcard ../src/*.cpp)
TARGET=../bin/starlight

#
# Compiler and tool configuration
#
CXX=g++

#
# Additional includes
#
CORELIB=../../../corelib/
INCLUDES=-I $(CORELIB)/include -I ../include/
LIBS=-L $(CORELIB)/SSNetLib/  \
     -L $(CORELIB)/NetLib/    \
     -L $(CORELIB)/TimeLib/   \
     -L $(CORELIB)/ThreadLib/ \
     -L $(CORELIB)/CryptoLib/ \
     -L $(CORELIB)/LogLib/    \
     -L $(CORELIB)/Nucleus/   \
     -l ssnetlib              \
     -l netlib                \
     -l timelib               \
     -l threadlib             \
     -l cryptolib             \
     -l loglib                \
     -l nucleus               \
     -l z

#
# Flags configuration
#
CFLAGS=$(shell pkg-config --cflags gio-2.0 gtk+-2.0 libglade-2.0 gdk-pixbuf-2.0) $(INCLUDES) -g
LDFLAGS=$(shell pkg-config --libs gio-2.0 gtk+-2.0 libglade-2.0 gdk-pixbuf-2.0) -rdynamic $(LIBS)

all:
	$(CXX) $(SOURCES) $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $(TARGET)
