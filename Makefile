include Makefile.defs

default:	all

# Build and create directories before installing.
install::	all
	install -d -m 755 $(INSTALL)/doc/linradio-toolkit-$(VERSION)
	install -d -m 755 $(INSTALL)/man/man1
	install -d -m 755 $(INSTALL)/bin
	install -d -m 755 $(INSTALL)/lib
	install -d -m 755 $(INSTALL)/lib/linradio-toolkit
	install -d -m 755 $(INSTALL)/include/linradio
ifeq ($(DRIVER_KIND),module)
	install -d -m 755 /lib/modules/`uname -r`/misc
endif
	install -m 644 README INSTALL CHANGES TODO KNOWN-PROBLEMS AUTHORS \
		$(INSTALL)/doc/linradio-toolkit-$(VERSION)

######################################################################

SUBDIRS=	driver tcl perl examples

all install clean::
	@for i in $(SUBDIRS); do $(MAKE) -C $$i $@ || exit 1; done

# Print a message after building.
all::
	@echo
	@echo "### Now 'export LD_LIBRARY_PATH=$(PWD)/driver'"
	@echo "###  or 'setenv LD_LIBRARY_PATH $(PWD)/driver'"
	@echo "### if you want to test before installing,"
	@echo "### or 'make install' to install in $(INSTALL)/{bin,lib,...}"
	@echo "###"
	@echo "### Try 'make -C driver test'"
	@echo
	@echo

ifeq ($(DRIVER_KIND),module)
# Print a message after installing.
install::
	@echo
	@echo "### Add 'alias char-major-82 linradio' to /etc/conf.modules"
	@echo
endif

######################## Source release ##############################

tgz:	clean
	cd ..;                                     \
	rm -f linradio-toolkit-$(VERSION); \
	ln -s toolkit linradio-toolkit-$(VERSION); \
	tar zcvfh linradio-toolkit-$(VERSION).tar.gz \
		  linradio-toolkit-$(VERSION)

########################### RPM ######################################

ifeq ($(DRIVER_KIND),module)
RPM_NAME=	linradio-toolkit-module`uname -r`
MODULE_PATH=	/lib/modules/`uname -r`/misc/linradio.o
IOCTL_PATH=	$(INSTALL)/include/linradio/radio_ioctl.h
DEV0=	/dev/winradio0
DEV1=	/dev/winradio1
DEVS0=	/dev/winradioS0
DEVS1=	/dev/winradioS1
DEVS2=	/dev/winradioS2
DEVS3=	/dev/winradioS3
else
RPM_NAME=	linradio-toolkit-usermode
MODULE_PATH=
IOCTL_PATH=
endif

RPM386=	$(RPM_NAME)-$(VERSION)-$(RELEASE).i386.rpm

rpm:	install
	@echo Building $(RPM386).rpm
	sed -e "s|%{module-path}|$(MODULE_PATH)|" \
	    -e "s|%{ioctl-path}|$(IOCTL_PATH)|" \
	    -e "s|%{dev0}|$(DEV0)|" \
	    -e "s|%{dev1}|$(DEV1)|" \
	    -e "s|%{devS0}|$(DEVS0)|" \
	    -e "s|%{devS1}|$(DEVS1)|" \
	    -e "s|%{devS2}|$(DEVS2)|" \
	    -e "s|%{devS3}|$(DEVS3)|" \
	    -e "s|%{version}|$(VERSION)|" \
	    -e "s|%{release}|$(RELEASE)|" \
	    -e "s|%{so-version}|$(SO_VERSION)|" \
	    -e "s|%{install-prefix}|$(INSTALL)|" \
	    -e "s|%{name}|$(RPM_NAME)|" \
		< toolkit.spec.src > $(RPM_NAME).spec
	rpm -bb $(RPM_NAME).spec
	mv /usr/src/redhat/RPMS/i386/$(RPM386) ..

clean::
	rm -f *.spec *~ \#*\#
