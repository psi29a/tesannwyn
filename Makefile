tesannwyn:
	cc -g -O2 -s -Wall -o tesannwyn tesannwyn.c -lm
install: tesannwyn
	install tesannwyn $(DESTDIR)/usr/bin/
uninstall:
	rm -f $(DESTDIR)$/usr/bin/tesannwyn
clean:
	rm -f tesannwyn
all:	tesannwyn
