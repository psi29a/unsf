CC=gcc

unsf:
	gcc -g -O2 -o unsf unsf.c -lm
install: unsf
	install unsf $(DESTDIR)/usr/bin/
uninstall:
	rm -f $(DESTDIR)$/usr/bin/unsf
clean:
	rm -f unsf
all:	unsf
