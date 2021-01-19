KERNEL_VERSION_MAJOR_NUMBER:=$(shell uname -r | cut -f1 -d.)
ifeq ("$(KERNEL_VERSION_MAJOR_NUMBER)","4")
DRIVER_PATH:=driver/kernel4.x
else
DRIVER_PATH:=driver/kernel2.x_3.x
endif

all: utility_make mxser

SP1: utility_make mxsersp1

SP2: utility_make mxsersp2

install: clean utility_install driver_install
installsp1: utility_install driver_installsp1
installsp2: utility_install driver_installsp2

clean: driver_clean  utility_clean

remove: uninstall
uninstall: driver_uninstall utility_uninstall


utility_make :
	@cd utility;\
	make -s

mxser :
	@cd ${DRIVER_PATH};\
	make -s 

mxsersp1 :
	@cd ${DRIVER_PATH};\
	make SP1 -s

mxsersp2 :
	@cd ${DRIVER_PATH};\
	make SP2 -s

driver_install:
	@echo ""
	@echo " Build driver for Linux kernel ${KERNEL_VERSION_MAJOR_NUMBER}.x"
	@echo ""
	@cd ${DRIVER_PATH};\
	make install -s 

driver_installsp1:
	@cd ${DRIVER_PATH};\
	make installsp1 -s
	
driver_installsp2:
	@cd ${DRIVER_PATH};\
	make installsp2 -s
	
utility_install:
	@cd utility;\
	make install -s 


driver_clean:
	@cd ${DRIVER_PATH};\
	make clean -s


utility_clean:
	@cd utility;\
	make clean -s

driver_uninstall:
	@cd ${DRIVER_PATH};\
	make uninstall -s

utility_uninstall:
	@cd utility;\
	make uninstall -s



