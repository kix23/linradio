include ../Makefile.defs
include ../.config

INCLUDE=	-I../driver -I/usr/X11R6/include $(TCLINCLUDE) $(TKINCLUDE)
LDFLAGS=        -L../driver -L/usr/X11R6/lib $(TCLPATH) $(TKPATH)

TARGETS=	tclradio wishradio

all:	$(TARGETS)

%.o:    %.c
	$(CC) -c $(CFLAGS) -DBSD_CODE $(INCLUDE) $*.c

tclradio:       tclwr.o tclradio.o
	$(CC) -o $@ $(LDFLAGS) $^ -llinradio $(TCLLIB) -lm -ldl

wishradio: tclwr.o wishradio.o
	$(CC) -o $@ $(LDFLAGS) $^ -llinradio $(TKLIB) $(TCLLIB) -lX11 -lm -ldl

install:
	install -m 755 tclradio wishradio $(INSTALL)/bin
	install -m 644 man1/*.1 $(INSTALL)/man/man1

clean:
	rm -f $(TARGETS) liblinradio.so *.o *~ \#*\# man1/*~ man1/\#*\#

depend:
	$(CC) -MM $(CFLAGS) $(INCLUDE) *.c > .depend

include .depend
