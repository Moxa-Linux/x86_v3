
modules:mxser
	@echo "Making modules $(TARGET_MODULE).ko ..."
	$(MAKE) -C mxser M=$(PWD) modules

install:
	-/bin/rm -rf /lib/modules/$(KERNEL_VERSION)/kernel/drivers/tty/mxser.ko
	$(MAKE) -C mxser M=$(PWD) install

clean:
	-/bin/rm -rf mxser

uninstall:
	-/bin/rm -f $(DESTDIR)/lib/modules/$(KERNEL_VERSION)/kernel/drivers/watchdog/$(TARGET_MODULE).ko
	depmod -a
	$(MAKE) -C mxser M=$(PWD) uninstall

moxa-linux-kernel-5.x.x-driver-v5.0.tgz:
	wget https://www.moxa.com/getmedia/75e70991-e74e-4403-9804-1030a75cf854/moxa-linux-kernel-5.x.x-driver-v5.0.tgz

mxser:moxa-linux-kernel-5.x.x-driver-v5.0.tgz
	tar -xzf moxa-linux-kernel-5.x.x-driver-v5.0.tgz
	(cd mxser; patch -p1 < ../mxser.RHEL8.diff)

