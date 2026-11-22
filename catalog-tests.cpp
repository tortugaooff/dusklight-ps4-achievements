#include "AchievementStorage.hpp"

#include <cassert>
#include <set>

int main(int argc, char** argv) {
    using namespace dusk::achievement_poc;
    assert(argc == 2);
    const auto root = std::filesystem::path(argv[1]);
    assert(std::size(catalog) == 15);
    std::set<std::string_view> ids;
    Mask seen = 0;
    for (const auto& definition : catalog) {
        assert(ids.insert(definition.key).second);
        const Mask unlock = bit(definition.id);
        assert((seen & unlock) == 0);
        seen |= unlock;
        AchievementCore core;
        assert(core.unlock(0) == 0);
        assert(core.unlock(unlock) == unlock);
        assert(core.unlock(unlock) == 0);
        assert(core.unlocked() == unlock);
    }
    assert(seen == allBits);
    assert(goatRunCompleted(20, 20, 59999));
    assert(!goatRunCompleted(20, 20, 60000));
    assert(!goatRunCompleted(20, 19, 59000));
    assert(!goatRunCompleted(10, 10, 50000));
    assert(!goatRunCompleted(20, 20, 0));
    for (Mask bits = 0; bits <= legacyBits; ++bits) {
        const auto fixture = root / std::to_string(bits);
        std::filesystem::create_directories(fixture);
        const auto legacy = fixture / "achievements-poc-v1.dat";
        const auto current = fixture / "achievements-v2.dat";
        std::ofstream(legacy, std::ios::binary) << "1:" << bits << '\n';
        assert(AchievementStorage::load(current, legacy) == bits);
        assert(AchievementStorage::load(current, legacy) == bits);
        AchievementCore migrated(bits);
        assert(migrated.unlock(allBits) == (allBits & ~bits));
        assert(AchievementStorage::save(current, migrated.unlocked()));
        assert(AchievementStorage::load(current, legacy) == allBits);
        assert(AchievementStorage::load_legacy(legacy) == bits);
    }
    const auto invalid = root / "invalid.dat";
    std::ofstream(invalid, std::ios::binary) << "CARAPACA:2\nslingshot\nslingshot\n";
    assert(!AchievementStorage::load(invalid, root / "missing.dat"));
    std::ofstream(invalid, std::ios::binary) << "CARAPACA:2\nslingshot";
    assert(!AchievementStorage::load(invalid, root / "missing.dat"));
}
