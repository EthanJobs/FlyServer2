#include <stdio.h>
#include <unistd.h>
#include <serverAPI.h>
#include <serverCustomAPI.h>

extern API *g_API;

// TODO: file
int file_fileUpload(taskTreePoolNode *ttpn, APINode *an, MYSQL *sqlConnection);
void file_API_init();

int file_fileUpload(taskTreePoolNode *ttpn, APINode *an, MYSQL *sqlConnection) {
    char buf[1024];
    read(ttpn->n_clientFd, buf, 1024);
    printf("%s\n", buf);
    return 0;
}

void file_API_init() {
    APINode *fileUpload = API_getAN(g_API, "/file/fileUpload");
    fileUpload->fun_handle = file_fileUpload;
}

void API_customInit() {
    file_API_init();
}