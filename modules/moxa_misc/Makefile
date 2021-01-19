# 
# You should configure the CONFIG_PRODUCT as one of these model, mc1100, da720, mc7400, mpc21xx or mc7200 before making this driver.
#

#CONFIG_PRODUCT?=mpc21xx
CONFIG_PRODUCT?=mc1100
#CONFIG_PRODUCT?=expc1519
#CONFIG_PRODUCT?=da720
#CONFIG_PRODUCT?=da682bk6
#CONFIG_PRODUCT?=mc7200
#CONFIG_PRODUCT?=mc7400

KERNEL_VERSION=`uname -r`
KDIR:=/lib/modules/$(KERNEL_VERSION)/build
TARGET_MODULE = misc-moxa-$(CONFIG_PRODUCT)
obj-m += $(TARGET_MODULE).o

export CONFIG_PRODUCT KERNEL_VERSION KDIR TARGET_MODULE

.PHONY: modules clean install package_install

modules: $(TARGET_MODULE).ko

$(TARGET_MODULE).ko: $(TARGET_MODULE).c 
	@echo "Making modules $(TARGET_MODULE).ko ..."
	$(MAKE) -C $(KDIR) M=`pwd` modules

clean:
	$(MAKE) -C $(KDIR) M=`pwd` clean
	-/bin/rm -rf *_src.tar.gz \
	fakeroot \
	*.deb

install: package_install
	if [ -f "/etc/redhat-release" ]; then \
		grep -q "release 7" /etc/redhat-release && make -f Makefile.Redhat7 install || echo "Sorry! It's not supported in this distribution"; \
	elif [ -f "/etc/debian_version" ]; then \
		make -f Makefile.Debian install; \
	fi

package_install:
	mkdir -v -p "$(DESTDIR)/lib/modules/$(KERNEL_VERSION)/kernel/drivers/misc"
	install $(TARGET_MODULE).ko "$(DESTDIR)/lib/modules/$(KERNEL_VERSION)/kernel/drivers/misc"
	depmod -a
	if [ -e "setinterface.$(CONFIG_PRODUCT)" ]; then \
	mkdir -v -p "$(DESTDIR)/usr/local/bin"; \
	install setinterface.$(CONFIG_PRODUCT) $(DESTDIR)/usr/local/bin/setinterface; \
	fi
	mkdir -v -p "$(DESTDIR)/sbin"
	install mx_gpio_export $(DESTDIR)/sbin/
	mkdir -v -p "$(DESTDIR)/lib/systemd/system"; \
	install mx_gpio_export.service $(DESTDIR)/lib/systemd/system/; \

uninstall:
	depmod -a
	if [ -f "/etc/redhat-release" ]; then \
		grep -q "release 7" /etc/redhat-release && make -f Makefile.Redhat7 uninstall || echo "Sorry! It's not supported in this distribution"; \
	elif [ -f "/etc/debian_version" ]; then \
		make -f Makefile.Debian uninstall; \
	fi
