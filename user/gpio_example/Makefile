
CC=$(PREFIX)gcc
AR=$(PREFIX)ar
CFLAGS+=-I.
LDFLAGS=-lgpio -L.
STRIP=$(PREFIX)strip
FNAME1=tgpio
FNAME2=tminipcie

all: libgpio.a $(FNAME1) $(FNAME2)
	$(STRIP) -s $(FNAME1) $(FNAME2)

$(FNAME1): $(FNAME1).c
	$(CC) $(CFLAGS) -o $(FNAME1) $(FNAME1).c $(LDFLAGS)

$(FNAME2): $(FNAME2).c
	$(CC) $(CFLAGS) -o $(FNAME2) $(FNAME2).c $(LDFLAGS)

libgpio.a: gpio.o
	$(AR) r libgpio.a gpio.o

%.o:%.c
	$(CC) -fPIC -c $<

clean:
	/bin/rm -f $(FNAME1) $(FNAME2) *.o libgpio.a
