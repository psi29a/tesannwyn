tesannwyn:
	cc -DDEBUG -ggdb -O2 -s -Wall -c common.c
	cc -DDEBUG -ggdb -O2 -s -Wall -c tes3_vtex.c
	cc -DDEBUG -ggdb -O2 -s -Wall -c tes3_import.c
	cc -DDEBUG -ggdb -O2 -s -Wall -c tes3_export.c
	cc -DDEBUG -ggdb -O2 -s -Wall -o tesannwyn tesannwyn.c common.c tes3_vtex.c tes3_import.o tes3_export.o -lm
install: tesannwyn
	install tesannwyn $(DESTDIR)/usr/bin/
uninstall:
	rm -f $(DESTDIR)$/usr/bin/tesannwyn
clean:
	rm -f tesannwyn
	rm -f *.o
all:	tesannwyn
