#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/vt.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include <xcb/xcb.h>

#define TTY 2

int get_free_display(){
	for (int i = 0; i < 200; ++i){
		char xlock[20];
		snprintf(xlock, 20, "/tmp/.X%d-lock", i);

		if (access(xlock, F_OK) == -1) return i;
	}

	return -1;
}

void xauth(const char* display_name, const char* shell, const char* home){
	char xauthority[256];
	snprintf(xauthority, 255, "%s/%s", home, ".NoMDMxauth");
	setenv("XAUTHORITY", xauthority, 1);

	FILE* fp = fopen(xauthority, "ab+");
	if (fp){
		fclose(fp);
	}

	pid_t pid = fork();

	if (pid == 0){
		char cmd[1024];
		snprintf(cmd, 1023, "%s add %s . `%s`", "/usr/bin/xauth", display_name, "/usr/bin/mcookie");
		execl(shell, shell, "-c", cmd, NULL);
		exit(EXIT_SUCCESS);
	}

	waitpid(pid, NULL, 0);
}


void xorg(const char* display, const char* vt, const char* desktop_cmd){
	xauth(display, "/bin/bash", "/root");

	pid_t pid = fork();

	if (pid == 0){
		char x_cmd[1024];
		snprintf(x_cmd, 1023, "/usr/bin/X %s %s", display, vt);
		execl("/bin/bash", "/bin/bash", "-c", x_cmd, NULL);
		exit(EXIT_SUCCESS);
	}

	int ok;
	xcb_connection_t* xcb;

	do{
		xcb = xcb_connect(NULL, NULL);
		ok = xcb_connection_has_error(xcb);
		kill(pid, 0);
	}while(ok != 0 && errno != ESRCH);

	if (ok != 0){
		return;
	}

	pid_t xorg_pid = fork();

	if (xorg_pid == 0){
		char de_cmd[1024];
		snprintf(de_cmd, 1023, "%s %s", "/etc/nomdm/xsetup.sh", desktop_cmd);
		execl("/bin/bash", "/bin/bash", "-c", de_cmd, NULL);
		exit(EXIT_SUCCESS);
	}

	waitpid(xorg_pid, NULL, 0);
	xcb_disconnect(xcb);
	kill(pid, 0);

	if (errno != ESRCH){
		kill(pid, SIGTERM);
		waitpid(pid, NULL, 0);
	}
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

	int display_id = get_free_display();
	if(display_id == 1) {
		printf("ERR: No display\n");
		return 1;
	}

	char vt[16], display[16];
	snprintf(vt, 15, "vt%d", TTY);
	snprintf(display, 15, ":%d", display_id);

	setenv("DISPLAY", display, 1);
	xorg(display, vt, "/usr/bin/i3");
	return 0;
}
