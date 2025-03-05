#pragma once

typedef uint32 blocks_t;

inline blocks_t to_blocks(size_t n_bytes) {
    // Calculate how many blocks are necessary to allocate n bytes of memory.
    return (n_bytes + (MEM_BLOCK_SIZE - 1)) / MEM_BLOCK_SIZE;
}

inline uint64 get_decimal_weight(uint64 number) {
    uint64 tens = 1;
    while(tens * 10 <= number && tens * 10 > tens) {
        // As long as we can, add the new digit 0 to tens, so that its lesser or equal than number, we are protecitng ourselves also from overflow, by checking if tens * 10 > tens.
        tens = tens * 10;
    }
    return tens;
}
