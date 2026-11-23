#ifdef NDEBUG
#undef NDEBUG
#endif
#include "AchievementStorage.hpp"

#include <cassert>
#include <set>

static dusk::achievement_poc::Mask observe(std::string_view flag, std::string_view item,
                                          unsigned poeCount, bool hasFishingRod = false,
                                          bool hasLantern = false) {
    using namespace dusk::achievement_poc;
    Mask mask;
    const auto record = [&mask](Id id, bool reached) {
        if (reached) mask |= bit(id);
    };
#define CARAPACA_READ_FLAG(value) (flag == #value)
#define CARAPACA_READ_ITEM(value) (item == #value)
#include "catalog/Observation.inc"
#undef CARAPACA_READ_FLAG
#undef CARAPACA_READ_ITEM
    return mask;
}

int main(int argc, char** argv) {
    using namespace dusk::achievement_poc;
    assert(argc == 2);
    const auto root = std::filesystem::path(argv[1]);
    std::filesystem::create_directories(root);
    assert(kCatalogCount == 85);
    std::set<std::string_view> keys;
    Mask expected;
    Mask mandatory;
    for (const auto& entry : catalog) {
        assert(keys.insert(entry.key).second);
        const Mask observed = bit(entry.id);
        assert((expected & observed).none());
        expected |= observed;
        if (!entry.optional) mandatory |= observed;
        if (entry.id == Id::carapaca_mastery) continue;
        AchievementCore core;
        assert(core.unlock({}).none());
        assert(core.unlock(observed) == observed);
        assert(core.unlock(observed).none());
    }
    assert(expected.all());
    assert(mandatory.count() == 47);
    assert(!mandatory.test(static_cast<std::size_t>(Id::carapaca_mastery)));
    assert(mandatory.test(static_cast<std::size_t>(Id::defeat_ganondorf)));
    assert(mastery_ready(mandatory));
    assert(!mastery_ready(mandatory & ~bit(Id::DefeatDiababa)));
    AchievementCore core;
    assert(core.unlock(expected) == (expected & ~bit(Id::carapaca_mastery)));
    assert(core.unlocked().count() == 84);
    assert(goatRunCompleted(20, 20, 59999));
    assert(!goatRunCompleted(20, 20, 60000));
    assert(!goatRunCompleted(20, 19, 59999));
    assert(!goatRunCompleted(20, 20, 0));
    assert(observe({}, {}, 19).none());
    assert(observe({}, {}, 20) == bit(Id::poe_20));
    assert(observe({}, {}, 39) == bit(Id::poe_20));
    assert(observe({}, {}, 40) == (bit(Id::poe_20) | bit(Id::poe_40)));
    assert(observe({}, {}, 59) == observe({}, {}, 40));
    assert(observe({}, {}, 60) == (observe({}, {}, 40) | bit(Id::poe_60)));
    assert(observe({}, {}, 0).none());
    assert(observe("M_095", {}, 0).none());
    const Mask equipment = bit(Id::FishingRod) | bit(Id::CoroLantern);
    assert(observe({}, "FISHING_ROD_1", 0) == bit(Id::FishingRod));
    assert(observe({}, "KANTERA", 0) == bit(Id::CoroLantern));
    assert(observe({}, {}, 0, true, false) == bit(Id::FishingRod));
    assert(observe({}, {}, 0, false, true) == bit(Id::CoroLantern));
    const Mask oldSaveEquipment = observe({}, "FISHING_ROD_1", 0) | observe({}, "KANTERA", 0);
    const auto equipmentRecord = root / "equipment-v3.dat";
    AchievementCore existing(bit(Id::Slingshot));
    assert(existing.unlock(oldSaveEquipment) == equipment);
    assert(AchievementStorage::save(equipmentRecord, existing.unlocked()));
    auto reloaded = AchievementStorage::load(equipmentRecord, {});
    assert(reloaded == (equipment | bit(Id::Slingshot)));
    assert(AchievementCore(*reloaded).unlock(oldSaveEquipment).none());
#define TEST_flag(id, value) assert(observe(#value, {}, 0) == bit(Id::id));
#define TEST_item(id, value) assert(observe({}, #value, 0) == bit(Id::id));
#define TEST_poe(id, value)
#define TEST_event(id, value)
#define TEST_mastery(id, value)
#define CARAPACA_ENTRY(id, name, desc, rarity, optional, rule, value) TEST_##rule(id, value)
#include "catalog/Index.inc"
#undef CARAPACA_ENTRY
#undef TEST_flag
#undef TEST_item
#undef TEST_poe
#undef TEST_event
#undef TEST_mastery

    for (unsigned old = 0; old <= legacyBits; ++old) {
        const auto sample = root / std::to_string(old);
        std::filesystem::create_directories(sample);
        const auto v1 = sample / "achievements-poc-v1.dat";
        const auto v2 = sample / "achievements-v3.dat";
        std::ofstream(v1, std::ios::binary) << "1:" << old << '\n';
        assert(AchievementStorage::load(v2, v1) == Mask{old});
        assert(AchievementStorage::load(v2, v1) == Mask{old});
        assert(AchievementStorage::save(v2, core.unlocked()));
        assert(AchievementStorage::load(v2, v1) == core.unlocked());
        assert(AchievementStorage::load_legacy(v1) == Mask{old});
    }

    const auto v2 = root / "published.dat";
    std::ofstream(v2, std::ios::binary) << "CARAPACA:2\nslingshot\nforest_boss_key\n";
    const auto v3 = root / "migrated-v3.dat";
    auto loaded = AchievementStorage::load(v3, {}, v2);
    assert(loaded == (bit(Id::Slingshot) | bit(Id::ForestBossKey)));
    std::ifstream original(v2, std::ios::binary);
    assert(std::string(std::istreambuf_iterator<char>(original), {}) ==
           "CARAPACA:2\nslingshot\nforest_boss_key\n");
    AchievementCore restored(*loaded);
    const auto retroactive = bit(Id::Slingshot) | bit(Id::defeat_argorok) | bit(Id::poe_60);
    assert(restored.unlock(retroactive) == (retroactive & ~bit(Id::Slingshot)));
    assert(restored.unlock(retroactive).none());
    std::ofstream(v3, std::ios::binary) << "CARAPACA:3\nslingshot\nslingshot\n";
    assert(!AchievementStorage::load(v3, {}));
    std::ofstream(v3, std::ios::binary) << "CARAPACA:3\nslingshot";
    assert(!AchievementStorage::load(v3, {}));
}
