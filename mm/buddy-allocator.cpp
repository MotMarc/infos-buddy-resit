#include <infos/mm/page-allocator.h>
#include <infos/mm/frame-descriptor.h>
#include <infos/util/linked-list.h>
#include <infos/kernel/kernel.h>

using namespace infos::mm;
using namespace infos::util;

#define MAX_ORDER 18  // 2^18 pages = 1GB if page size = 4KB

class BuddyPageAllocator : public PageAllocatorAlgorithm {
public:
    void initialise(FrameDescriptor *page_descriptors, uint64_t nr_page_descriptors) override {
        _page_descriptors = page_descriptors;
        _nr_page_descriptors = nr_page_descriptors;

        for (int i = 0; i < MAX_ORDER; i++) {
            _free_lists[i].clear();
        }

        int block_size = 1 << (MAX_ORDER - 1);
        for (int i = 0; i < (int)nr_page_descriptors; i += block_size) {
            if (i + block_size <= nr_page_descriptors) {
                insert_block(i, MAX_ORDER - 1);
            }
        }
    }

    FrameDescriptor *allocate(int order) override {
        int current_order = order;
        while (current_order < MAX_ORDER && _free_lists[current_order].empty()) {
            current_order++;
        }

        if (current_order == MAX_ORDER) return nullptr;

        FrameDescriptor *block = (FrameDescriptor *)_free_lists[current_order].remove_first();

        while (current_order > order) {
            current_order--;
            uint64_t buddy_index = index_of(block) + (1 << current_order);
            insert_block(buddy_index, current_order);
        }

        block->flags = 1;
        return block;
    }

    void free(FrameDescriptor *page, int order) override {
        uint64_t index = index_of(page);

        while (order < MAX_ORDER - 1) {
            uint64_t buddy_index = index ^ (1 << order);
            FrameDescriptor *buddy = &_page_descriptors[buddy_index];

            if (!is_free(buddy)) break;

            remove_specific_block(buddy, order);
            index = index < buddy_index ? index : buddy_index;
            order++;
        }

        insert_block(index, order);
    }

private:
    LinkedList _free_lists[MAX_ORDER];
    FrameDescriptor *_page_descriptors;
    uint64_t _nr_page_descriptors;

    bool is_free(FrameDescriptor *page) {
        return page->flags == 0;
    }

    uint64_t index_of(FrameDescriptor *page) {
        return page - _page_descriptors;
    }

    void insert_block(uint64_t index, int order) {
        FrameDescriptor *page = &_page_descriptors[index];
        page->flags = 0;
        _free_lists[order].append(page);
    }

    FrameDescriptor *remove_block(int order) {
        FrameDescriptor *page = (FrameDescriptor *)_free_lists[order].remove_first();
        page->flags = 1;
        return page;
    }

    void remove_specific_block(FrameDescriptor *page, int order) {
        void *current = _free_lists[order].first();
        while (current) {
            if (current == page) {
                _free_lists[order].remove(current);
                return;
            }
            current = _free_lists[order].next(current);
        }
    }
};

RegisterPageAllocator(BuddyPageAllocator);
