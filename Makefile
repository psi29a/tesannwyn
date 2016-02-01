tesannwyn:
	cc -std=c99 -g -O2 -Wall -c tes3_import.c
	cc -std=c99 -g -O2 -Wall -c tes3_export.c
	cc -std=c99 -g -O2 -Wall -o tesannwyn tesannwyn.c tes3_import.o tes3_export.o -lm
install: tesannwyn
	install tesannwyn $(DESTDIR)/usr/bin/
uninstall:
	rm -f $(DESTDIR)$/usr/bin/tesannwyn
clean:
	rm -f tesannwyn
	rm -f *.o
all:	tesannwyn
