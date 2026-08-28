#include <switch.h>
#include <cstdio>
#include <sys/stat.h>

namespace {

constexpr char kStateDir[] = "sdmc:/config/cyberfoil";
constexpr char kMarker[] = "sdmc:/config/cyberfoil/backend-running.flag";
constexpr char kState[] = "sdmc:/config/cyberfoil/backend.state";

bool isEmuMMC() {
    if (R_FAILED(splInitialize())) return false;

    u64 value = 0;
    const Result rc = splGetConfig(SplConfigItem(65007), &value);
    splExit();

    return R_SUCCEEDED(rc) && value != 0;
}

void writeMarker() {
    FILE* f = std::fopen(kMarker, "wb");
    if (!f) return;

    const char marker[] = "CyberFoil backend running\n";
    std::fwrite(marker, 1, sizeof(marker) - 1, f);
    std::fclose(f);

    f = std::fopen(kState, "wb");
    if (!f) return;

    const char state[] =
        "version=1\n"
        "environment=emuMMC\n"
        "phase=sysmodule-smoke-test\n";
    std::fwrite(state, 1, sizeof(state) - 1, f);
    std::fclose(f);
}

} // namespace

extern "C" {

u32 __nx_applet_type = AppletType_None;
u32 __nx_fs_num_sessions = 4;

void __appInit() {
    Result rc = smInitialize();
    if (R_FAILED(rc)) diagAbortWithResult(rc);

    rc = fsInitialize();
    if (R_FAILED(rc)) diagAbortWithResult(rc);
}

void __appExit() {
    fsdevUnmountAll();
    fsExit();
    smExit();
}

} // extern "C"

int main(int, char**) {
    // Fail closed: no CyberFoil backend activity outside emuMMC.
    if (!isEmuMMC()) {
        return 0;
    }

    if (R_SUCCEEDED(fsdevMountSdmc())) {
        (void)::mkdir("sdmc:/config", 0777);
        (void)::mkdir(kStateDir, 0777);
        writeMarker();
    }

    // Smoke-test phase: remain alive independently of the CyberFoil GUI.
    while (true) {
        svcSleepThread(1000000000ULL);
    }
}
