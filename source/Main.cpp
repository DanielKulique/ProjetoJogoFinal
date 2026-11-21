// Seleciona a origem do mapa, compoe os objetos e apresenta erros sem globais.
#include "Dungeon2D.hpp"
#include "DungeonVisualizer.hpp"
#include "Game.hpp"
#include "TileMap.hpp"

#include <charconv>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace {
constexpr const char* usage =
    "Uso: space_arena [--procedural [--seed N] | --csv ARQUIVO] [--game-seed N] [--smoke-test]\n"
    "Padrao: arena livre 31 x 23, sem paredes.\n"
    "--procedural: gerar paredes; semente padrao 1234.\n"
    "--seed: inteiro sem sinal de 32 bits; --smoke-test: desenha 3 quadros e fecha.\n"
    "--game-seed: semente dos aliens, drops e eventos (padrao 1234).\n"
    "WASD: mover; mouse: mirar; botao esquerdo: disparar; R: reiniciar apos derrota.\n"
    "Setas: camera; F: enquadrar mapa; Home: restaurar; Esc: sair.\n";

std::uint32_t readSeed(const std::string& text) {
    std::uint32_t value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size()) {
        throw std::invalid_argument("Semente invalida: " + text);
    }
    return value;
}
}

int main(int argc, char* argv[]) {
    try {
        std::string csvPath;
        std::uint32_t seed = 1234;
        GameConfig config;
        bool procedural = false;
        bool csvSelected = false;
        bool seedSelected = false;
        bool smokeTest = false;
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--help") {
                std::cout << usage;
                return 0;
            }
            if (arg == "--procedural") procedural = true;
            else if (arg == "--smoke-test") smokeTest = true;
            else if (arg == "--csv" && i + 1 < argc) {
                csvSelected = true;
                csvPath = argv[++i];
            } else if (arg == "--game-seed" && i + 1 < argc) {
                config.seed = readSeed(argv[++i]);
            } else if (arg == "--seed" && i + 1 < argc) {
                seedSelected = true;
                seed = readSeed(argv[++i]);
            } else {
                throw std::invalid_argument("Argumento invalido ou sem valor: " + arg);
            }
        }
        if (csvSelected && (procedural || seedSelected)) {
            throw std::invalid_argument("--csv nao pode ser combinado com --procedural ou --seed");
        }

        TileMap map;
        if (csvSelected) {
            std::string error;
            if (!map.loadCSV(csvPath, &error)) throw std::runtime_error(error);
        } else if (procedural || seedSelected) {
            Dungeon2D dungeon(31, 23);
            dungeon.generate(seed);
            map.loadFromDungeon(dungeon); // Copia validada; dungeon pode sair de escopo.
            std::cout << "Arena procedural: semente " << seed << '\n';
        } else {
            map = TileMap(TileMap::Matrix(23, std::string(31, EMPTY_TILE)));
        }
        Game game(std::move(map), config);
        DungeonVisualizer view(game);
        if (!view.init(1024, 768)) throw std::runtime_error("Falha ao criar janela SFML");

        const unsigned frames = view.run(smokeTest ? 3 : 0);
        if (smokeTest) {
            if (frames != 3) throw std::runtime_error("Janela fechada antes de completar 3 quadros");
            std::cout << "Teste grafico: " << frames << " quadros renderizados e janela encerrada\n";
        }
    } catch (const std::exception& error) {
        std::cerr << "ERRO: " << error.what() << '\n' << usage;
        return 1;
    }
    return 0;
}
