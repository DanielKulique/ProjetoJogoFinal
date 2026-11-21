# Compila os componentes reutilizaveis, o visualizador SFML e os testes de logica.
cmake_minimum_required(VERSION 3.16)
project(SpaceArena VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SFML 2.6.1 EXACT COMPONENTS graphics window system REQUIRED)

add_library(space_arena_core
    source/Vector2D.cpp
    source/Transform2D.cpp
    source/RigidBody2D.cpp
    source/Collision2D.cpp
    source/Entity.cpp
    source/BeheviorTree.cpp
    source/AStar.cpp
    source/TileMap.cpp
    source/Dungeon2D.cpp
    source/Scenario.cpp
    source/Game.cpp
    source/LocalStorage.cpp
    source/SaveManager.cpp
    source/ScoreManager.cpp
    source/Menu.cpp
    source/Player.cpp
    source/Projectile.cpp
    source/Weapon.cpp
    source/WeaponUpgrade.cpp
    source/UpgradeManager.cpp
    source/ProjectilePattern.cpp
    source/Enemy.cpp
    source/WaveManager.cpp
    source/Portal.cpp
    source/GameConfig.cpp
    source/Asteroid.cpp
    source/GravityField.cpp
    source/Pickup.cpp
    source/NCP.cpp
)
configure_file(assets/maps/maze.csv assets/maps/maze.csv COPYONLY)
target_compile_definitions(space_arena_core PRIVATE
    SPACE_ARENA_DEFAULT_CSV_PATH="${CMAKE_CURRENT_BINARY_DIR}/assets/maps/maze.csv")
target_include_directories(space_arena_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/source")
target_link_libraries(space_arena_core PUBLIC sfml-graphics sfml-window sfml-system)

add_library(space_arena_view source/DungeonVisualizer.cpp source/Button.cpp)
target_link_libraries(space_arena_view PUBLIC space_arena_core sfml-graphics sfml-window sfml-system)
configure_file(assets/fonts/DejaVuSans.ttf assets/fonts/DejaVuSans.ttf COPYONLY)
configure_file(assets/fonts/DejaVu-LICENSE.txt assets/fonts/DejaVu-LICENSE.txt COPYONLY)
target_compile_definitions(space_arena_view PRIVATE
    SPACE_ARENA_FONT_PATH="${CMAKE_CURRENT_BINARY_DIR}/assets/fonts/DejaVuSans.ttf")

add_executable(space_arena source/Main.cpp)
target_link_libraries(space_arena PRIVATE space_arena_view)

add_library(space_arena_simulation source/Simulation.cpp)
target_link_libraries(space_arena_simulation PUBLIC space_arena_core)
add_executable(space_arena_sim source/SimulationMain.cpp)
target_link_libraries(space_arena_sim PRIVATE space_arena_simulation)

foreach(target space_arena_core space_arena_view space_arena space_arena_simulation space_arena_sim)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()

include(CTest)
if(BUILD_TESTING)
    add_executable(scenario_tests tests/ScenarioTests.cpp)
    target_link_libraries(scenario_tests PRIVATE space_arena_core)
    add_test(NAME scenario_integration COMMAND scenario_tests)
    add_executable(persistence_tests tests/PersistenceTests.cpp)
    target_link_libraries(persistence_tests PRIVATE space_arena_simulation)
    add_test(NAME persistence_contracts COMMAND persistence_tests)
    add_executable(procedural_weapon_tests tests/ProceduralWeaponTests.cpp)
    target_link_libraries(procedural_weapon_tests PRIVATE space_arena_core)
    add_test(NAME procedural_weapon_contracts COMMAND procedural_weapon_tests)
    add_executable(simulation_tests tests/SimulationTests.cpp)
    target_link_libraries(simulation_tests PRIVATE space_arena_simulation)
    add_test(NAME simulation_contracts COMMAND simulation_tests)
    add_test(NAME sim_cli_help COMMAND space_arena_sim --help)
    add_test(NAME sim_cli_invalid_profile COMMAND space_arena_sim --profile invalid)
    add_test(NAME sim_cli_invalid_label COMMAND space_arena_sim --preset all --label invalid_scope)
    set_tests_properties(sim_cli_invalid_profile sim_cli_invalid_label PROPERTIES WILL_FAIL TRUE)
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_test(NAME balancing_analysis_contracts COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/AnalysisTests.py")
        add_test(NAME simulation_cli_contracts COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/SimulationCLITests.py" $<TARGET_FILE:space_arena_sim>)
    endif()
    add_executable(mechanics_tests tests/MechanicsTests.cpp)
    target_link_libraries(mechanics_tests PRIVATE space_arena_core)
    add_test(NAME mechanics_contracts COMMAND mechanics_tests)
    add_executable(gameplay_tests tests/GameplayTests.cpp)
    target_link_libraries(gameplay_tests PRIVATE space_arena_core)
    add_test(NAME gameplay_contracts COMMAND gameplay_tests)
    add_executable(map_tests tests/MapTests.cpp)
    target_link_libraries(map_tests PRIVATE space_arena_core)
    add_test(NAME map_contracts COMMAND map_tests "${CMAKE_CURRENT_SOURCE_DIR}/assets/maps/maze.csv")
    add_test(NAME cli_help COMMAND space_arena --help)
    add_test(NAME cli_invalid_seed COMMAND space_arena --seed -1)
    add_test(NAME cli_invalid_game_seed COMMAND space_arena --game-seed -1)
    add_test(NAME cli_conflicting_modes COMMAND space_arena --csv unused.csv --seed 1)
    add_test(NAME cli_missing_csv COMMAND space_arena --csv "${CMAKE_CURRENT_BINARY_DIR}/absent-map.csv")
    set_tests_properties(cli_invalid_seed cli_invalid_game_seed cli_conflicting_modes cli_missing_csv PROPERTIES WILL_FAIL TRUE)
endif()
