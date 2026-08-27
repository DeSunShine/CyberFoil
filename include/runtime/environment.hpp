#pragma once

namespace cyberfoil::runtime {

enum class Environment {
    Unknown,
    SysMMC,
    EmuMMC,
};

Environment DetectEnvironment();
bool IsEmuMMC();

} // namespace cyberfoil::runtime
