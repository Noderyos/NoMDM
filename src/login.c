#include "login.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <xcb/xcb.h>
#include <security/pam_appl.h>

int login_conv(
    int num,
    const struct pam_message **msg,
    struct pam_response **resp,
    void *str)
{
    struct pam_response *response = malloc(sizeof(struct pam_response) * num);

    for (int i = 0; i < num; ++i) {
        response[i].resp_retcode = 0;
        if (msg[i]->msg_style == PAM_PROMPT_ECHO_OFF
            || msg[i]->msg_style == PAM_PROMPT_ECHO_ON) {
            response[i].resp = strdup(str);
            } else {
                response[i].resp = NULL;
            }
    }
    *resp = response;
    return PAM_SUCCESS;
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
        snprintf(cmd, 1023, "/usr/bin/xauth add %s . `/usr/bin/mcookie`", display_name);
        execl(shell, shell, "-c", cmd, NULL);
        exit(EXIT_SUCCESS);
    }

    waitpid(pid, NULL, 0);
}


void xorg(const char* display, const char* vt, struct passwd *pw, const char* desktop_cmd){
    xauth(display, pw->pw_shell, pw->pw_dir);

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

        setenv("HOME", pw->pw_dir, 1);
        setenv("PWD", pw->pw_dir, 1);
        setenv("SHELL", pw->pw_shell, 1);
        setenv("USER", pw->pw_name, 1);
        setenv("LOGNAME", pw->pw_name, 1);
        setenv("DISPLAY", display, 1);

        if (initgroups(pw->pw_name, pw->pw_gid) != 0) {
            perror("initgroups error");
            return;
        }

        if (setgid(pw->pw_gid) != 0) {
            perror("setgid error");
            return;
        }

        if (setuid(pw->pw_uid) != 0) {
            perror("setuid error");
            return;
        }

        printf("Starting %s...\n", desktop_cmd);
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

int auth(char* username, char* password) {
    pam_handle_t *pam_handle;
    struct pam_conv pam_conv = {login_conv, (void *)password};

    int result = pam_start("login", username, &pam_conv, &pam_handle);

    if (result != PAM_SUCCESS) {
        fprintf(stderr, "ERROR: PAM init: %s\n", pam_strerror(pam_handle, result));
        return 1;
    }

    result = pam_authenticate(pam_handle, 0);
    if (result != PAM_SUCCESS) {
        fprintf(stderr, "ERROR: Authentication failed: %s\n",
            pam_strerror(pam_handle, result));
        pam_end(pam_handle, result);
        return 1;
    }

    result = pam_acct_mgmt(pam_handle, 0);
    if (result != PAM_SUCCESS) {
        fprintf(stderr, "ERROR: Access management: %s\n", pam_strerror(pam_handle, result));
        pam_end(pam_handle, result);
        return 1;
    }
    pam_end(pam_handle, result);
    return 0;
}
