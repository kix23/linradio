include ../Makefile.defs

all:

FILES=	$(wildcard *.tcl *.pl)

# Install files in lib/linradio-toolkit and wrpanel as a standalone app
install:
	install -m 755 $(FILES) $(INSTALL)/lib/linradio-toolkit
	install -m 644 man1/*.1 $(INSTALL)/man/man1
	echo "#!$(INSTALL)/bin/wishradio" > $(INSTALL)/bin/wrpanel
	cat wrpanel.tcl >> $(INSTALL)/bin/wrpanel
	chmod 755 $(INSTALL)/bin/wrpanel

run:
	LD_LIBRARY_PATH=/home/pb/Radio/linradio/toolkit/driver ./wrpanel.tcl
	

clean:
	rm -f *~ \#*\#
