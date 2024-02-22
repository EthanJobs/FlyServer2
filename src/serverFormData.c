#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <serverFormData.h>

FormDataValue *FormDataValue_init(char *name, int dataType, void *value) {
    FormDataValue *fdv = (FormDataValue *)malloc(sizeof(FormDataValue));
    fdv->n_name = name;
    fdv->n_dataType = dataType;
    fdv->n_value = value;

    return fdv;
}

void FormDataValue_free(void *a) {
    if (a == NULL) return;

    FormDataValue *fdv = (FormDataValue *)a;
    if (fdv->n_name) free(fdv->n_name);
    if (fdv->n_value != NULL) free((char *)fdv->n_value);
    free(fdv);
    return;
}

int compareFDVToFDV(void *a, void *b) {
    return strcmp(((FormDataValue *)a)->n_name, ((FormDataValue *)b)->n_name);
}

int compareStringToFDV(void *a, void *b) {
    return strcmp((char *)a, ((FormDataValue *)b)->n_name);
}

FormData *FormData_init() {
    FormData *fromdata = (FormData *)malloc(sizeof(FormData));
    fromdata->n_value = Tree_init(compareFDVToFDV, FormDataValue_free);
    fromdata->n_boundary = NULL;

    return fromdata;
}

int FormData_fd_init(FormData *formdata, int fd) {
    char buff[2048], tmp;

    int state = read(fd, &tmp, 1);
    write(1, &tmp, 1);

    while (1) {
        if (state <= 0) return 1;
        state = read(fd, &tmp, 1);
        write(1, &tmp, 1);
    }
}