#pragma once

#include <cstdint>
#include <iterator>

namespace dusk::achievement_poc {

enum class Id : std::uint8_t {
    Slingshot, Goats, Cucco, RescueTalo, MidnaFreesWolf, MeetZelda, FaronVessel,
    RestoreFaron, FishingRod, CoroLantern, RecoverLantern, ForestMap,
    GaleBoomerang, ForestBossKey, DefeatDiababa
};
enum class Rarity : std::uint8_t { Green, Blue, Red };
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
};

using Mask = std::uint64_t;
constexpr Mask bit(Id id) { return Mask{1} << static_cast<unsigned>(id); }
inline constexpr Mask legacyBits = bit(Id::Slingshot) | bit(Id::Goats) | bit(Id::Cucco);
inline constexpr Mask allBits = [] {
    Mask result = 0;
    for (const auto& item : catalog) result |= bit(item.id);
    return result;
}();

constexpr bool goatRunCompleted(int target, int entered, int elapsedMs) {
    return target == 20 && entered == target && elapsedMs > 0 && elapsedMs < 60000;
}

class AchievementCore {
public:
    explicit AchievementCore(Mask unlocked = 0) : mUnlocked(unlocked) {}
    Mask unlock(Mask observed) {
        const Mask newlyUnlocked = observed & allBits & ~mUnlocked;
        mUnlocked |= newlyUnlocked;
        return newlyUnlocked;
    }
    Mask unlocked() const { return mUnlocked; }
private:
    Mask mUnlocked;
};
} // namespace dusk::achievement_poc
