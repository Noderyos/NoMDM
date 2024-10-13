#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/vt.h>
#include <sys/ioctl.h>

#include "login.h"

#define TTY 2

int get_free_display(){
	for (int i = 0; i < 200; ++i){
		char xlock[20];
		snprintf(xlock, 20, "/tmp/.X%d-lock", i);

		if (access(xlock, F_OK) == -1) return i;
	}

	return -1;
}


void switch_tty(int number){
	FILE* console = fopen("/dev/console", "w");

	if (console == NULL){
		return;
	}

	int fd = fileno(console);

	ioctl(fd, VT_ACTIVATE, number);
	ioctl(fd, VT_WAITACTIVE, number);

	fclose(console);
}

int main(){
	switch_tty(TTY);

	char username[64];
    char password[64];

    printf("Username: ");
    scanf("%63s", username);
    printf("Password: ");
    scanf("%63s", password);

	if(auth(username, password)) {
		return 1;
	}

	int display_id = get_free_display();
	if(display_id == 1) {
		printf("ERR: No display\n");
		return 1;
	}


	printf("Auth succeeded!\n");

	struct passwd *pw = getpwnam(username);
	if (pw == NULL) {
		fprintf(stderr, "ERROR: Cannot retrieve user data\n");
		return 1;
	}


	char vt[16], display[16];
	snprintf(vt, 15, "vt%d", TTY);
	snprintf(display, 15, ":%d", display_id);

	setenv("DISPLAY", display, 1);
	xorg(display, vt, pw, "/usr/bin/i3");
	return 0;
}
