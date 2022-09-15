/*
 * This program implements an in-place malloc and free using a buffer in the
 * data segement of memory. It is based on an interview question I did while
 * interviewing.
 */

#include <iostream>
#include <iomanip>
#include <assert.h>
#include <string.h>
using namespace std;

// Constants and heap space.
constexpr int HEAP_SIZE = 4096; // 4KB simulated heap.
constexpr int BLOCK_SIZE = 16;  // Blocks are always multiples of 16 bytes.
constexpr uint8_t NO_FREE_BLOCKS = 0;
char simulated_heap[HEAP_SIZE];

/* Initialize the heap. Leave the first BLOCK_SIZE bytes free, except 1st byte.
 * The first byte contains the offset of the first free list block (in
 * BLOCK_SIZE-byte blocks), so this is initialized to 1.
 */
void init_heap() {

    // The offset of the first block (in BLOCK_SIZE byte multiples).
    simulated_heap[0] = 1;

    // Start/end pointers.
    simulated_heap[BLOCK_SIZE]  = 0;
    simulated_heap[HEAP_SIZE-1] = 0;

    // Size of block in BLOCK_SIZE byte blocks (255 * 16 = 4080 bytes)
    simulated_heap[BLOCK_SIZE+1] = 0xff;
}

// Reads the byte at position `index` in the heap.
uint8_t read_unsigned(uint16_t index) {
    assert (0 <= index && index < HEAP_SIZE);
    return (uint8_t) simulated_heap[index];
}

/* Get the size of the block at `offset` in bytes.
 * Assumes the size of the block is written in its 2nd byte. */
uint16_t get_block_bytes(uint8_t offset) {
    uint16_t i = read_unsigned(1 + offset * BLOCK_SIZE);
    return i * BLOCK_SIZE;
}

// Get a free block's next pointer.
uint8_t get_next(int offset) {
    uint16_t block_bytes = get_block_bytes(offset);
    return read_unsigned(-1 + block_bytes + offset * BLOCK_SIZE);
}

// Get a free block's prev pointer.
uint8_t get_prev(int offset) {
    return read_unsigned(offset * BLOCK_SIZE);
}

// Update the next pointer of a given block.
void update_next(int offset, uint8_t new_next) {
    uint16_t block_bytes = get_block_bytes(offset);
    simulated_heap[-1 + block_bytes + offset * BLOCK_SIZE] = new_next;
}

// Update the next pointer of a given block.
void update_prev(int offset, uint8_t new_prev) {
    simulated_heap[offset * BLOCK_SIZE] = new_prev;
}

// Write the size of this block in terms of 16-byte chunks into its 2nd byte.
void update_size(int offset, uint8_t chunks) {
    simulated_heap[1 + offset * BLOCK_SIZE] = chunks;
}

void rdx_free(void *ptr) {

    // Read the size of this block.
    uint8_t *iptr = (uint8_t *) ptr;
    uint8_t chunks = *(iptr-1);

    // Get the offset of this block with pointer arithmetic.
    uint8_t offset = (uint8_t) ((iptr - (uint8_t *)simulated_heap - 1) / 16);

    // Write the size of the block to its second byte (for get_block_bytes).
    simulated_heap[offset * BLOCK_SIZE + 1] = chunks;

    // The current head of the list.
    uint8_t head = read_unsigned(0);

    // Add this block to the head of the free list.
    update_next(offset, head);
    update_prev(offset, 0);
    update_size(offset, chunks);

    simulated_heap[0] = offset;
}

/* Returns address of a block with `bytes` space if available, otherwise NULL.
 * Looks through the free-list of blocks for one of appropriate size.
 * If the block has excess room, split the block and add the remainder to the
 * free list. Write the number of chunks in the block to its first byte, and
 * return a pointer to the second byte. */
void *rdx_malloc(int bytes) {

    // Add an extra byte for block size and round it up to a multiple of 16.
    bytes++;
    if (bytes % BLOCK_SIZE != 0) {
        bytes += BLOCK_SIZE - (bytes % BLOCK_SIZE);
    }
    assert (bytes % BLOCK_SIZE == 0);

    // The first block in the free list.
    uint8_t offset = read_unsigned(0);

    while (offset != 0) {
        uint16_t block_bytes = get_block_bytes(offset);

        // There is enough space.
        if (block_bytes >= bytes) {

            uint8_t prev = get_prev(offset);
            uint8_t next = get_next(offset);

            // Make the previous block point to the next block.
            if (prev != 0) {
                update_next(prev, next);
            } else if (next != 0) {
                simulated_heap[0] = next;
            } else {
                simulated_heap[0] = NO_FREE_BLOCKS;
            }

            // Make the next block point to the previous block.
            if (next != 0) {
                update_prev(next, prev);
            }

            // Write the size of this block into the start.
            simulated_heap[offset * BLOCK_SIZE] = bytes / 16;

            /* If this block had bytes leftover, split it and add remainder
             * to the beginning of the list. */
            if (block_bytes > bytes) {
                int leftover_bytes = block_bytes - bytes;
                assert (leftover_bytes % BLOCK_SIZE == 0);

                /* Write the number of chunks in this block to the first byte.
                 * Then, call free, passing a pointer to the second byte.
                 * This avoids rewriting code to add the block to the list. */
                uint8_t chunks = leftover_bytes / 16;
                simulated_heap[offset * BLOCK_SIZE + bytes] = chunks;
                void *ptr = simulated_heap + offset * BLOCK_SIZE + bytes + 1;
                rdx_free(ptr);
            }

            return (void *) (simulated_heap + offset * BLOCK_SIZE + 1);
        }

        // Look to the next block.
        else {
            offset = get_next(offset);
        }
    }

    return NULL;
}

// Dump a range of bytes to stdout.
void dump(int start, int end) {
    assert (start <= end);
    assert (0 <= start && start < HEAP_SIZE);
    assert (0 <= end && end < HEAP_SIZE);

    cout << "--------";
    cout << "dumping bytes " << start << " through " << end;
    for (int i = start; i <= end; i++) {
        int j = (int) (uint8_t) simulated_heap[i];
        if (i % BLOCK_SIZE == 0) {
            cout << "\n";
        }
        std::cout << std::setfill('0') << std::setw(3) << j << "|";
    }
    cout << "--------\n";
}

// TODO write some rigorous tests.
int main() {
    init_heap();

    char *p = (char *)rdx_malloc(16);
    strcpy(p, "hello world");
    assert (strcmp(p, "hello world") == 0);
    rdx_free(p);

    char *q = (char *)rdx_malloc(5);
    strcpy(q, "hell");
    assert (strcmp(q, "hell") == 0);
    rdx_free(q);
}
