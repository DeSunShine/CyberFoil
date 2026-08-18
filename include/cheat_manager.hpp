#pragma once

#include <string>
#include <vector>

namespace inst::cheats {
    struct LocalCheat {
        std::string buildId;
        std::string text;
    };

    // Identifiers are intentionally strict: they become components of an SD path.
    bool IsValidTitleId(const std::string& titleId);
    bool IsValidBuildId(const std::string& buildId);
    std::string TargetPath(const std::string& titleId, const std::string& buildId);
    bool IsInstalled(const std::string& titleId, const std::string& buildId);
    bool Read(const std::string& titleId, const std::string& buildId, std::string& text, std::string& error);
    bool WriteAtomically(const std::string& titleId, const std::string& buildId, const std::string& text, std::string& error);
    bool Remove(const std::string& titleId, const std::string& buildId, std::string& error);
    std::vector<LocalCheat> List(const std::string& titleId, std::string& error);
}
