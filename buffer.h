#include <cstdlib>
#include <ostream>

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

    std::size_t getFullBytes() const;

    Buffer* getNext() const;

    void setNext(Buffer* next);

    Buffer* allocate(std::size_t size);

    void* getDataPtr() const;

    static Buffer* getBufferByDataPtr(void* data_ptr);

    Buffer* getLowerNeighbor() const;

    Buffer* getHigherNeighbor() const;

    void dump(std::ostream &os) const;

  private:

    void init(std::size_t full_size, bool free, Buffer* next);

    Buffer** getTail() const;

    friend void inner_test();

    uintptr_t _tail;
    Buffer* _next;
};
