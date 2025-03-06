#include "k_memory.hpp"
#include "utils.hpp"

namespace Kernel {
    MemoryAllocator::MemoryAllocator() {
        // Calculate the maximum number of blocks that we can allocate.
        blocks_t total_blocks = ((uint64)HEAP_END_ADDR - (uint64)HEAP_START_ADDR) / MEM_BLOCK_SIZE;

        // Initialize the allocation table with zeroes, after that move the HEAP_START after the table.
        this->alloc_table = (blocks_t*)HEAP_START_ADDR;
        for (blocks_t i = 0; i < total_blocks; ++i) {
            this->alloc_table[i] = 0;
        }
        HEAP_START_ADDR = (void*)((uint64)HEAP_START_ADDR + total_blocks * sizeof(blocks_t));

        if((uint64)HEAP_START_ADDR % MEM_BLOCK_SIZE != 0) {
            // In case the HEAP_START_ADDR address is not alligned to blocks, then shift it backwards (to the left) by remainder to make it aligned. 
            // Then push it forwards by the size of the block, so that its not within the alloc table.
            HEAP_START_ADDR = (void*)((uint64)HEAP_START_ADDR - (uint64)HEAP_START_ADDR % MEM_BLOCK_SIZE + MEM_BLOCK_SIZE);
        }
        this->fb_head = (FreeBlocks*)HEAP_START_ADDR;

        // Divide the space between the current HEAP_START_ADDR and HEAP_END_ADDR addresses by the block size, to get the number of usable blocks. 
        // The remainder that is smaller than one size of the block will be discarded through integer division. As the smallest allocatable unit is a block.
        this->fb_head->n_blocks = ((uint64)HEAP_END_ADDR - (uint64)HEAP_START_ADDR) / MEM_BLOCK_SIZE;
        this->fb_head->next = (FreeBlocks*)nullptr;
        this->fb_head->prev = (FreeBlocks*)nullptr;
    }

    MemoryAllocator& MemoryAllocator::get_instance() {
        static MemoryAllocator mem_allocator;
        return mem_allocator;
    }

    void* MemoryAllocator::alloc(blocks_t n_blocks) {
        // Go through the list of free blocks, and try to find exactly n free blocks, or at least more than n, Best-Fit algorithm.
        FreeBlocks* best = (FreeBlocks*)nullptr;
        for (FreeBlocks* curr = this->fb_head; curr && (!best || best->n_blocks != n_blocks); curr = curr->next) {
            if (curr->n_blocks >= n_blocks && (!best || curr->n_blocks < best->n_blocks)) {
                best = curr;
            }
        }

        if (best) {
            blocks_t remaining_blocks = best->n_blocks - n_blocks;
            if (to_blocks(sizeof(FreeBlocks)) <= remaining_blocks) {
                // If there is enough memory left for header FreeBlocks, then we are creating new FreeBlocks element, and we are chaining it to the list.
                FreeBlocks* new_fb = (FreeBlocks*)((uint64)best + n_blocks * MEM_BLOCK_SIZE);
                new_fb->n_blocks = remaining_blocks;
                new_fb->next = best->next;
                new_fb->prev = best->prev;

                if (new_fb->prev) {
                    new_fb->prev->next = new_fb;
                }

                if (new_fb->next) {
                    new_fb->next->prev = new_fb;
                }

                if (this->fb_head == best) {
                    this->fb_head = new_fb;
                }
            }
            else {
                // If there is not enough free blocks, we are unchaining the FreeBlocks element.
                if (best->prev) {
                    best->prev->next = best->next;
                }

                if (best->next) {
                    best->next->prev = best->prev;
                }

                if (this->fb_head == best) {
                    this->fb_head = best->next;
                }
            }

            // Write for this memory allocation, for this address best, how many blocks have we allocated to the allocation table, and return the free memory location.
            alloc_table[((uint64)best - (uint64)HEAP_START_ADDR) / MEM_BLOCK_SIZE] = n_blocks;
            return (void*)best;
        }

        // If we failed to find at least n free blocks or more, we return nullptr.
        return nullptr;
    }

    int MemoryAllocator::free(void* address) {
        if (!address) {
            return ADDRESS_IS_NULL;
        }

        if ((uint64)address % MEM_BLOCK_SIZE != 0) {
            // If the address is not alligned to the block size, it for sure wasn't allocated by the kernel.
            return ADDRESS_IS_NOT_ALIGNED;
        }

        blocks_t n_blocks = this->alloc_table[((uint64)address - (uint64)HEAP_START_ADDR) / MEM_BLOCK_SIZE];
        if (n_blocks == 0) {
            // If the address does not use any blocks in the allocation table, then it wasn't allocated by the kernel.
            return ADDRESS_IS_NOT_USED;
        }

        // Find the adequate place where we can place the new FreeBlock.
        FreeBlocks* new_fb = (FreeBlocks*)address, *prev = nullptr;
        for (FreeBlocks* curr_fb = this->fb_head; curr_fb && new_fb >= curr_fb; curr_fb = curr_fb->next) {
            if (new_fb < (FreeBlocks*)((uint64)curr_fb + curr_fb->n_blocks * MEM_BLOCK_SIZE)) {
                // In case the address belongs to the free part of the memory, then it for sure didn't come from the kernel.
                return ADDRESS_IS_NOT_USED;
            }
            prev = curr_fb;
        }

        // Only after that, write at the address of used blocks, the number of free blocks.
        new_fb->n_blocks = n_blocks;

        // Chaining the freed blocks to the list of free blocks.
        if (prev) {
            new_fb->prev = prev;
            new_fb->next = prev->next;
            if (new_fb->next) {
                new_fb->next->prev = new_fb;
            }
            prev->next = new_fb;
        }
        else {
            new_fb->next = fb_head;
            new_fb->prev = (FreeBlocks*)nullptr;
            if (this->fb_head) {
                this->fb_head->prev = new_fb;
            }
            this->fb_head = new_fb;
        }

        // We remember in the allocation table, that for this address we haven't allocated any blocks.
        this->alloc_table[((uint64)address - (uint64)HEAP_START_ADDR) / MEM_BLOCK_SIZE] = 0;

        MemoryAllocator::merge_blocks(new_fb);
        MemoryAllocator::merge_blocks(new_fb->prev);
        return MEM_SUCCESS;
    }

    int MemoryAllocator::merge_blocks(FreeBlocks* fb) {
        if (!fb) {
            return ADDRESS_IS_NULL;
        }

        if (fb->next && fb->next == (FreeBlocks*)((uint64)fb + fb->n_blocks * MEM_BLOCK_SIZE)) {
            // In case fb has successor that starts immediatelly after this one ends, we can merge them.
            fb->n_blocks += fb->next->n_blocks;
            fb->next = fb->next->next;

            if (fb->next) {
                fb->next->prev = fb;
            }

            return MEM_SUCCESS;
        }

        return MEM_FAILED;
    }
}