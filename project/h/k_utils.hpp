#pragma once

typedef uint32 blocks_t;

namespace Kernel::Utils {
    // Just small inline functions (therefore they are exempted from the ODR one definition rule), used for general stuff.
    
    inline blocks_t to_blocks(size_t n_bytes) {
        // Calculate how many blocks are necessary to allocate n bytes of memory. If you would graph this function, it would look like "staircase".
        // IF MEM_BLOCK_SIZE = 64B, then for n_bytes=0, it would return us 0 as result. For n_bytes in range [1, MEM_BLOCK_SIZE], it would return us 1, and so on.
        return (n_bytes + (MEM_BLOCK_SIZE - 1)) / MEM_BLOCK_SIZE;
    }

    inline uint64 get_decimal_weight(uint64 number) {
        // A single digit has at least weight of 1.
        uint64 weight = 1;

        while (weight * 10 <= number && weight * 10 > weight) {
            // As long as we can, add the new digit 0 to weight. so that its lesser or equal than number.
            // And we can do so, as long as long as that new weight would still be smaller or equal to the number.
            // And as long as we wouldn't overflow. If weight * 10 < weight, then we would overflow.
            weight = weight * 10;
        }

        return weight;
    }
}