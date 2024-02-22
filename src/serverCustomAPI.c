#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>
#include <serverAPI.h>
#include <serverCustomAPI.h>

extern API *g_API;

// 加载路由处理函数
void load_handlers_from_directory(const char *directory);

void load_handlers_from_directory(const char *directory) {
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(directory)) == NULL)  return;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char subdir[257];
            snprintf(subdir, sizeof(subdir), "%s/%s", directory, entry->d_name);
            load_handlers_from_directory(subdir);
            continue;
        }

        if (entry->d_type != DT_REG || strstr(entry->d_name, ".so") == NULL) continue;

        char path[257];
        snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);

        void *handle = dlopen(path, RTLD_LAZY);
        if (handle == NULL) continue;

        // 获取处理函数并设置到路由表中
        HandlerFunc custom = (HandlerFunc)dlsym(handle, "custom");
        if (custom == NULL) continue;

        char *apiName = *(char **)dlsym(handle, "APINAME");
        if (apiName == NULL) continue;

        APINode *an = API_getAN(g_API, apiName);
        if (an == NULL) continue;

        an->fun_handle = custom;
    }

    closedir(dir);
}

void API_customInit() {
    load_handlers_from_directory(CUSTOMAPIDIR);
}