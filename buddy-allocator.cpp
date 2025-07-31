#include <infos/mm/page-allocator.h>
#include <infos/kernel/kernel.h>
#include <infos/kernel/log.h>
#include <infos/util/bitops.h>
#include <infos/util/math.h>

using namespace infos::mm;

class BuddyPageAllocator : public PageAllocatorAlgorithm
{
public:
    static const int MAX_ORDER = 10;

    bool init(FrameDescriptor *pf_descriptors, uint64_t nr_pf_descriptors) override
    {
        pgalloc_log.message(LogLevel::INFO, "BuddyPageAllocator::init() called");
        // TODO: Implement initialization
        return true;
    }

    FrameDescriptor *allocate(int order) override
    {
        pgalloc_log.messagef(LogLevel::INFO, "BuddyPageAllocator::allocate(%d)", order);
        // TODO: Implement allocation
        return nullptr;
    }

    void free(FrameDescriptor *base, int order) override
    {
        pgalloc_log.messagef(LogLevel::INFO, "BuddyPageAllocator::free(order=%d)", order);
        // TODO: Implement freeing
    }

    void insert_range(FrameDescriptor *start, uint64_t count) override
    {
        pgalloc_log.message(LogLevel::INFO, "BuddyPageAllocator::insert_range() called");
        // TODO: Handle inserting memory ranges into the allocator
    }

    void remove_range(FrameDescriptor *start, uint64_t count) override
    {
        pgalloc_log.message(LogLevel::INFO, "BuddyPageAllocator::remove_range() called");
        // TODO: Optional: handle removing ranges
    }

    const char *name() const override
    {
        return "buddy";
    }
};

// Register it so the kernel picks it up
RegisterPageAllocatorAlgorithm(BuddyPageAllocator);
