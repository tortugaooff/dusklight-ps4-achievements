#pragma once

#include "AchievementCore.hpp"

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

namespace dusk::achievement_poc {

class AchievementStorage {
public:
    static std::optional<Mask> load_legacy(const std::filesystem::path& path) {
        std::error_code error;
        const bool exists = std::filesystem::exists(path, error);
        if (error) {
            return std::nullopt;
        }
        if (!exists) {
            return Mask{0};
        }
        std::ifstream file(path, std::ios::binary);
        char record[4]{};
        if (!file.read(record, sizeof(record)) || file.peek() != std::char_traits<char>::eof() ||
            file.bad() || record[0] != '1' || record[1] != ':' ||
            record[2] < '0' || record[2] > static_cast<char>('0' + legacyBits) ||
            record[3] != '\n') {
            return std::nullopt;
        }
        return static_cast<Mask>(record[2] - '0');
    }

    static std::optional<Mask> load(const std::filesystem::path& path,
                                    const std::filesystem::path& legacy) {
        std::error_code error;
        if (!std::filesystem::exists(path, error)) {
            if (error) return std::nullopt;
            if (!std::filesystem::exists(legacy, error)) {
                return error ? std::nullopt : std::optional<Mask>{0};
            }
            const auto migrated = load_legacy(legacy);
            if (!migrated || !save(path, *migrated)) return std::nullopt;
            return migrated;
        }

        std::ifstream file(path, std::ios::binary);
        std::string line;
        if (!std::getline(file, line) || line != "CARAPACA:2" || file.eof()) return std::nullopt;
        Mask mask = 0;
        while (std::getline(file, line)) {
            if (file.eof()) return std::nullopt;
            Mask entry = 0;
            for (const auto& item : catalog) {
                if (line == item.key) {
                    entry = bit(item.id);
                    break;
                }
            }
            if (entry == 0 || (mask & entry) != 0) return std::nullopt;
            mask |= entry;
        }
        return file.eof() && !file.bad() ? std::optional<Mask>{mask} : std::nullopt;
    }

    static bool save(const std::filesystem::path& path, Mask unlocked) {
        auto temporary = path;
        temporary += ".tmp";
        {
            std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
            file << "CARAPACA:2\n";
            for (const auto& item : catalog) {
                if (unlocked & bit(item.id)) file << item.key << '\n';
            }
            file.close();
            if (!file) {
                return false;
            }
        }
        std::error_code error;
        std::filesystem::rename(temporary, path, error);
        return !error;
    }
};

} // namespace dusk::achievement_poc
