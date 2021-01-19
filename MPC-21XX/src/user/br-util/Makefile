#CROSS_COMPILE=arm-none-linux-gnueabi-
CC=${CROSS_COMPILE}gcc
STRIP=${CROSS_COMPILE}strip

CFLAGS+= -fPIC -DBUILTDATE='"$(shell date +%y%m%d%H)"'

# The source of brightness control utility
TARGET1=br-util
# The set brightness programming example code
TARGET2=br-setbrightness-example

.PHONY: clean romfs install uninstall package_install

all: ${TARGET1} ${TARGET1}-debug ${TARGET2} ${TARGET2}-debug

libbr.a: br.o
	ar r libbr.a br.o

libbr.so: br.o
	$(CC) br.o -shared -o libbr.so

${TARGET1}: ${TARGET1}.c libbr.a
	${CC} $(CFLAGS) ${TARGET1}.c -o ${TARGET1} -L. -lbr
	${STRIP} -s ${TARGET1}

${TARGET1}-debug: ${TARGET1}.c libbr.a
	# Define -DDEBUG for debugging purpose
	${CC} $(CFLAGS) -DDEBUG br.c ${TARGET1}.c -o ${TARGET1}-debug

${TARGET2}: ${TARGET2}.c libbr.a
	${CC} $(CFLAGS) ${TARGET2}.c -o ${TARGET2} -L. -lbr
	${STRIP} -s ${TARGET2}

${TARGET2}-debug: ${TARGET2}.c libbr.a
	# Define -DDEBUG for debugging purpose
	${CC} $(CFLAGS) -DDEBUG br.c ${TARGET2}.c -o ${TARGET2}-debug

install: all
	mkdir -p $(DESTDIR)/usr/local/bin
	install -m uog+rx $(TARGET1) $(DESTDIR)/usr/local/bin
	chmod a+s $(DESTDIR)/usr/local/bin/$(TARGET1)

package_install:
	mkdir -v -p "$(DESTDIR)/usr/local/bin"
	install -m uog+rx $(TARGET1) "$(DESTDIR)/usr/local/bin/"
	chmod a+s $(DESTDIR)/usr/local/bin/$(TARGET1)

clean:
	/bin/rm -rf *.o *.a *.so \
	${TARGET1} ${TARGET1}-debug \
	${TARGET2} ${TARGET2}-debug \
	*_src.tar.gz \
	fakeroot \
	*.deb


