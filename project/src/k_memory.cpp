#include "../h/k_memory.hpp"
#include "../h/utils.hpp"

using namespace Kernel;

MemoryAllocator::MemoryAllocator() {
    // Izracunaj maximalni broj blokova, koji se moze alocirati.
    blocks_t total_blocks = ((uint64)HEAP_END_ADDR - (uint64)HEAP_START_ADDR) / MEM_BLOCK_SIZE;

    // Inicijalizuj tabelu nulama, zatim pomeri HEAP_START iza tabele.
    this->alloc_table = (blocks_t*)HEAP_START_ADDR;
    for(blocks_t* blk_usage = this->alloc_table; blk_usage < (this->alloc_table + total_blocks); blk_usage++) {
        *blk_usage = 0;
    }
    HEAP_START_ADDR = (void*)((uint64)HEAP_START_ADDR + total_blocks * sizeof(blocks_t));

    if((uint64)(HEAP_START_ADDR) % MEM_BLOCK_SIZE > 0) {
        // Ako heap_start adresa nije poravnata na blokove, pomeri je nazad u levo za ostatak, pa napred za velicinu bloka, kako bi adrese postale poravnate na blokove.
        HEAP_START_ADDR = (void*)((uint64)HEAP_START_ADDR - (uint64)(HEAP_START_ADDR) % MEM_BLOCK_SIZE + MEM_BLOCK_SIZE);
    }
    this->fb_head = (FreeBlocks*)HEAP_START_ADDR;

    // Raspodeli prostor izmedju krajnje i pocetne adrese na blokove, ostatak koji je manji od velicine bloka odbaci celobrojnim deljenjem.
    this->fb_head->n_blocks = ((uint64)HEAP_END_ADDR - (uint64)HEAP_START_ADDR) / MEM_BLOCK_SIZE;
    this->fb_head->next = (FreeBlocks*)nullptr;
    this->fb_head->prev = (FreeBlocks*)nullptr;
}

MemoryAllocator& MemoryAllocator::get_instance() {
    static MemoryAllocator mem_allocator;
    return mem_allocator;
}

void* MemoryAllocator::alloc(blocks_t n_blocks) {
    // Prodji kroz listu slobodnih blokova, pokusaj da nadjes tacno n slobodnih blokova, ili bar vise od n, Best-Fit algoritam.
    FreeBlocks* best = (FreeBlocks*)nullptr;
    for(FreeBlocks* curr = this->fb_head; curr && (!best || best->n_blocks != n_blocks); curr = curr->next) {
        if(curr->n_blocks >= n_blocks && (!best || curr->n_blocks < best->n_blocks)) {
            best = curr;
        }
    }

    if(best) {
        blocks_t remaining_blocks = best->n_blocks - n_blocks;
        if(to_blocks(sizeof(FreeBlocks)) <= remaining_blocks) {
            // Ako je ostalo dovoljno memorije za zaglavlje FreeBlocks, pravimo nov FreeBlocks element, ulancaj ga u listu.
            FreeBlocks* new_fb = (FreeBlocks*)((uint64)best + n_blocks * MEM_BLOCK_SIZE);
            new_fb->n_blocks = remaining_blocks;
            new_fb->next = best->next;
            new_fb->prev = best->prev;
            if(new_fb->prev) {
                new_fb->prev->next = new_fb;
            }
            if(new_fb->next) {
                new_fb->next->prev = new_fb;
            }
            if(this->fb_head == best) {
                this->fb_head = new_fb;
            }
        }
        else {
            // Ako nema preostalih slobodnih blokova, odlancavamo FreeBlocks element.
            if(best->prev) {
                best->prev->next = best->next;
            }
            if(best->next) {
                best->next->prev = best->prev;
            }
            if(this->fb_head == best) {
                this->fb_head = best->next;
            }
        }

        // U tabelu za adresu best za trenutnu alokaciju, upisujemo koliko ona zauzima blokova, i vracamo adresu.
        alloc_table[((uint64)best - (uint64)HEAP_START_ADDR) / MEM_BLOCK_SIZE] = n_blocks;
        return (void*)best;
    }

    // Ako nismo pronasli n slobodnih blokova, vrati nullptr.
    return nullptr;
}

int MemoryAllocator::free(void* address) {
    if(!address) {
        return ADDRESS_IS_NULL;
    }

    if((uint64)address % MEM_BLOCK_SIZE != 0) {
        // Ako adresa nije poravanata na blokove nije dosla iz alloc-a.
        return ADDRESS_IS_NOT_ALIGNED;
    }

    blocks_t n_blocks = this->alloc_table[((uint64)address - (uint64)HEAP_START_ADDR) / MEM_BLOCK_SIZE];
    if(n_blocks == 0) {
        // Ako adresa ne koristi blokove u tabeli nije dosla iz alloc-a.
        return ADDRESS_IS_NOT_USED;
    }

    // Pronadji adekvatno mesto gde moze da se smesti nov FreeBlock.
    FreeBlocks* new_fb = (FreeBlocks*)address, *prev = (FreeBlocks*)nullptr;
    for(FreeBlocks* curr_fb = this->fb_head; curr_fb && new_fb >= curr_fb; curr_fb = curr_fb->next) {
        if(new_fb < (FreeBlocks*)((uint64)curr_fb + curr_fb->n_blocks * MEM_BLOCK_SIZE)) {
            // Ako adresa pripada slobodnom delu memorije nije dosla iz alloc-a.
            return ADDRESS_IS_NOT_USED;
        }
        prev = curr_fb;
    }

    // Tek sada upisi na adresu zauzetih blokova, broj slobodnih blokova, kako bi prosle gornje provere.
    new_fb->n_blocks = n_blocks;

    // Ulancavamo slobodne blokove u listu.
    if(prev) {
        new_fb->prev = prev;
        new_fb->next = prev->next;
        if(new_fb->next) {
            new_fb->next->prev = new_fb;
        }
        prev->next = new_fb;
    }
    else {
        new_fb->next = fb_head;
        new_fb->prev = (FreeBlocks*)nullptr;
        if(this->fb_head) {
            this->fb_head->prev = new_fb;
        }
        this->fb_head = new_fb;
    }

    // Belezimo u tabeli da se za ovu adresu vise ne cuvaju blokovi.
    this->alloc_table[((uint64)address - (uint64)HEAP_START_ADDR) / MEM_BLOCK_SIZE] = 0;

    MemoryAllocator::merge_blocks(new_fb);
    MemoryAllocator::merge_blocks(new_fb->prev);
    return MEM_SUCCESS;
}

int MemoryAllocator::merge_blocks(FreeBlocks* fb) {
    // NAPOMENA: Ova funkcija je napravljena po uzoru na resenje iz Septembra 2015. Zadatak 2.
    if(!fb) {
        return ADDRESS_IS_NULL;
    }

    if(fb->next && fb->next == (FreeBlocks*)((uint64)fb + fb->n_blocks * MEM_BLOCK_SIZE)) {
        // Ako fb ima sledbenika koji pocinje odmah pored gde se ovaj zavrsava, onda mozemo da spojimo blokove.
        fb->n_blocks += fb->next->n_blocks;
        fb->next = fb->next->next;

        if(fb->next) {
            fb->next->prev = fb;
        }

        return MEM_SUCCESS;
    }

    return MEM_FAILED;
}
