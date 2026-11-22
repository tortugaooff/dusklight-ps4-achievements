#pragma once

#include <cstdint>
#include <bitset>
#include <iterator>

namespace dusk::achievement_poc {

enum class Id : std::uint8_t {
    Slingshot, Goats, Cucco, RescueTalo, MidnaFreesWolf, MeetZelda, FaronVessel,
    RestoreFaron, FishingRod, CoroLantern, RecoverLantern, ForestMap,
    GaleBoomerang, ForestBossKey, DefeatDiababa,
#define CARAPACA_ENTRY(id, name, desc, rarity, optional, rule, value) id,
#include "catalog/Index.inc"
#undef CARAPACA_ENTRY
    Count
};
enum class Rarity : std::uint8_t { Green, Blue, Red, Luminous };
struct Definition {
    Id id;
    const char* key;
    const char* name;
    const char* description;
    Rarity rarity;
    bool optional = false;
};

// Preserve the first three bit positions for the published v1 format.
inline constexpr Definition catalog[] = {
    {Id::Slingshot, "slingshot", "Na mira", "Obtenha o estilingue.", Rarity::Green},
    {Id::Cucco, "cucco_control", "Comando emplumado", "Controle uma galinha.", Rarity::Blue, true},
    {Id::Goats, "goats_under_minute", "De volta ao curral", "Reúna 20 cabras em menos de um minuto.", Rarity::Red, true},
    {Id::RescueTalo, "rescue_talo", "Talo a salvo", "Resgate Talo e a macaquinha em Faron.", Rarity::Green},
    {Id::MidnaFreesWolf, "midna_frees_wolf", "Correntes ao chão", "Liberte-se das correntes com a ajuda de Midna.", Rarity::Green},
    {Id::MeetZelda, "meet_zelda", "Uma princesa nas sombras", "Encontre Zelda no Castelo de Hyrule.", Rarity::Green},
    {Id::FaronVessel, "faron_vessel", "Recipiente da Luz", "Receba o Recipiente da Luz de Faron.", Rarity::Green},
    {Id::RestoreFaron, "restore_faron", "A luz de Faron", "Restaure a luz de Faron.", Rarity::Blue},
    {Id::FishingRod, "fishing_rod", "Linha na água", "Obtenha a vara de pesca.", Rarity::Green},
    {Id::CoroLantern, "coro_lantern", "Uma luz na trilha", "Receba a lanterna de Coro.", Rarity::Green},
    {Id::RecoverLantern, "recover_lantern", "De volta às mãos", "Recupere a lanterna depois da travessia da neblina.", Rarity::Green, true},
    {Id::ForestMap, "forest_map", "O caminho no papel", "Obtenha o mapa do Templo da Floresta.", Rarity::Green, true},
    {Id::GaleBoomerang, "gale_boomerang", "Vento de volta", "Obtenha o Bumerangue do Vento.", Rarity::Blue},
    {Id::ForestBossKey, "forest_boss_key", "A chave da última porta", "Obtenha a chave do chefe do Templo da Floresta.", Rarity::Blue, true},
    {Id::DefeatDiababa, "defeat_diababa", "Raízes arrancadas", "Derrote Diababa.", Rarity::Red},
#define CARAPACA_ENTRY(id, name, desc, rarity, optional, rule, value) \
    {Id::id, #id, name, desc, Rarity::rarity, optional},
#include "catalog/Index.inc"
#undef CARAPACA_ENTRY
};

inline constexpr auto kCatalogCount = static_cast<std::size_t>(Id::Count);
static_assert(std::size(catalog) == kCatalogCount);
using Mask = std::bitset<kCatalogCount>;
inline Mask bit(Id id) { return Mask{}.set(static_cast<std::size_t>(id)); }
inline constexpr unsigned legacyBits = 7;

inline bool mastery_ready(const Mask& unlocked) {
    for (const auto& item : catalog) {
        if (!item.optional && item.id != Id::carapaca_mastery &&
            !unlocked.test(static_cast<std::size_t>(item.id))) return false;
    }
    return true;
}

// Arm only after the mandatory catalog has passed console validation.
inline constexpr bool kDefinitiveCatalogApproved = false;

constexpr bool goatRunCompleted(int target, int entered, int elapsedMs) {
    return target == 20 && entered == target && elapsedMs > 0 && elapsedMs < 60000;
}

class AchievementCore {
public:
    explicit AchievementCore(Mask unlocked = 0) : mUnlocked(unlocked) {}
    Mask unlock(Mask observed) {
        observed &= ~bit(Id::carapaca_mastery);
        if (kDefinitiveCatalogApproved && mastery_ready(mUnlocked | observed)) {
            observed |= bit(Id::carapaca_mastery);
        }
        const Mask newlyUnlocked = observed & ~mUnlocked;
        mUnlocked |= newlyUnlocked;
        return newlyUnlocked;
    }
    Mask unlocked() const { return mUnlocked; }
private:
    Mask mUnlocked;
};
} // namespace dusk::achievement_poc
