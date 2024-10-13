NAME = nomdm
CC = gcc
CFLAGS = -Wall -Wextra -Werror=vla -Werror -Iinclude
LDFLAGS = -lxcb -lpam
CMD = ./$(NAME)

all: src/main.c src/login.c
	$(CC) $(CFLAGS) -o $(NAME) $? $(LDFLAGS)

install: all
	install -dZ ${DESTDIR}/etc/$(NAME)
	install -DZ $(NAME) -t ${DESTDIR}/usr/bin
	install -DZ res/xsetup.sh -t ${DESTDIR}/etc/$(NAME)
	install -DZ res/nomdm.service -t ${DESTDIR}/usr/lib/systemd/system

uninstall:
	rm -rf ${DESTDIR}/etc/$(NAME)
	rm -f ${DESTDIR}/usr/bin/$(NAME)
	rm -f ${DESTDIR}/usr/lib/systemd/system/nomdm.service

clean:
	rm -rf $(NAME)
