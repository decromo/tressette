#define _GNU_SOURCE
#include <stdbool.h>

#include "client/client_plug.h"

static void *renderMemory;
void plug_init() {
    render_init(&renderMemory);
}

#ifdef PLUG_DEV
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>

#include <raylib.h>

// DLL handle
static void *plug_handle = NULL;

// Function pointer global definition
#define F(name, ...) plug_##name##_t *name;
PLUG_FUNCS
#undef F

void plug_open() {
    char *error = NULL;
    // open the DLL
    do {
        plug_handle = dlopen(PLUG_FILE, RTLD_LAZY);

        if (plug_handle == NULL) {
            fprintf(stderr, "dlopen: %s\n", dlerror());
            usleep(33333);
        }

    } while (plug_handle == NULL);
    dlerror();  // clear the error

    // set the function pointers
    #define F(name, ...) \
        name = dlsym(plug_handle, #name); \
        error = dlerror(); \
        if (error != NULL) { \
            fprintf(stderr, "Could not find symbol %s in %s: %s\n", #name, PLUG_FILE, error); \
        } 
    PLUG_FUNCS
    #undef F
    return;
}

void plug_close() {
    if (dlclose(plug_handle) != 0) {
        fprintf(stderr, "dlclose: %s\n", dlerror());
    }
    return;
}
#endif // PLUG_DEV