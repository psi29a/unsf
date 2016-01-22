CC=gcc

unsf:
	gcc -lm -g -O2 -o unsf unsf.c
install: unsf
	install unsf $(DESTDIR)/usr/bin/
uninstall:
	rm -f $(DESTDIR)$/usr/bin/unsf
clean:
	rm -f unsf
all:	unsf
