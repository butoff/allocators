#include <cassert>
#include <new>

#include "myalloc.h"
#include "buffer.h"

Buffer* _free_buffers_head;

// allocated buffer
Buffer::Buffer(std::size_t full_size) {
    init(full_size, false, 0);
}

// free buffer
Buffer::Buffer(std::size_t full_size, Buffer* next) {
    init(full_size, true, next);
}

bool Buffer::isFree() const {
    return _tail & 1;
}

std::size_t Buffer::getFreeBytes() const {
    return isFree() ? (getTail() - &_next - 1) * sizeof(void*) : 0;
}

Buffer* Buffer::getNext() const {
    return _next;
}

void Buffer::setNext(Buffer* next) {
    _next = next;
}

Buffer* Buffer::allocate(std::size_t size) {
    assert(isFree());
    assert(size <= getFreeBytes());
    // TODO
    return this;
}

void* Buffer::getDataPtr() const {
    assert(isFree());
    return reinterpret_cast<void*>(const_cast<Buffer**>(&_next));
}

void Buffer::init(std::size_t full_size, bool free, Buffer* next) {
    // must be aligned
    assert_align(this);

    // full size must be aligned too
    assert(full_size % sizeof(void*) == 0);

    // and there must be enough space
    assert(full_size >= (free ? 3 : 2) * sizeof(void*));

    // place _tail to the end of the buffer;
    Buffer** tail_ptr = reinterpret_cast<Buffer**>(reinterpret_cast<char*>(this) + full_size - sizeof(void*));
    *tail_ptr = this;
    _tail = reinterpret_cast<uintptr_t>(tail_ptr);

    if (free) {
        // free buffer flag
        _tail |= 1;
        _next = next;
    }
}

Buffer** Buffer::getTail() const {
    return reinterpret_cast<Buffer**>(_tail & ~1);
}

Buffer* Buffer::lower_neighbor() const {
    return *(reinterpret_cast<Buffer* const *>(this) - 1);
}

Buffer* Buffer::higher_neighbor() const {
    return reinterpret_cast<Buffer*>(getTail() + 1);
}

void assert_align(void *ptr) {
    assert(reinterpret_cast<uintptr_t>(ptr) % sizeof(void*) == 0);
}

void mysetup(void* buf, std::size_t size)
{
    if (!buf) return;

    char* unaligned_end = static_cast<char*>(buf) + size;
    char* const end = unaligned_end - reinterpret_cast<uintptr_t>(unaligned_end) % sizeof(void*);

    const std::size_t offset = reinterpret_cast<uintptr_t>(buf) % sizeof(void*);
    char* begin = static_cast<char*>(buf);
    if (offset) {
        begin += sizeof(void*) - offset;
    }

    size = end - begin;
    assert(size > sizeof(Buffer));

    const std::size_t sentinel_buffer_size = 2 * sizeof(void*);
    // begin and end boundary buffers
    new(begin) Buffer(sentinel_buffer_size);
    new(end - 2 * sizeof(void*)) Buffer(sentinel_buffer_size);

    _free_buffers_head = new(begin + sentinel_buffer_size) Buffer(size - 2 * sentinel_buffer_size, 0);
}

void* myalloc(std::size_t size) {
    // search free buffer, large enough
    Buffer* prev = 0;
    Buffer* buf = _free_buffers_head;
    while (buf && buf->getFreeBytes() < size) {
        prev = buf;
        buf = buf->getNext();
    }

    // no buffers large enough
    if (!buf) return 0;

    Buffer* allocated_buffer = buf->allocate(size);

    // if buffer is fully allocated
    if (allocated_buffer == buf) {
        if (prev) {
            prev->setNext(buf->getNext());
        } else {
            prev = buf->getNext();
        }
    }

    return allocated_buffer->getDataPtr();
}

void myfree(void* p) {
    // TODO
}
