CC=gcc
AS=as
AR=ar
RANLIB=ranlib

CFLAGS =-Wall -std=gnu99 -g -O2
CFLAGS_LIB =-DUNSF_BUILD=1
#CFLAGS_LIB+=-fPIC -DPIC
# symbol visibility:
#CFLAGS_LIB+=-DSYM_VISIBILITY -fvisibility=hidden
ARFLAGS=crv

unsf:
	$(CC) $(CFLAGS) $(CFLAGS_LIB) -c libunsf.c
	$(AR) $(ARFLAGS) libunsf.a libunsf.o
	$(RANLIB) libunsf.a
	$(CC) $(CFLAGS) -o unsf unsf.c -L. -lunsf -lm

install: unsf
	install unsf $(DESTDIR)/usr/bin/
uninstall:
	rm -f $(DESTDIR)$/usr/bin/unsf

clean:
	rm -f unsf libunsf.o libunsf.a

all:	unsf
