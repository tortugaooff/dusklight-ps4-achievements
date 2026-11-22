#pragma once

#include "dusk/achievements.h"
#include "window.hpp"
#include <vector>

namespace dusk::ui {

void show_achievement_toasts(achievement_poc::Mask newlyUnlocked);
void register_carapaca_art();
void unregister_carapaca_art();

class AchievementsWindow : public Window {
public:
    AchievementsWindow();
    void update() override;
    void preview_selected();
    bool preview_pending() const { return static_cast<bool>(mPreviewFinished); }

private:
    bool handle_nav_command(Rml::Event& event, NavCommand cmd) override;
    void select_row(std::size_t index);
    enum class Filter { All, Locked, Unlocked };
    enum class RarityFilter { All, Green, Blue, Red, Luminous };
    Filter mFilter = Filter::All;
    RarityFilter mRarity = RarityFilter::All;
    std::size_t mSelection = 0;
    std::size_t mVisibleCount = 0;
    struct VisibleRow {
        Rml::Element* element;
        const achievement_poc::Definition* definition;
    };
    std::vector<VisibleRow> mRows;
    bool mScrollToSelection = false;
    const achievement_poc::Definition* mSelected = nullptr;
    std::shared_ptr<bool> mPreviewFinished;
    achievement_poc::Mask mUnlocked = 0;
    bool mStorageUnavailable = false;
};

} // namespace dusk::ui
