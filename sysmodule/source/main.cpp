#include <switch.h>
#include <cstdio>
#include <sys/stat.h>

namespace {

constexpr char kStateDir[] = "sdmc:/config/cyberfoil";
constexpr char kMarker[] = "sdmc:/config/cyberfoil/backend-started.flag";
constexpr char kState[] = "sdmc:/config/cyberfoil/backend.state";

bool isEmuMMC() {
    if (R_FAILED(splInitialize())) return false;

    u64 value = 0;
    const Result rc = splGetConfig(SplConfigItem(65007), &value);
    splExit();

    return R_SUCCEEDED(rc) && value != 0;
}

bool writeFile(const char* path, const char* data) {
    FILE* f = std::fopen(path, "wb");
    if (!f) return false;

    const size_t length = std::strlen(data);
    const size_t written = std::fwrite(data, 1, length, f);
    std::fclose(f);
    return written == length;
}

} // namespace

extern "C" {

u32 __nx_applet_type = AppletType_None;
u32 __nx_fs_num_sessions = 4;

void __appInit() {
    // Keep initialization portable and fail-closed.  There is no UI or
    // install logic in this milestone.
    if (R_FAILED(smInitialize())) svcExitProcess(0);
    if (R_FAILED(fsInitialize())) svcExitProcess(0);
}

void __appExit() {
    fsdevUnmountAll();
    fsExit();
    smExit();
}

} // extern "C"

int main(int, char**) {
    // CyberFoil background backend is intentionally emuMMC-only.
    if (!isEmuMMC()) {
        return 0;
    }

    if (R_FAILED(fsdevMountSdmc())) {
        return 0;
    }

    (void)::mkdir("sdmc:/config", 0777);
    (void)::mkdir(kStateDir, 0777);

    const bool markerOk = writeFile(kMarker, "CyberFoil backend started\nenvironment=emuMMC\n");
    if (markerOk) {
        (void)writeFile(kState,
            "version=1\n"
            "environment=emuMMC\n"
            "phase=sysmodule-smoke-test\n"
            "alive=1\n");
    }

    // Keep the process alive independently of CyberFoil NRO.
    while (true) {
        svcSleepThread(1000000000ULL);
    }
}
