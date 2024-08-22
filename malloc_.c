#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define ALIGN(x) (((((x) - 1) >> 2) << 2) + 4)

static __thread pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct meta_block {
    struct meta_block *next;
    struct meta_block *prev;
    int free;
    size_t size;
    void *data;
};

typedef struct meta_block *ptr_meta_block;

static ptr_meta_block base_meta_block = NULL;

ptr_meta_block get_block(void *ptr) {
    return (ptr_meta_block)((char *)ptr - offsetof(struct meta_block, data));
}

int valid_addr(void *ptr) {
    if (base_meta_block) {
        if (ptr > (void*)base_meta_block && ptr < sbrk(0)) {
            return ptr == get_block(ptr)->data;
        }
    }
    return 0;
}

static ptr_meta_block extend_heap(ptr_meta_block last, size_t size) {
    ptr_meta_block block = sbrk(0);
    if (sbrk(ALIGN(size + sizeof(struct meta_block))) == (void *) -1)
        return NULL;

    block->size = size;
    block->next = NULL;
    block->data = (void *)block + sizeof(struct meta_block);

    if (last) {
        last->next = block;
        block->prev = last;
    }
    block->free = 0;

    return block;
}

static ptr_meta_block find_block(ptr_meta_block *last, size_t size) {
    ptr_meta_block block = base_meta_block;
    while (block && !(block->free && block->size >= size)) {
        *last = block;
        block = block->next;
    }
    return block;
}

void *malloc(size_t size) {
    ptr_meta_block block, last;
    size_t aligned_size;

    if (size <= 0)
        return NULL;

    pthread_mutex_lock(&mutex);

    aligned_size = ALIGN(size);
    if (base_meta_block) {
        last = base_meta_block;
        block = find_block(&last, aligned_size);
        if (block) {
            if ((block->size - aligned_size) >= (sizeof(struct meta_block) + ALIGN(4))) {
                ptr_meta_block new_block = (ptr_meta_block)(((char *)block->data) + aligned_size);
                new_block->size = block->size - aligned_size - sizeof(struct meta_block);
                new_block->next = block->next;
                new_block->prev = block;
                new_block->free = 1;
                new_block->data = new_block + 1;
                block->next = new_block;
                block->size = aligned_size;
            }
            block->free = 0;
        } else {
            block = extend_heap(last, aligned_size);
        }
    } else {
        block = extend_heap(NULL, aligned_size);
        base_meta_block = block;
    }

    pthread_mutex_unlock(&mutex);

    if (!block)
        return NULL;

    return block->data;
}

void free(void *ptr) {
    ptr_meta_block block;

    if (!ptr || !valid_addr(ptr))
        return;

    pthread_mutex_lock(&mutex);

    block = get_block(ptr);
    block->free = 1;

    if (block->prev && block->prev->free) {
        block = block->prev;
        block->size += block->next->size + sizeof(struct meta_block);
        block->next = block->next->next;
        if (block->next)
            block->next->prev = block;
    }

    if (block->next && block->next->free) {
        block->size += block->next->size + sizeof(struct meta_block);
        block->next = block->next->next;
        if (block->next)
            block->next->prev = block;
    }

    if (!block->next) {
        if (block->prev) {
            block->prev->next = NULL;
            sbrk(0 - block->size - sizeof(struct meta_block));
        } else {
            base_meta_block = NULL;
            sbrk(0 - block->size - sizeof(struct meta_block));
        }
    }

    pthread_mutex_unlock(&mutex);
}