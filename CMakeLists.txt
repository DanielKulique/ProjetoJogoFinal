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
    source/Game.cpp
    source/Player.cpp
    source/Projectile.cpp
    source/Weapon.cpp
    source/Enemy.cpp
    source/WaveManager.cpp
    source/Portal.cpp
    source/GameConfig.cpp
    source/Asteroid.cpp
    source/GravityField.cpp
    source/Pickup.cpp
    source/NCP.cpp
)
target_include_directories(space_arena_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/source")
target_link_libraries(space_arena_core PUBLIC sfml-graphics sfml-window sfml-system)

add_library(space_arena_view source/DungeonVisualizer.cpp)
target_link_libraries(space_arena_view PUBLIC space_arena_core sfml-graphics sfml-window sfml-system)
configure_file(assets/fonts/DejaVuSans.ttf assets/fonts/DejaVuSans.ttf COPYONLY)
configure_file(assets/fonts/DejaVu-LICENSE.txt assets/fonts/DejaVu-LICENSE.txt COPYONLY)
target_compile_definitions(space_arena_view PRIVATE
    SPACE_ARENA_FONT_PATH="${CMAKE_CURRENT_BINARY_DIR}/assets/fonts/DejaVuSans.ttf")

add_executable(space_arena source/Main.cpp)
target_link_libraries(space_arena PRIVATE space_arena_view)

foreach(target space_arena_core space_arena_view space_arena)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()

include(CTest)
if(BUILD_TESTING)
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
