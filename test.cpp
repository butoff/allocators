#include <cassert>
#include <cstring>
#include <iostream>
#include <cstddef>

#include "buffer.h"
#include "myalloc.h"

// asserts that a memory at ptr is filled with a char c
void assert_region(const char* ptr, const char c, const std::size_t size) {
    for (std::size_t i = 0; i < size; i++) {
        assert(ptr[i] == c);
    }
}

void inner_test() {
    const std::size_t BUFFER_SIZE = 10 * sizeof(void*);
    const char SENTINEL = 'S';

    char arena_array[2 * BUFFER_SIZE + 2 * sizeof(void*)];
    std::memset(arena_array, SENTINEL, sizeof(arena_array));

    char* const arena = arena_array + sizeof(void*) - reinterpret_cast<uintptr_t>(arena_array) % sizeof(void*);
    assert_region(arena, SENTINEL, 2 * BUFFER_SIZE);

    assert_align(arena);
    std::cout << "ARENA OK" << std::endl;

    Buffer* buffer = new(arena) Buffer(BUFFER_SIZE);
    assert(buffer->getFreeBytes() == 0);

    assert(arena[-1] == SENTINEL);
    assert(arena[BUFFER_SIZE] == SENTINEL);

    Buffer* buffer1 = new(arena) Buffer(BUFFER_SIZE);
    Buffer* buffer2 = new(arena + BUFFER_SIZE) Buffer(BUFFER_SIZE);

    assert(!buffer1->isFree());
    assert(!buffer2->isFree());
    assert(buffer1->getTail() + 1 == reinterpret_cast<Buffer**>(buffer2));
    assert(buffer1->higher_neighbor() == buffer2);
    assert(*(buffer1->getTail()) == buffer1);
    assert(buffer2->lower_neighbor() == buffer1);

    buffer1 = new(arena) Buffer(BUFFER_SIZE, 0);
    buffer2 = new(arena + BUFFER_SIZE) Buffer(BUFFER_SIZE, 0);

    assert(buffer1->isFree());
    assert(buffer1->getFreeBytes() == BUFFER_SIZE - 3 * sizeof(void*));
    assert(buffer2->isFree());
    assert(buffer2->getFreeBytes() == BUFFER_SIZE - 3 * sizeof(void*));

    assert(buffer1->getTail() + 1 == reinterpret_cast<Buffer**>(buffer2));
    assert(buffer1->higher_neighbor() == buffer2);
    assert(*(buffer1->getTail()) == buffer1);
    assert(buffer2->lower_neighbor() == buffer1);
}

// Sentinel ReAlloc: frees the bufer previously allocated by this function,
// allocates a bufer of size bytes and returns a pointer to it. Also, creates
// sentinel blocks before and after the allocated bufer, fills them with a
// template and checks that the previously created sentinel blocks are not
// damaged.
//
// If size == 0, just frees the previously allocated bufer and returns NULL
char *sralloc(const std::size_t size) {

    // allocated buffer including sentinels
    static char *buf = 0;
    static size_t prev_size = 0;

    const size_t sentinel_size = 16;
    const char sentinel_template = 'S';

    // sentinel check
    if (buf != 0) {
        char *c = (char *) buf - 1;
        while (++c < buf + sentinel_size) {
            assert(*c == sentinel_template);
        }
        c += prev_size - 1;
        while (++c < buf + sentinel_size + prev_size + sentinel_size - 3) {
            assert(*c == sentinel_template);
        }
    }

    prev_size = size;

    if (buf == 0) {
        buf = (char*) malloc(sentinel_size + size + sentinel_size);
    } else if (size == 0) {
        free(buf);
        return 0;
    } else {
        buf = (char*) realloc(buf, sentinel_size + size + sentinel_size);
    }

    // post sentinels
    memset(buf, sentinel_template, sentinel_size);
    memset(buf + sentinel_size + size, sentinel_template, sentinel_size);

    return buf + sentinel_size;
}

void outer_test() {
    char *buf = 0;

    buf = sralloc(420);
    assert(buf != 0);
    mysetup(buf, 420);

    buf = sralloc(0x1000);
    assert(buf != 0);
    mysetup(buf, 0x1000);

    char *ptr1 = static_cast<char*>(myalloc(0x100));
    assert(ptr1 > buf);
    assert(ptr1 < buf + 0x1000);
    memset(ptr1, '1', 0x100);

    char *ptr2 = (char*) myalloc(0x100);
    assert(ptr2 > buf);
    assert(ptr2 < buf + 0x1000);
    memset(ptr2, '2', 0x100);

    //assert_region(ptr1, '1', 0x100);
    assert_region(ptr2, '2', 0x100);
    myfree(ptr1);
    myfree(ptr2);

    buf = (char*) sralloc(0x10000);
    assert(buf != 0);
    mysetup(buf, 0x10000);

    buf = sralloc(0);
    assert(buf == 0);
    mysetup(buf, 42);
}

int main() {
    std::cout << "TESTING..." << std::endl;

    inner_test();
    std::cout << "INNER TEST PASSED" << std::endl;

    outer_test();
    std::cout << "ALL TESTS PASSED" << std::endl;
}

/*
BufSize - размер участка логической памяти, который ваш аллокатор будет распределять

MaxSize - наибольший размер участка памяти, который можно аллоцировать вашим аллокатором для данного BufSize
(проверяющая система найдет его бинарным поиском)

EffectiveSize - максимальное количество памяти, которое может быть выделено пользователю для данного BufSize
(например, если мы аллоцируем много небольших участков памяти)

Ваш аллокатор памяти должен удовлетворять следующим условиям:
MaxSize должен быть не меньше 8/9 BufSize
EffectiveSize должен быть не меньше 1/9 BufSize

ваш аллокатор должен бороться с фрагментацией, т. е. если от начального состояния аллокатора мы смогли успешно
аллоцировать какое-то количество памяти, то если мы освободим всю эту память и заново попробуем повторить
аллокацию, она должна быть успешной

если аллокатор не смог аллоцировать участок памяти нужного размера, то он должен вернуть NULL

Гарантируется
что BufSize будет не меньше 100Kb и не больше 1Mb
что минимальный аллоцируемый участок памяти будет не меньше 16 байт.
*/
