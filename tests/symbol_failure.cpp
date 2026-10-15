#include <cstdlib>
#include <cstring>

extern "C" void* __real_dlsym(void*, const char*);

// Linker wrapping only in the test build; no fault-injection switch in production.
extern "C" void* __wrap_dlsym(void* handle, const char* symbol) {
    const char* missing = std::getenv("OGL_TEST_MISSING_SYMBOL");
    if (missing && std::strcmp(missing, symbol) == 0) return nullptr;
    return __real_dlsym(handle, symbol);
}
