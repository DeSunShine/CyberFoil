#include <switch.h>
#include <cstdio>

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
    const char state[] = "version=0\nenvironment=emuMMC\nphase=backend-test\n";
    std::fwrite(state, 1, sizeof(state) - 1, f);
    std::fclose(f);
}

void removeMarker() {
    std::remove(kMarker);
}

} // namespace

extern "C" {

u32 __nx_applet_type = AppletType_None;
u32 __nx_fs_num_sessions = 1;

void __appInit() {
    R_ABORT_UNLESS(smInitialize());
    R_ABORT_UNLESS(fsInitialize());
}

void __appExit() {
    fsdevUnmountAll();
    fsExit();
    smExit();
}

} // extern "C"

int main(int, char**) {
    // The backend is deliberately fail-closed: it must never stay alive on sysMMC.
    if (!isEmuMMC()) {
        return 0;
    }

    if (R_SUCCEEDED(fsdevMountSdmc())) {
        mkdir("sdmc:/config", 0777);
        mkdir(kStateDir, 0777);
        writeMarker();
    }

    // Phase 1 only: prove persistent lifetime. IPC/worker/download/install comes later.
    while (true) {
        svcSleepThread(1000000000ULL);
    }

    removeMarker();
    return 0;
}
