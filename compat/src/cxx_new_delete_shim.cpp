// Minimal, self-contained operator new/delete implementations.
//
// This project links with -nostdlib on Linux, so nothing provides these
// automatically, and nothing already loaded in the target server process
// exports them in a way the dynamic loader can resolve at plugin-load time
// either. Every C++ program that constructs/destroys objects needs these
// four symbols to exist somewhere; providing them here removes any
// dependency on an external C++ runtime for them entirely.
//
// -fno-sized-deallocation (set project-wide) ensures the compiler never
// emits calls to the sized delete overloads, so only the plain forms below
// are needed.

#include <cstdlib>

void* operator new(std::size_t size)
{
    void* ptr = std::malloc(size ? size : 1);
    return ptr;
}

void* operator new[](std::size_t size)
{
    void* ptr = std::malloc(size ? size : 1);
    return ptr;
}

void operator delete(void* ptr) noexcept
{
    std::free(ptr);
}

void operator delete[](void* ptr) noexcept
{
    std::free(ptr);
}
