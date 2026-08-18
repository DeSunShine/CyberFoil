#include "cheat_manager.hpp"

#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>

namespace {
    constexpr const char* kCheatRoot = "sdmc:/atmosphere/contents";

    bool IsUpperHex(const std::string& value, std::size_t minLength, std::size_t maxLength)
    {
        if (value.size() < minLength || value.size() > maxLength)
            return false;
        for (unsigned char c : value) {
            if (!std::isdigit(c) && !(c >= 'A' && c <= 'F'))
                return false;
        }
        return true;
    }
}

namespace inst::cheats {
    bool IsValidTitleId(const std::string& titleId) { return IsUpperHex(titleId, 16, 16); }
    bool IsValidBuildId(const std::string& buildId) { return IsUpperHex(buildId, 1, 64); }

    std::string TargetPath(const std::string& titleId, const std::string& buildId)
    {
        if (!IsValidTitleId(titleId) || !IsValidBuildId(buildId))
            return {};
        return std::string(kCheatRoot) + "/" + titleId + "/cheats/" + buildId + ".txt";
    }

    bool IsInstalled(const std::string& titleId, const std::string& buildId)
    {
        const std::string path = TargetPath(titleId, buildId);
        return !path.empty() && std::filesystem::is_regular_file(path);
    }

    bool Read(const std::string& titleId, const std::string& buildId, std::string& text, std::string& error)
    {
        text.clear(); error.clear();
        const std::string path = TargetPath(titleId, buildId);
        if (path.empty()) { error = "Invalid title ID or build ID."; return false; }
        std::ifstream in(path, std::ios::binary);
        if (!in) { error = "Cheat file is not installed."; return false; }
        text.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
        if (!in.good() && !in.eof()) { error = "Could not read cheat file."; return false; }
        return true;
    }

    bool WriteAtomically(const std::string& titleId, const std::string& buildId, const std::string& text, std::string& error)
    {
        error.clear();
        const std::string path = TargetPath(titleId, buildId);
        if (path.empty()) { error = "Invalid title ID or build ID."; return false; }
        const std::filesystem::path target(path);
        std::error_code ec;
        std::filesystem::create_directories(target.parent_path(), ec);
        if (ec) { error = "Could not create Atmosphere cheat directory."; return false; }
        const std::filesystem::path temporary = target.string() + ".tmp";
        {
            std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
            if (!out) { error = "Could not create temporary cheat file."; return false; }
            out.write(text.data(), static_cast<std::streamsize>(text.size()));
            out.flush();
            if (!out) { error = "Could not write cheat text."; std::filesystem::remove(temporary, ec); return false; }
        }
        // FatFs rejects rename-over-an-existing-file. Move an existing file aside
        // first, then restore it if promoting the complete temporary file fails.
        const std::filesystem::path backup = target.string() + ".bak";
        std::filesystem::remove(backup, ec); // stale backup from an interrupted older write
        const bool replacing = std::filesystem::exists(target, ec);
        if (ec) { std::filesystem::remove(temporary, ec); error = "Could not inspect existing cheat file."; return false; }
        if (replacing && std::rename(target.string().c_str(), backup.string().c_str()) != 0) {
            std::filesystem::remove(temporary, ec);
            error = "Could not prepare existing cheat file for replacement.";
            return false;
        }
        if (std::rename(temporary.string().c_str(), target.string().c_str()) != 0) {
            if (replacing)
                std::rename(backup.string().c_str(), target.string().c_str());
            std::filesystem::remove(temporary, ec);
            error = "Could not replace cheat file.";
            return false;
        }
        if (replacing)
            std::filesystem::remove(backup, ec);
        return true;
    }

    bool Remove(const std::string& titleId, const std::string& buildId, std::string& error)
    {
        error.clear();
        const std::string path = TargetPath(titleId, buildId);
        if (path.empty()) { error = "Invalid title ID or build ID."; return false; }
        std::error_code ec;
        if (!std::filesystem::remove(path, ec)) {
            error = ec ? "Could not delete cheat file." : "Cheat file is not installed.";
            return false;
        }
        return true;
    }

    std::vector<LocalCheat> List(const std::string& titleId, std::string& error)
    {
        error.clear();
        std::vector<LocalCheat> result;
        if (!IsValidTitleId(titleId)) { error = "Invalid title ID."; return result; }
        const std::filesystem::path dir = std::string(kCheatRoot) + "/" + titleId + "/cheats";
        std::error_code ec;
        if (!std::filesystem::exists(dir, ec)) return result;
        for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
            if (ec) break;
            if (!entry.is_regular_file() || entry.path().extension() != ".txt") continue;
            const std::string buildId = entry.path().stem().string();
            if (!IsValidBuildId(buildId)) continue;
            std::string text, readError;
            if (Read(titleId, buildId, text, readError)) result.push_back({buildId, std::move(text)});
        }
        if (ec) error = "Could not list local cheats.";
        return result;
    }
}
