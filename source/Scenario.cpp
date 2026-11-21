#include "Scenario.hpp"
#include "Dungeon2D.hpp"
#include <stdexcept>

std::string Scenario::professorCSVPath() { return SPACE_ARENA_DEFAULT_CSV_PATH; }
const char* Scenario::name(ScenarioType type) noexcept {
    switch(type) {
    case ScenarioType::Classic: return "Arena espacial classica";
    case ScenarioType::Procedural: return "Masmorra procedural";
    case ScenarioType::Custom: return "Masmorra personalizada";
    }
    return "Cenario desconhecido";
}
Scenario Scenario::create(ScenarioType type, std::uint32_t seed, const std::string& csvPath) {
    Scenario result;
    result.info.type=type;
    switch(type) {
    case ScenarioType::Classic:
        result.map=TileMap(TileMap::Matrix(23,std::string(31,EMPTY_TILE)));
        break;
    case ScenarioType::Procedural: {
        Dungeon2D dungeon(31,23);
        dungeon.generate(seed);
        result.map.loadFromDungeon(dungeon);
        result.info.seed=seed;
        break;
    }
    case ScenarioType::Custom: {
        std::string error;
        const auto path=csvPath.empty()?professorCSVPath():csvPath;
        if(!result.map.loadCSV(path,&error))throw std::runtime_error(error);
        result.info.csvPath=path;
        break;
    }
    default: throw std::invalid_argument("Tipo de cenario invalido");
    }
    if(result.map.width()>256 || result.map.height()>256)
        throw std::invalid_argument("O cenario deve ter no maximo 256 x 256 celulas para salvar a partida");
    // Mesmo foco inicial de Game. Rejeita somente uma regiao sem espaco para combate.
    int cx=result.map.width()/2,cy=result.map.height()/2;
    if(!result.map.isFree(cx,cy)) {
        bool found=false;
        for(int y=0;y<result.map.height() && !found;++y)for(int x=0;x<result.map.width();++x)
            if(result.map.isFree(x,y)){cx=x;cy=y;found=true;break;}
        if(!found)throw std::invalid_argument("O cenario precisa de piso livre");
    }
    if(!result.map.isFree(cx+1,cy) && !result.map.isFree(cx-1,cy) &&
       !result.map.isFree(cx,cy+1) && !result.map.isFree(cx,cy-1))
        throw std::invalid_argument("A regiao inicial precisa de pelo menos dois pisos conectados para combate");
    return result;
}
