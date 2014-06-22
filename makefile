all: chat rt
install:
	cp chat rt /usr/bin
uninstall:
	rm /usr/bin/chat /usr/bin/rt
tarball:
	tar czf syscalls_0.1.tar.gz chat.c rt.c makefile README
	md5sum syscalls_0.1.tar.gz
%: %.c
	gcc -g -Wall -lm -o $@ $<
