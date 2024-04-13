#include <cassert>
#include <cstddef>
#include <iostream>
#include <new>

#include "myalloc.h"
#include "buffer.h"

Buffer* _free_buffers_head;
Buffer* _begin_sentinel;
Buffer* _end_sentinel;

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
    assert(isFree());
    return getFullBytes() - sizeof(Buffer) - sizeof(void*);
}

std::size_t Buffer::getFullBytes() const {
    return (getHigherNeighbor() - this) * sizeof(Buffer);
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

    Buffer** tail = getTail();

    // evaluate size in number of blocks of size of void*
    std::size_t p_size = size / sizeof(void*) + (size % sizeof(void*) ? 1 : 0) + 2;
    std::size_t full_size_for_allocated_buffer = p_size * sizeof(void*);

    init(getFullBytes() - full_size_for_allocated_buffer, true, getNext());
    void* ptr = getHigherNeighbor();

    size = p_size * sizeof(void*);

    Buffer* buf = new(ptr) Buffer(full_size_for_allocated_buffer);
    // HERE >>>>>>>>>>>
    assert(buf->getTail() == tail);
    // <<<<<<<<<<<<<<<<
    return buf;
}

void* Buffer::getDataPtr() const {
    assert(!isFree());
    return reinterpret_cast<void*>(const_cast<Buffer**>(&_next));
}

Buffer* Buffer::getBufferByDataPtr(void* data_ptr) {
    return reinterpret_cast<Buffer*>(reinterpret_cast<char*>(data_ptr) - offsetof(Buffer, _next));
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

Buffer* Buffer::getLowerNeighbor() const {
    return *(reinterpret_cast<Buffer* const *>(this) - 1);
}

Buffer* Buffer::getHigherNeighbor() const {
    return reinterpret_cast<Buffer*>(getTail() + 1);
}

void Buffer::dump(std::ostream& os) const {
    os << "Buffer at " << this << ", " << getFullBytes() << " bytes, " << (isFree() ? "free" : "occupied");
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

    // In order not to bother with going out of boundaries, we just place two
    // small fictional sentinel buffers at the begin and the end of the whole memory area
    const std::size_t sentinel_buffer_size = 2 * sizeof(void*);
    _begin_sentinel = new(begin) Buffer(sentinel_buffer_size);
    _end_sentinel = new(end - 2 * sizeof(void*)) Buffer(sentinel_buffer_size);

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
    assert(!allocated_buffer->isFree());

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
/**/
    Buffer *buffer = Buffer::getBufferByDataPtr(p);
    assert(!buffer->isFree());
/**/
    std::size_t new_size = buffer->getFullBytes();
std::cerr << new_size << std::endl;
    Buffer* higher = buffer;
    if (buffer->getHigherNeighbor() && buffer->getHigherNeighbor()->isFree()) {
        higher = buffer->getHigherNeighbor();
        new_size += higher->getFullBytes();
std::cerr << new_size << std::endl;
    }
std::cerr << 11111 << std::endl;
    if (buffer->getLowerNeighbor() && buffer->getLowerNeighbor()->isFree()) {
std::cerr << 22222 << std::endl;
        buffer = buffer->getLowerNeighbor();
        new_size += buffer->getFullBytes();
std::cerr << new_size << std::endl;
    }

    Buffer *new_free = new(buffer) Buffer(new_size, _free_buffers_head);
    _free_buffers_head = new_free;
/**/
}

void mydump() {
    Buffer *p = _begin_sentinel;
    do {
        p->dump(std::cerr);
        std::cerr << std::endl;
        p = p->getHigherNeighbor();
    } while (p != _end_sentinel);
    p->dump(std::cerr);
    std::cerr << std::endl;
}
