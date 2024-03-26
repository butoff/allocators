#include <cstdlib>

extern "C" {
#include <stdint.h>
}

// asserts that ptr is aligned at the size of void*
void assert_align(void *ptr);

struct Buffer {

    // allocated buffer
    Buffer(std::size_t full_size);

    // free buffer
    Buffer(std::size_t full_size, Buffer* next);

    bool isFree() const;

    std::size_t getFreeBytes() const;

    Buffer* getNext() const;

    void setNext(Buffer* next);

    Buffer* allocate(std::size_t size);

    void* getDataPtr() const;

  private:

    void init(std::size_t full_size, bool free, Buffer* next);

    Buffer** getTail() const;

    Buffer* lower_neighbor() const;

    Buffer* higher_neighbor() const;

    friend void inner_test();

    uintptr_t _tail;
    Buffer* _next;
};
