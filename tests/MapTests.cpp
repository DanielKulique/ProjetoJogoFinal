// Verifica contratos dos mapas, conectividade, reserva central e integracao com A* sem janela.
#include "AStar.hpp"
#include "Dungeon2D.hpp"
#include "Game.hpp"
#include "TileMap.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <queue>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

template<class Exception, class Function>
void requireThrows(Function function, const std::string& message) {
    try {
        function();
    } catch (const Exception&) {
        return;
    }
    throw std::runtime_error(message);
}

struct TemporaryCSV {
    std::filesystem::path directory;
    TemporaryCSV() {
        std::random_device random;
        for (int attempt = 0; attempt < 100; ++attempt) {
            auto candidate = std::filesystem::temp_directory_path() /
                             ("space-arena-test-" + std::to_string(random()));
            if (std::filesystem::create_directory(candidate)) {
                directory = std::move(candidate);
                return;
            }
        }
        throw std::runtime_error("Nao foi possivel criar pasta temporaria de testes");
    }
    ~TemporaryCSV() {
        std::error_code error;
        std::filesystem::remove_all(directory, error);
    }
    std::string write(const std::string& contents) const {
        const auto path = directory / "map.csv";
        std::ofstream output(path, std::ios::binary);
        output << contents;
        output.close();
        require(static_cast<bool>(output), "Falha ao escrever fixture CSV");
        return path.string();
    }
};

void testCSV(const std::string& sample) {
    TemporaryCSV file;
    TileMap map;
    std::string error = "erro anterior";
    require(map.loadCSV(file.write(" 1 , 0 ,1\r\n0,0,1\r\n"), &error), error);
    require(error.empty(), "Carregamento bem-sucedido deve limpar erro");
    require(map.width() == 3 && map.height() == 2, "Dimensoes CSV incorretas");
    require(map.rows() == TileMap::Matrix{"#.#", "..#"}, "Conversao 0/1 para ./# incorreta");
    const auto before = map.rows();
    const std::vector<std::string> invalid{
        "", "\n", "0,1\n0\n", "0,2\n", "0,,1\n", "0,1,\n", "0,1, \r\n",
        ",0\n", "0,1\n\n", "0,a\n", "0,10\n", "0;1\n", "0,1\n , \n"
    };
    for (const auto& contents : invalid) {
        require(!map.loadCSV(file.write(contents), &error), "CSV invalido foi aceito");
        require(!error.empty(), "CSV invalido precisa explicar o erro");
        require(map.rows() == before, "Falha CSV alterou o mapa anterior");
    }
    require(!map.loadCSV((file.directory / "missing.csv").string(), &error),
            "Arquivo ausente foi aceito");
    require(map.rows() == before, "Arquivo ausente alterou o mapa");
    require(map.loadCSV(file.write("0")), "CSV de uma celula sem quebra final deve funcionar");
    require(map.width() == 1 && map.isFree(0, 0), "Mapa de uma celula incorreto");
    require(map.loadCSV(sample, &error), error);
    require(map.width() > 0 && map.height() > 0, "Mapa distribuido esta vazio");
    Game playable(map);
    require(map.isFreeAtWorld(playable.initialCenter()), "Mapa distribuido nao e utilizavel");
}

void testCoordinatesAndGame() {
    TileMap empty;
    require(empty.width() == 0 && empty.height() == 0 && empty.isWall(0, 0),
            "Contrato do mapa vazio incorreto");
    requireThrows<std::invalid_argument>([] { TileMap map(TileMap::Matrix{}); }, "Matriz vazia aceita");
    requireThrows<std::invalid_argument>([] { TileMap map({"..", "."}); }, "Matriz irregular aceita");
    requireThrows<std::invalid_argument>([] { TileMap map({".x"}); }, "Simbolo invalido aceito");
    TileMap map({"#..", "..."});
    require(map.at(0, 0) == WALL_TILE && map.at(1, 0) == EMPTY_TILE, "Acesso incorreto");
    require(map.isWall(-1, 0) && map.isWall(0, -1) && map.isWall(3, 0) && map.isWall(0, 2),
            "Exterior deve ser parede");
    require(!map.isFree(-1, 0), "Exterior nao pode ser livre");
    requireThrows<std::out_of_range>([&] { (void)map.at(3, 0); }, "at() sem limite");
    requireThrows<std::out_of_range>([&] { (void)map.cellCenter(-1, 0); }, "Centro fora da matriz aceito");
    require(map.cellCenter(1, 0).equals({96, 32}), "TILE_SIZE ou centro em pixels incorreto");
    require(!map.isFreeAtWorld({63.99f, 32}) && map.isFreeAtWorld({64, 32}),
            "Conversao na borda da celula incorreta");
    require(!map.isFreeAtWorld({192, 32}) && !map.isFreeAtWorld({64, 128}) &&
            !map.isFreeAtWorld({-0.1f, 32}), "Limites mundiais incorretos");
    require(!map.isFreeAtWorld({std::numeric_limits<float>::infinity(), 0}) &&
            !map.isFreeAtWorld({0, std::numeric_limits<float>::quiet_NaN()}), "Nao finitos aceitos");
    Game center(map);
    require(center.initialCenter().equals(map.cellCenter(1, 1)), "Game nao priorizou centro livre");
    Game fallback(TileMap({".##", "###", "###"}));
    require(fallback.initialCenter().equals({32, 32}), "Game nao selecionou primeiro piso");
    requireThrows<std::invalid_argument>([] { Game game(TileMap({"##"})); }, "Mapa sem piso aceito");
    requireThrows<std::invalid_argument>([] { Game game{TileMap{}}; }, "Game aceitou mapa vazio");

    const auto path = findPathAStar(map.rows(), {1, 0}, {2, 1});
    require(path.size() == 3 && path.front().equals({1, 0}) && path.back().equals({2, 1}),
            "Representacao comum nao integra com A*");
}

void verifyDungeon(const TileMap& map) {
    const int cx = map.width() / 2;
    const int cy = map.height() / 2;
    for (int y = cy - 1; y <= cy + 1; ++y) {
        for (int x = cx - 1; x <= cx + 1; ++x) {
            require(map.isFree(x, y), "Reserva central de 3 x 3 foi bloqueada");
        }
    }
    int floors = 0;
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            if (x == 0 || y == 0 || x == map.width() - 1 || y == map.height() - 1) {
                require(map.isWall(x, y), "Gerador abriu o contorno");
            }
            floors += map.isFree(x, y) ? 1 : 0;
        }
    }
    std::vector<std::vector<bool>> seen(map.height(), std::vector<bool>(map.width(), false));
    std::queue<std::pair<int, int>> queue;
    queue.emplace(cx, cy);
    seen[cy][cx] = true;
    int reached = 0;
    constexpr int offsets[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    while (!queue.empty()) {
        const auto [x, y] = queue.front();
        queue.pop();
        ++reached;
        for (const auto& offset : offsets) {
            const int nx = x + offset[0];
            const int ny = y + offset[1];
            if (map.isFree(nx, ny) && !seen[ny][nx]) {
                seen[ny][nx] = true;
                queue.emplace(nx, ny);
            }
        }
    }
    require(reached == floors, "Existem pisos desconectados do centro");
}

void testDungeons() {
    requireThrows<std::invalid_argument>([] { Dungeon2D dungeon(6, 7); }, "Dimensao minima nao validada");
    requireThrows<std::invalid_argument>([] { Dungeon2D dungeon(7, -1); }, "Dimensao negativa aceita");
    Dungeon2D notGenerated(7, 7);
    TileMap map({"."});
    requireThrows<std::invalid_argument>([&] { map.loadFromDungeon(notGenerated); }, "Dungeon nao gerada aceita");
    require(map.rows() == TileMap::Matrix{"."}, "Falha procedural alterou mapa anterior");

    const std::vector<std::pair<int, int>> dimensions{{7, 7}, {8, 8}, {7, 12}, {12, 7}, {31, 23}, {40, 30}};
    for (const auto& [width, height] : dimensions) {
        Dungeon2D dungeon(width, height);
        for (std::uint32_t seed = 0; seed < 100; ++seed) {
            dungeon.generate(seed);
            map.loadFromDungeon(dungeon);
            require(map.width() == width && map.height() == height, "Dimensoes procedurais incorretas");
            verifyDungeon(map);
            dungeon.generate(seed);
            require(map.rows() == dungeon.rows(), "Mesma semente nao reproduziu o mapa");
        }
    }
    Dungeon2D first(31, 23), second(31, 23);
    first.generate(1234);
    second.generate(5678);
    require(first.rows() != second.rows(), "Sementes de exemplo produziram mapas identicos");
    first.generate(std::numeric_limits<std::uint32_t>::max());
    map.loadFromDungeon(first);
    verifyDungeon(map);
}
}

int main(int argc, char* argv[]) {
    try {
        require(argc == 2, "Informe o caminho do CSV distribuido");
        testCSV(argv[1]);
        testCoordinatesAndGame();
        testDungeons();
        std::cout << "CSV, coordenadas, Game, A* e 601 cenarios procedurais: OK\n";
    } catch (const std::exception& error) {
        std::cerr << "FALHA: " << error.what() << '\n';
        return 1;
    }
}
