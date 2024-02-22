#ifndef _COREFORMDATA_H
#define _COREFORMDATA_H

#include <coreTree.h>

#define FORMDATASTR 0
#define FORMDATAFILE 1

typedef struct FormDataValue {
    int n_dataType;
    char *n_name;
    void *n_value;
} FormDataValue;

typedef struct FormData {
    char *n_boundary;
    Tree *n_value;
} FormData;

FormDataValue *FormDataValue_init(char *name, int dataType, void *value);
void FormDataValue_free(void *a);

FormData *FormData_init();
int FormData_fd_init(FormData *formdata, int fd);
/* FormDataValue *FormData_getValue(FormData *formdata, char *name);
void FormData_free(FormData *formdata); */

#endif