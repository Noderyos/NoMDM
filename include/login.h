#ifndef LOGIN_H
#define LOGIN_H

#include <pwd.h>

void xorg(const char* display, const char* vt, struct passwd *pw, const char* desktop_cmd);
int auth(char* username, char* password);

#endif