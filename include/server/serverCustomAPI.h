#ifndef _SERVERCUSTOMAPI_H
#define _SERVERCUSTOMAPI_H

#include <serverAPI.h>

#define CUSTOMAPIDIR "./build"

typedef int (*HandlerFunc)(taskTreePoolNode *, struct APINode *, MYSQL *);;

void API_customInit();

#endif