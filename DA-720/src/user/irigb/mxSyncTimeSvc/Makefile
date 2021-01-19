EXEC=ServiceSyncTime

CXX=g++
STRIP=strip
ifeq ("$(CONFIG_PRODUCT)","DA-682A")
CXXFLAGS=-D__DA682A__
else
ifeq ("$(CONFIG_PRODUCT)","DA-820")
CXXFLAGS=-D__DA820__
endif
endif
LDFLAGS = -L../mxirig -lmxirig-$(shell uname -m) -lrt -lm
#CXXFLAGS=-D__ENABLE_OUTPUT_FEATURE__	# The output signal feature are not support. You should use the mxIrigUtil or unitest to enable the IRIG-B output feature

export EXEC

all: $(EXEC).o
	$(CXX) $(EXEC).o -o $(EXEC) $(LDFLAGS)

%.o:%.cpp
	$(CXX) $(CXXFLAGS) $? -c

clean:
	rm -rf $(EXEC).o $(EXEC)

install: package_install
	if [ -f "/etc/redhat-release" ]; then \
		grep -q "release 7\|release 8" /etc/redhat-release && make -f Makefile.Redhat7 install || make -f Makefile.Redhat6 install; \
	elif [ -f "/etc/debian_version" ]; then \
		make -f Makefile.Debian install; \
	fi

#romfs:
#	mkdir -p $(ROMFSDIR)/usr/sbin
#	$(ROMFSINST) $(EXEC)  /usr/sbin
#	$(ROMFSINST) $(MOXA_PACKAGE_DIR)/etc/init.d/mx_irigb.sh  /etc/init.d/
#	$(STRIP) -s $(ROMFSDIR)/usr/sbin/$(EXEC)

package_install:
	mkdir -p $(DESTDIR)/usr/sbin
	install $(EXEC)  $(DESTDIR)/usr/sbin/
	$(STRIP) -s $(DESTDIR)/usr/sbin/$(EXEC)

uninstall:
	if [ -f "/etc/redhat-release" ]; then \
		grep -q "release 7\|release 8" /etc/redhat-release && make -f Makefile.Redhat7 uninstall || make -f Makefile.Redhat6 uninstall; \
	elif [ -f "/etc/debian_version" ]; then \
		make -f Makefile.Debian uninstall; \
	fi
	-/bin/rm -f $(DESTDIR)/usr/sbin/$(EXEC)
