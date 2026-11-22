#pragma once

#include "AchievementCore.hpp"

#include <chrono>

namespace dusk {

class AchievementSystem {
public:
    static AchievementSystem& get();
    void tick();
    void signal(achievement_poc::Id id);
    // Existing producers for the upstream catalog remain linked in this variant.
    void signal(const char*) {}

    achievement_poc::Mask unlocked() const { return mCore.unlocked(); }
    bool storageUnavailable() const { return mStorageUnavailable; }

private:
    achievement_poc::AchievementCore mCore;
    achievement_poc::Mask mObserved = 0;
    achievement_poc::Mask mPending = 0;
    std::chrono::steady_clock::time_point mNextStorageAttempt{};
    bool mLoaded = false;
    bool mStorageUnavailable = false;
};

} // namespace dusk
