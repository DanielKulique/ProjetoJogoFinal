#pragma once

#include "TileMap.hpp"
#include <cstdint>
#include <string>

enum class ScenarioType { Classic, Procedural, Custom };
struct ScenarioInfo {
    ScenarioType type{ScenarioType::Classic};
    std::uint32_t seed{0};
    std::string csvPath;
};
struct Scenario {
    TileMap map;
    ScenarioInfo info;
    static Scenario create(ScenarioType type, std::uint32_t seed = 0, const std::string& csvPath = {});
    static std::string professorCSVPath();
    static const char* name(ScenarioType type) noexcept;
};
