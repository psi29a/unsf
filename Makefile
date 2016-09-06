unsf:
	$(CC) -Wall -std=gnu99 -g -O2 -c libunsf.c
	ar -cvq libunsf.a libunsf.o
	$(CC) -g -O0 -o unsf unsf.c -L. -lunsf -lm
win:
	x86_64-w64-mingw32-gcc -DUNSF_BUILD=1 -DDLL_EXPORT=1 -Wall -std=gnu99 -g -O2 -c libunsf.c
	x86_64-w64-mingw32-gcc -shared -o libunsf.dll libunsf.o -Wl,--out-implib,libunsf.dll.a
	x86_64-w64-mingw32-gcc -g -O2 -o unsf.w64 unsf.c -L. -lunsf
install: unsf
	install unsf $(DESTDIR)/usr/bin/
uninstall:
	rm -f $(DESTDIR)$/usr/bin/unsf
clean:
	rm -f unsf libunsf.o libunsf.a libunsf.dll libunsf.dll.a unsf.w64
all:	unsf
