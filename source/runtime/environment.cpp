#include "runtime/environment.hpp"
#include <switch.h>

namespace cyberfoil::runtime {

Environment DetectEnvironment() {
    if (R_FAILED(splInitialize())) return Environment::Unknown;

    u64 isEmu = 0;
    const Result rc = splGetConfig(SplConfigItem(65007), &isEmu);
    splExit();

    if (R_FAILED(rc)) return Environment::Unknown;
    return isEmu != 0 ? Environment::EmuMMC : Environment::SysMMC;
}

bool IsEmuMMC() {
    return DetectEnvironment() == Environment::EmuMMC;
}

} // namespace cyberfoil::runtime
