#ifndef _COREMIME_H
#define _COREMIME_H

#define MIME_FILE "./mime.json"

int MIME_init();
char *MIME_getFileMIME(char *fileName);
void MIME_free();

#endif