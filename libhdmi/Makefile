CFLAGS= -static -Wall -O3 
CROSS_COMPILE = arm-linux-
#CROSS_COMPILE = arm-linux-gnueabihf-

CC      =$(CROSS_COMPILE)gcc #--verbose
STRIP   =$(CROSS_COMPILE)strip
LD      =$(CROSS_COMPILE)ld #-m elf32arm26
AR      =$(CROSS_COMPILE)ar
RANLIB  =$(CROSS_COMPILE)ranlib

CPP		= $(CC) -E
NM		= $(CROSS_COMPILE)nm
DEP		= $(CROSS_COMPILE)gcc
CPP		= $(CROSS_COMPILE)g++
ARFLAGS = rv
LINKER  = $(CROSS_COMPILE)g++ --verbose


CFLAGS += -I./include

LIBSRCS = hdmi_api.c libcec.c libedid.c libddc.c

LIBOBJS=$(LIBSRCS:.c=.o)

default: libhdmi.a 
libhdmi.a: $(LIBOBJS)
	rm -f $@
	$(AR) rcs $@ $(LIBOBJS)
	$(RANLIB) $@

link-check:
	if [ ! -L a_pass.h ] ; then make link ; fi

clean:
	rm -f $(LIBOBJS)
	rm -f libhdmi.a

