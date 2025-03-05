#pragma once

typedef uint32 blocks_t;

inline blocks_t to_blocks(size_t n_bytes) {
    // Izracunaj koliko blokova je neophodno za alokaciju n bajtova. NAPOMENA: Ova formula je pokazana na vezbama.
    return (n_bytes + (MEM_BLOCK_SIZE - 1)) / MEM_BLOCK_SIZE;
}

inline uint64 get_decimal_weight(uint64 number) {
    uint64 tens = 1;
    while(tens * 10 <= number && tens * 10 > tens) {
        // Dok god moze, dodaj cifru 0 na tens a da bude <= od broja, stiti se od overflow-a preko tens * 10 > tens.
        tens = tens * 10;
    }
    return tens;
}
