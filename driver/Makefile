include ../Makefile.defs

all:	targets

LIB_FILES=	wrbfo.o wrcmd.o wrfreq.o wrifg.o wrinit.o \
	wrio.o wrmode.o wrserial.o wrss.o

SHAREDLIB=	liblinradio.so

DRIVER_TARGETS:=	$(SHAREDLIB)

ifeq ($(DRIVER_KIND),usermode)
########################## User-mode #################################

%.o:	wrlib/%.c
	$(CC) -c $(CFLAGS) -DGNU_SOURCE $< -o $@

$(SHAREDLIB):	$(LIB_FILES)
	$(LD) -shared $^ -o $@

else
########################## Loadable module ###########################

$(SHAREDLIB):	wrstub.o
	$(LD) -shared $^ -o $@

DRIVER_TARGETS+=	linradio.o devices

%.o:	wrlib/%.c
	$(CC) -c $(CFLAGS) -DMODULE -D__KERNEL__ \
		-DWRVERSION=\"$(VERSION)\" $< -o $@

wrmod.o:	wrmod.c
	$(CC) -c $(CFLAGS) -DMODULE -D__KERNEL__ \
		-DWRVERSION=\"$(VERSION)\" $< -o $@

linradio.o:	$(LIB_FILES) wrmod.o
	$(LD) -o $@ -r $^

endif

######################### Common targets #############################

%.o:	%.c
	$(CC) -c $(CFLAGS) $< -o $@

targets:	wrcontrol $(DRIVER_TARGETS)

wrcontrol:	wrcontrol.o $(SHAREDLIB)
	$(CC) -o $@ $^

###################### Module-specific targets #######################

devices:	/dev/winradio0 /dev/winradio1 \
		/dev/winradioS0 /dev/winradioS1 /dev/winradioS2 /dev/winradioS3
/dev/winradio0:
	mknod $@ c 82 0    || true
/dev/winradio1:
	mknod $@ c 82 1    || true
/dev/winradioS0:
	mknod $@ c 82 128  || true
/dev/winradioS1:
	mknod $@ c 82 129  || true
/dev/winradioS2:
	mknod $@ c 82 130  || true
/dev/winradioS3:
	mknod $@ c 82 131  || true

############################### Test #################################

ifeq ($(DRIVER_KIND),module)

test:	linradio.o wrcontrol
	rmmod serial || true
	rmmod linradio || true
	sync
	insmod ./linradio.o
	sync
	LD_LIBRARY_PATH=. ./wrcontrol -P $(PORT) -p 1 -u 0 -a 0 -f 96e6 -m 3

else

test:	wrcontrol

	LD_LIBRARY_PATH=. ./wrcontrol -P $(PORT) -p 1 -u 0 -a 0 -f 96e6 -m 3

endif

######################### Build tools ################################

install::
	install -m 755 wrcontrol $(INSTALL)/bin/wrcontrol
	install -m 755 $(SHAREDLIB) $(INSTALL)/lib/liblinradio.so.$(SO_VERSION)
	rm -f $(INSTALL)/lib/liblinradio.so
	ln -s $(INSTALL)/lib/liblinradio.so.$(SO_VERSION) \
	      $(INSTALL)/lib/liblinradio.so
	install -m 644 wrlib/wrapi.h $(INSTALL)/include/linradio

ifeq ($(DRIVER_KIND),module)
install::	devices
	install -m 644 radio_ioctl.h $(INSTALL)/include/linradio
	install -m 644 linradio.o    /lib/modules/`uname -r`/misc
endif

clean:
	rm -f wrcontrol linradio.o $(SHAREDLIB) *.o *~ \#*\# \
		wrlib/*~ wrlib/\#*\#

depend:
	$(CC) -MM $(CFLAGS) -Iwrlib *.c wrlib/*.c > .depend

include .depend
