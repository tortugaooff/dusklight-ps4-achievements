#include "achievements.hpp"

#include <fmt/format.h>

#include <iterator>

namespace dusk::ui {
namespace {
using namespace achievement_poc;

const char* rupee_color(Id id) {
    switch (id) {
    case Id::Slingshot: return "#80dba1";
    case Id::Cucco: return "#86bffc";
    case Id::Goats: return "#f0887c";
    }
    return "#e4c894";
}

const char* rupee_label(Id id) {
    switch (id) {
    case Id::Slingshot: return "Verde";
    case Id::Cucco: return "Azul";
    case Id::Goats: return "Vermelha · Opcional";
    }
    return "";
}

const char* rupee_file(Id id) {
    switch (id) {
    case Id::Slingshot: return "rupia-verde.png";
    case Id::Cucco: return "rupia-azul.png";
    case Id::Goats: return "rupia-vermelha.png";
    }
    return "";
}

Rml::String art_path(std::string_view file) {
    return resource_path(std::filesystem::path("carapaca") / file).generic_string();
}

Rml::String illustration(const Definition& item) {
    return art_path(fmt::format("{}.png", item.key));
}

void append_image(Rml::Element* parent, const char* css, const Rml::String& path) {
    auto* image = append(parent, "img");
    image->SetClass(css, true);
    image->SetAttribute("src", path);
}

void enqueue_toast(const Definition& item, std::shared_ptr<bool> preview = {}) {
    push_toast({
        .type = "carapaca",
        .title = preview ? "Prévia" : "Conquista desbloqueada!",
        .content = item.name,
        .duration = std::chrono::seconds(4),
        .illustration = illustration(item),
        .rupee = art_path(rupee_file(item.id)),
        .finished = std::move(preview),
    });
}
} // namespace

void show_achievement_toasts(achievement_poc::Mask newlyUnlocked) {
    for (const auto& item : achievement_poc::catalog) {
        if (newlyUnlocked & achievement_poc::bit(item.id)) {
            enqueue_toast(item);
        }
    }
}

AchievementsWindow::AchievementsWindow()
    : Window(Props{.tabBar = false, .styleSheets = {"res/rml/carapaca.rcss"}}) {
    mRoot->SetClass("carapaca-window", true);
    set_content([this](Rml::Element* content) {
        int totalUnlocked = 0;
        for (const auto& item : achievement_poc::catalog) {
            totalUnlocked += (mUnlocked & achievement_poc::bit(item.id)) != 0;
        }
        auto* heading = append(content, "h1");
        heading->SetClass("carapaca-title", true);
        append_text(heading, "Conquistas");
        auto* progress = append(content, "p");
        progress->SetClass("carapaca-progress", true);
        append_text(progress,
                    fmt::format("{} / {} desbloqueadas", totalUnlocked, std::size(catalog)));
        auto* progressBar = append(content, "progress");
        progressBar->SetClass("carapaca-progress-bar", true);
        progressBar->SetAttribute("value", static_cast<float>(totalUnlocked) / std::size(catalog));
        auto* filter = append(content, "p");
        filter->SetClass("carapaca-filter", true);
        const char* filterLabel = "Todas";
        if (mFilter == Filter::Locked) {
            filterLabel = "Bloqueadas";
        } else if (mFilter == Filter::Unlocked) {
            filterLabel = "Desbloqueadas";
        }
        constexpr const char* rarityLabels[] = {"Todas as rúpias", "Verde", "Azul", "Vermelha"};
        append_text(filter, fmt::format("L1 / R1 · {}    ← / → · {}    △ Prévia",
            filterLabel, rarityLabels[static_cast<int>(mRarity)]));
        if (mStorageUnavailable) {
            append_text(append(content, "p"), "Registro local indisponível.");
        }
        mVisibleCount = 0;
        const auto passes_filter = [this](const Definition& item, bool unlocked) {
            const bool matchesStatus = mFilter == Filter::All ||
                (mFilter == Filter::Unlocked && unlocked) ||
                (mFilter == Filter::Locked && !unlocked);
            const bool matchesRarity = mRarity == RarityFilter::All ||
                (mRarity == RarityFilter::Green && item.id == Id::Slingshot) ||
                (mRarity == RarityFilter::Blue && item.id == Id::Cucco) ||
                (mRarity == RarityFilter::Red && item.id == Id::Goats);
            return matchesStatus && matchesRarity;
        };
        for (const auto& item : catalog) {
            const bool unlocked = (mUnlocked & bit(item.id)) != 0;
            if (passes_filter(item, unlocked)) {
                ++mVisibleCount;
            }
        }
        if (mVisibleCount == 0) {
            mSelection = 0;
            append_text(append(content, "p"), "Nenhuma conquista neste filtro.");
        } else if (mSelection >= mVisibleCount) {
            mSelection = mVisibleCount - 1;
        }
        std::size_t visibleIndex = 0;
        mSelected = nullptr;
        mScrollToSelection = true;
        for (const auto& item : catalog) {
            const bool unlocked = (mUnlocked & achievement_poc::bit(item.id)) != 0;
            if (!passes_filter(item, unlocked)) {
                continue;
            }
            auto* row = append(content, "achievement-row");
            const bool selected = visibleIndex++ == mSelection;
            row->SetClass("carapaca-selected", selected);
            if (selected) {
                mSelected = &item;
            }
            append_image(row, "carapaca-illustration", illustration(item));
            auto* info = append(row, "achievement-info");
            auto* header = append(info, "achievement-header");
            auto* name = append(header, "achievement-name");
            name->SetClass("unlocked", unlocked);
            append_text(name, item.name);
            auto* rupee = append(info, "p");
            rupee->SetClass("carapaca-rarity", true);
            rupee->SetProperty("color", rupee_color(item.id));
            append_image(rupee, "carapaca-rupee", art_path(rupee_file(item.id)));
            append_text(rupee, rupee_label(item.id));
            auto* badge = append(header, "status-badge");
            badge->SetClass(unlocked ? "success" : "error", true);
            append_text(badge, unlocked ? "Desbloqueada" : "Bloqueada");
            auto* description = append(info, "p");
            description->SetClass("achievement-desc", true);
            append_text(description, item.description);
        }
    });
    update();
}

bool AchievementsWindow::handle_nav_command(Rml::Event& event, NavCommand cmd) {
    switch (cmd) {
    case NavCommand::Next:
    case NavCommand::Previous: {
        const int step = cmd == NavCommand::Next ? 1 : 2;
        mFilter = static_cast<Filter>((static_cast<int>(mFilter) + step) % 3);
        mSelection = 0;
        break;
    }
    case NavCommand::Left:
    case NavCommand::Right: {
        const int step = cmd == NavCommand::Right ? 1 : 3;
        mRarity = static_cast<RarityFilter>((static_cast<int>(mRarity) + step) % 4);
        mSelection = 0;
        break;
    }
    case NavCommand::Up:
        if (mSelection > 0) {
            --mSelection;
        }
        break;
    case NavCommand::Down:
        if (mSelection + 1 < mVisibleCount) {
            ++mSelection;
        }
        break;
    default:
        return Window::handle_nav_command(event, cmd);
    }
    rebuild_content();
    return true;
}

void AchievementsWindow::update() {
    if (mPreviewFinished && *mPreviewFinished) {
        mPreviewFinished.reset();
        if (active() && top_document() == this) {
            show();
            focus();
        }
    }
    const auto& system = AchievementSystem::get();
    if (mUnlocked != system.unlocked() || mStorageUnavailable != system.storageUnavailable()) {
        mUnlocked = system.unlocked();
        mStorageUnavailable = system.storageUnavailable();
        rebuild_content();
    }
    Window::update();
    if (mScrollToSelection && visible()) {
        mScrollToSelection = false;
        if (auto* row = mContentRoot->QuerySelector(".carapaca-selected")) {
            row->ScrollIntoView(Rml::ScrollIntoViewOptions{
                Rml::ScrollAlignment::Center,
                Rml::ScrollAlignment::Center,
                Rml::ScrollBehavior::Smooth,
                Rml::ScrollParentage::Closest,
            });
        }
    }
}

void AchievementsWindow::preview_selected() {
    if (!visible() || mSelected == nullptr || mPreviewFinished) {
        return;
    }
    mPreviewFinished = std::make_shared<bool>(false);
    enqueue_toast(*mSelected, mPreviewFinished);
    hide(false);
}

} // namespace dusk::ui
