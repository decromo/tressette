#ifndef CLIENT_PLUG_H
#define CLIENT_PLUG_H


// uncomment this region to activate plugin mode
// #define PLUG_DEV
// #define PLUG_FILE "./libsecoclient.so"
// end region


#ifdef PLUG_DEV
void plug_open();
void plug_close();
#else // PLUG_DEV
#   define plug_open() true
#   define plug_close() true
#endif // PLUG_DEV
void plug_init();

// Function list macro
#define PLUG_FUNCS \
    F(client_main, int, void*) \
    F(render_init, void, void **memPtr)

// Function type definitions
#define F(name, ret, ...) typedef ret (plug_##name##_t)(__VA_ARGS__);
    PLUG_FUNCS
#undef F

// Declare functions or extern function pointers
#ifndef PLUG_DEV
    #define F(name, ...) plug_##name##_t name;
        PLUG_FUNCS
    #undef F
#else
    #define F(name, ...) extern plug_##name##_t *name;
        PLUG_FUNCS
    #undef F
#endif

// Declaration of plugin memory struct
// struct Render_memory;
// extern struct Render_memory *renderMemory;

#endif // CLIENT_PLUG_H