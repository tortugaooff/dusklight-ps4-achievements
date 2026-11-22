#include "achievements.hpp"

#include <aurora/rmlui.hpp>
#include <borealis/log.hpp>
#include <fmt/format.h>

#include <fstream>
#include <array>
#include <iterator>
#include <optional>
#include <span>
#include <vector>

namespace dusk::ui {
namespace {
using namespace achievement_poc;

constexpr borealis::Log Log{"dusk::ui::carapaca"};
constexpr std::string_view kArtScheme = "carapaca-art";
constexpr std::string_view kArtPrefix = "carapaca-art://";

struct Artwork {
    std::string_view key;
    unsigned size;
    std::vector<std::byte> pixels;
    bool attempted = false;
};

auto artwork = [] {
    std::array<Artwork, std::size(catalog) + 3> result{};
    std::size_t index = 0;
    for (const auto& item : catalog) result[index++] = Artwork{item.key, 256};
    for (const auto* rupee : {"rupia-verde", "rupia-azul", "rupia-vermelha"})
        result[index++] = Artwork{rupee, 128};
    return result;
}();

std::optional<aurora::rmlui::RuntimeTexture> load_artwork(std::string_view source) {
    if (!source.starts_with(kArtPrefix)) {
        return std::nullopt;
    }
    const auto key = source.substr(kArtPrefix.size());
    for (auto& art : artwork) {
        if (art.key != key) {
            continue;
        }
        if (!art.attempted) {
            art.attempted = true;
            const auto path = resource_path(fmt::format("carapaca/{}.rgba", key));
            std::ifstream file(path, std::ios::binary);
            art.pixels.resize(art.size * art.size * 4);
            if (!file.read(reinterpret_cast<char*>(art.pixels.data()), art.pixels.size()) ||
                file.peek() != std::char_traits<char>::eof() || file.bad()) {
                Log.warn("Invalid Carapaca texture: {}", path.generic_string());
                art.pixels.clear();
            }
        }
        if (!art.pixels.empty()) {
            return aurora::rmlui::RuntimeTexture{
                .width = art.size,
                .height = art.size,
                .rgba8 = std::span<const std::byte>(art.pixels),
                .premultipliedAlpha = false,
            };
        }
        break;
    }
    return std::nullopt;
}

const char* rupee_color(Rarity rarity) {
    switch (rarity) {
    case Rarity::Green: return "#80dba1";
    case Rarity::Blue: return "#86bffc";
    case Rarity::Red: return "#f0887c";
    }
    return "#e4c894";
}

const char* rupee_label(Rarity rarity) {
    switch (rarity) {
    case Rarity::Green: return "Verde";
    case Rarity::Blue: return "Azul";
    case Rarity::Red: return "Vermelha";
    }
    return "";
}

const char* rupee_file(Rarity rarity) {
    switch (rarity) {
    case Rarity::Green: return "rupia-verde";
    case Rarity::Blue: return "rupia-azul";
    case Rarity::Red: return "rupia-vermelha";
    }
    return "";
}

Rml::String art_path(std::string_view file) {
    return fmt::format("{}{}", kArtPrefix, file);
}

Rml::String illustration(const Definition& item) {
    return art_path(item.key);
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
        .rupee = art_path(rupee_file(item.rarity)),
        .finished = std::move(preview),
    });
}
} // namespace

void register_carapaca_art() {
    aurora::rmlui::register_texture_provider(std::string(kArtScheme), load_artwork);
}

void unregister_carapaca_art() {
    aurora::rmlui::unregister_texture_provider(kArtScheme);
    for (auto& art : artwork) {
        art.pixels.clear();
        art.attempted = false;
    }
}

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
                (mRarity == RarityFilter::Green && item.rarity == Rarity::Green) ||
                (mRarity == RarityFilter::Blue && item.rarity == Rarity::Blue) ||
                (mRarity == RarityFilter::Red && item.rarity == Rarity::Red);
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
            rupee->SetProperty("color", rupee_color(item.rarity));
            append_image(rupee, "carapaca-rupee", art_path(rupee_file(item.rarity)));
            append_text(rupee, fmt::format("{}{}", rupee_label(item.rarity),
                item.optional ? " · Opcional" : ""));
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
    if (preview_pending()) {
        return true;
    }
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
