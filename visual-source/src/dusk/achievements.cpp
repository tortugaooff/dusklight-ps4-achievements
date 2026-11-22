#include "dusk/achievements.h"
#include "dusk/AchievementStorage.hpp"
#include "dusk/game_mode.hpp"
#include "dusk/main.h"
#include "dusk/settings.h"
#include "dusk/speedrun.h"
#include "dusk/ui/achievements.hpp"

#include "d/d_com_inf_game.h"
#include "d/d_item_data.h"
#include "d/d_stage.h"

#include <utility>

namespace dusk {
using namespace achievement_poc;
namespace {

Mask persistent_progress() {
    Mask result = 0;
    const auto record = [&result](Id id, bool reached) {
        if (reached) result |= bit(id);
    };
    using Flags = dSv_event_flag_c;
    const auto poeCount = dComIfGs_getPohSpiritNum();
#define CARAPACA_READ_FLAG(value) dComIfGs_isEventBit(Flags::value)
#define CARAPACA_READ_ITEM(value) dComIfGs_isItemFirstBit(dItemNo_##value##_e)
#include "dusk/catalog/Observation.inc"
#undef CARAPACA_READ_FLAG
#undef CARAPACA_READ_ITEM
    record(Id::Slingshot, dComIfGs_getItem(SLOT_23, true) == dItemNo_PACHINKO_e ||
        dComIfGs_isEventBit(Flags::F_0600));
    record(Id::RescueTalo, dComIfGs_isEventBit(Flags::F_0625));
    record(Id::MidnaFreesWolf, dComIfGs_isEventBit(Flags::M_011));
    record(Id::MeetZelda, dComIfGs_isEventBit(Flags::M_012));
    record(Id::FaronVessel, dComIfGs_isEventBit(Flags::F_0055));
    record(Id::RestoreFaron, dComIfGs_isEventBit(Flags::M_019));
    record(Id::FishingRod, dComIfGp_checkItemGet(dItemNo_FISHING_ROD_1_e, 0));
    record(Id::CoroLantern, dComIfGs_getItem(SLOT_1, true) == dItemNo_KANTERA_e ||
        dComIfGs_isEventBit(Flags::M_095));
    record(Id::RecoverLantern, dComIfGs_isEventBit(Flags::F_0226));
    record(Id::GaleBoomerang, dComIfGs_getItem(SLOT_0, true) == dItemNo_BOOMERANG_e);
    // PAL GZ2P01 STAG records share save table 16 across D_MN05, D_MN05A and D_MN05B.
    constexpr int forestTemple = 16;
    if (dComIfGp_getStageStagInfo() != nullptr) {
        record(Id::ForestMap, dComIfGs_isDungeonItemMap(forestTemple));
        record(Id::ForestBossKey, dComIfGs_isDungeonItemBossKey(forestTemple));
        record(Id::DefeatDiababa, dComIfGs_isStageBossEnemy(forestTemple) ||
            dComIfGs_isEventBit(Flags::M_022));
    }
    return result;
}

} // namespace

AchievementSystem& AchievementSystem::get() {
    static AchievementSystem instance;
    return instance;
}

void AchievementSystem::signal(Id id) {
    mObserved |= bit(id);
}

void AchievementSystem::tick() {
    const Mask observed = std::exchange(mObserved, Mask{});
    const auto& mode = gamemode::getGameModeManager();
    const bool eligible = mode.isCurrentGameMode(gamemode::kVanillaGameModeId) ||
                          mode.isCurrentGameMode(speedrun::kSpeedrunGameModeId);
    if (eligible && IsGameLaunched) {
        mPending |= observed;
        if (dComIfGp_getPlayer(0) != nullptr) {
            mPending |= persistent_progress();
        }
    }

    if (ConfigPath.empty()) {
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    if (now < mNextStorageAttempt) {
        return;
    }
    if (mLoaded && (mPending & ~mCore.unlocked()).none()) {
        mPending = 0;
        return;
    }

    const auto path = ConfigPath / "achievements-v3.dat";
    if (!mLoaded) {
        const auto loaded = AchievementStorage::load(path, ConfigPath / "achievements-poc-v1.dat",
            ConfigPath / "achievements-v2.dat");
        if (!loaded) {
            mStorageUnavailable = true;
            mNextStorageAttempt = now + std::chrono::seconds(2);
            return;
        }
        mCore = AchievementCore(*loaded);
        mLoaded = true;
        mStorageUnavailable = false;
    }

    auto next = mCore;
    const Mask newlyUnlocked = next.unlock(mPending);
    if (newlyUnlocked.any() && !AchievementStorage::save(path, next.unlocked())) {
        mStorageUnavailable = true;
        mNextStorageAttempt = now + std::chrono::seconds(2);
        return;
    }
    mCore = next;
    mPending = 0;
    mStorageUnavailable = false;
    if (newlyUnlocked.any() && getSettings().game.enableAchievementToasts) {
        ui::show_achievement_toasts(newlyUnlocked);
    }
}

} // namespace dusk
