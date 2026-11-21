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
#include <random>

namespace {
constexpr const char* usage =
    "Uso: space_arena [--procedural [--seed N] | --csv ARQUIVO] [--game-seed N] [--data-dir PASTA] [--smoke-test]\n"
    "Padrao: arena livre 31 x 23, sem paredes.\n"
    "--procedural: gerar paredes; semente padrao 1234.\n"
    "--seed: inteiro sem sinal de 32 bits; --smoke-test: desenha 3 quadros e fecha.\n"
    "--game-seed: semente inicial do nucleo; Novo jogo sorteia outra.\n"
    "WASD: mover; mouse: mirar; botao esquerdo: disparar; R: reiniciar apos derrota.\n"
    "Setas: camera; F: enquadrar mapa; Home: restaurar; Esc: pausa / voltar.\n";

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
        auto dataDirectory=SaveManager::defaultDirectory();
        std::uint32_t seed = 1234;
        GameConfig config = GameConfig::recommended();
        bool gameSeedSelected = false;
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
            else if (arg == "--data-dir" && i + 1 < argc) dataDirectory=argv[++i];
            else if (arg == "--csv" && i + 1 < argc) {
                csvSelected = true;
                csvPath = argv[++i];
            } else if (arg == "--game-seed" && i + 1 < argc) {
                gameSeedSelected = true;
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

        Scenario scenario;
        if(csvSelected) scenario=Scenario::create(ScenarioType::Custom,0,csvPath);
        else if(procedural || seedSelected) {
            scenario=Scenario::create(ScenarioType::Procedural,seed);
            std::cout << "Masmorra procedural: semente " << seed << '\n';
        } else scenario=Scenario::create(ScenarioType::Classic);
        config.reseedOnRestart = !gameSeedSelected;
        if (!gameSeedSelected) config.seed = std::random_device{}();
        std::cout << "Semente da partida: " << config.seed << '\n';
        Game game(std::move(scenario.map), config, std::move(scenario.info));
        DungeonVisualizer view(game,dataDirectory);
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
