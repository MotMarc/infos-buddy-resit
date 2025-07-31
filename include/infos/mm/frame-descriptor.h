#pragma once

#include <cstdint>

namespace infos {
    namespace mm {
        struct FrameDescriptor {
            uint8_t flags; // 0 = free, 1 = allocated
        };
    }
}
