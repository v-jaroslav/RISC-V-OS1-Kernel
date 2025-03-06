#pragma once

#include "hw.h"

namespace Kernel {
    typedef uint32 blocks_t;

    class MemoryAllocator {
    private:
        struct FreeBlocks {
            FreeBlocks* next;
            FreeBlocks* prev;
            blocks_t n_blocks;
        };

        // Pointers to the freeblocks linkedlist that keeps track of which blocks are free, and the allocation table, that keeps track of how many blocks were allocated for each memory allocation.
        FreeBlocks* fb_head;
        blocks_t* alloc_table;

        MemoryAllocator();

        static int merge_blocks(FreeBlocks* fb);

    public:
        static MemoryAllocator& get_instance();

        MemoryAllocator(const MemoryAllocator&) = delete;
        MemoryAllocator &operator=(const MemoryAllocator&) = delete;

        void* alloc(blocks_t n_blocks);
        int free(void* address);

        // Success/Failure codes.
        constexpr static int MEM_SUCCESS            =  0;
        constexpr static int MEM_FAILED             = -1;
        constexpr static int ADDRESS_IS_NULL        = -2;
        constexpr static int ADDRESS_IS_NOT_ALIGNED = -3;
        constexpr static int ADDRESS_IS_NOT_USED    = -4;
    };
}
