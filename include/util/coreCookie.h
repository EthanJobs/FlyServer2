#ifndef _CORECOOKIE_H
#define _CORECOOKIE_H

#define COOKIE_KEY "FLYSERVER"

char *Cookie_get(char *name, char *password_md5, char *lastDate);

#endif