#pragma once
#include <cstdint>

namespace cyberfoil::runtime {

enum class BackendCommand : std::uint32_t {
    Ping = 0x43465001,
    GetStatus,
    GetQueue,
    AddJob,
    Pause,
    Resume,
    Cancel,
};

struct BackendStatus {
    std::uint32_t protocolVersion;
    std::uint32_t active;
    std::uint64_t completedBytes;
    std::uint64_t totalBytes;
};

constexpr std::uint32_t BackendProtocolVersion = 1;

} // namespace cyberfoil::runtime
