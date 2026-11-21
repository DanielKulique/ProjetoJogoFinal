#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

// Observacoes de eventos reais do Game; nao mudam regras nem fornecem entrada aos bots.
struct GameStats {
    std::uint64_t shotsFired{0};
    std::uint64_t projectileHits{0};
    std::uint64_t enemyKills{0};
    std::uint64_t playerKills{0};
    std::uint64_t damageDealt{0};
    std::uint64_t asteroidDamageDealt{0};
    std::uint64_t damageReceived{0};
    std::uint64_t enginesLost{0};
    std::uint64_t enginesRecovered{0};
    std::uint64_t upgradesCollected{0};
    std::uint64_t roundsCompleted{0};
    std::size_t peakProjectiles{0};
    std::uint32_t patternMask{0};
    std::array<std::uint64_t, 8> perPatternShots{};
    std::array<std::uint64_t, 8> perPatternHits{};
    std::array<std::uint64_t, 8> perPatternDamage{};
    std::vector<double> roundDurations;
};
