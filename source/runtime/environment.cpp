#include "runtime/environment.hpp"
#include <switch.h>

namespace cyberfoil::runtime {

Environment DetectEnvironment() {
    bool isEmu = false;
    const Result rc = splGetConfig(SplConfigItem(65007), &isEmu, sizeof(isEmu));
    if (R_FAILED(rc)) return Environment::Unknown;
    return isEmu ? Environment::EmuMMC : Environment::SysMMC;
}

bool IsEmuMMC() {
    return DetectEnvironment() == Environment::EmuMMC;
}

} // namespace cyberfoil::runtime
