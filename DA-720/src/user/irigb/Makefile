CXX=g++

DIR =  \
mxIrigUtil \
mxSyncTimeSvc


all:
	for i in $(DIR); do \
		make -C $$i; \
	done

clean:
	for i in $(DIR); do \
		make -C $$i clean; \
	done

install:
	for i in $(DIR); do \
		make -C $$i install; \
	done

uninstall:
	for i in $(DIR); do \
		make -C $$i uninstall; \
	done
