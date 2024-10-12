NAME = nomdm
CC = gcc
CFLAGS = -Wall -Wextra -Werror=vla -Werror
LDFLAGS = -lxcb
CMD = ./$(NAME)

BIND = bin
OBJD = obj
SRCD = src
RESD = res

INCL = -I$(SRCD)

SRCS = $(SRCD)/main.c

SRCS_OBJS:= $(patsubst %.c,$(OBJD)/%.o,$(SRCS))

all: $(BIND)/$(NAME)

$(OBJD)/%.o: %.c
	@echo "CC $@"
	@mkdir -p $(@D)
	@$(CC) $(INCL) $(CFLAGS) -c -o $@ $<

$(BIND)/$(NAME): $(SRCS_OBJS)
	@echo "LD $@"
	@mkdir -p $(@D)
	@$(CC) -o $@ $^ $(LDFLAGS)

run:
	@cd $(BIND) && $(CMD)

install: $(BIND)/$(NAME)
	install -dZ ${DESTDIR}/etc/nomdm
	install -DZ $(BIND)/$(NAME) -t ${DESTDIR}/usr/bin
	install -DZ $(RESD)/xsetup.sh -t ${DESTDIR}/etc/nomdm
	install -DZ $(RESD)/nomdm.service -t ${DESTDIR}/usr/lib/systemd/system

uninstall:
	rm -rf ${DESTDIR}/etc/nomdm
	rm -f ${DESTDIR}/usr/bin/nomdm
	rm -f ${DESTDIR}/usr/lib/systemd/system/nomdm.service

clean:
	rm -rf $(BIND) $(OBJD)
