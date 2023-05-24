CC=gcc
STRIP=strip
LFLAGS=-fPIC -shared -L/usr/lib/x86_64-linux-gnu

CFLAGS = -DPRPLED_HA

all:
	@$(CC) $(CFLAGS) mxhsrprpd.c mxhsrprp.c -lpthread -o mxhsrprpd $(LFLAGS)
	@$(CC) mxprpinfo.c -o mxprpinfo $(LFLAGS)
	@$(CC) prpsuper.c -o mxprpsuper $(LFLAGS)

install: all
	mkdir -p $(DESTDIR)/usr/local/bin
	install -m 755 mxhsrprpd $(DESTDIR)/usr/local/bin
	install -m 755 mxprpinfo $(DESTDIR)/usr/local/bin
	install -m 755 mxprpsuper $(DESTDIR)/usr/local/bin
	install -m 755 mxprpalarm $(DESTDIR)/usr/local/bin
	install -m 755 chk-mx-prp-card $(DESTDIR)/usr/local/bin
	mkdir -p $(DESTDIR)/etc/init.d
	install -m 755 mx_prp.sh $(DESTDIR)/etc/init.d

clean:
	@rm -f mxhsrprpd
	@rm -f mxprpinfo
	@rm -f mxprpsuper

