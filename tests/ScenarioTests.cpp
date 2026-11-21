#include "Scenario.hpp"
#include "Dungeon2D.hpp"
#include "Game.hpp"
#include "Menu.hpp"
#include "SaveManager.hpp"
#include "LocalStorage.hpp"
#include "AStar.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
TileMap arena(){return Scenario::create(ScenarioType::Classic).map;}
GameInput controller(const Game& game) {
    Vector2D target=game.portal().body.position;
    if(game.state()==GameState::Playing && !game.enemies().empty())target=game.enemies().front()->body.position;
    if(game.state()==GameState::Playing && game.enemies().empty())target=game.player().body.position;
    if(game.state()==GameState::RoundCompleted && game.portal().waitingForExit())target=game.portal().body.position+Vector2D{128,0};
    const auto p=game.player().body.position;
    Vector2D movement;
    const Vector2D start{std::floor(p.x/TILE_SIZE),std::floor(p.y/TILE_SIZE)};
    const Vector2D finish{std::floor(target.x/TILE_SIZE),std::floor(target.y/TILE_SIZE)};
    const auto path=findPathAStar(game.map().rows(),start,finish);
    if(path.size()>1){
        const auto center=game.map().cellCenter(static_cast<int>(start.x),static_cast<int>(start.y));
        const auto next=game.map().cellCenter(static_cast<int>(path[1].x),static_cast<int>(path[1].y));
        if(next.x!=center.x){if(std::abs(p.y-center.y)>3)movement.y=center.y-p.y;else movement.x=next.x-p.x;}
        else {if(std::abs(p.x-center.x)>3)movement.x=center.x-p.x;else movement.y=next.y-p.y;}
    }else if(game.state()==GameState::RoundCompleted && (target-p).length()>3)movement=target-p;
    return {movement,target,true};
}
std::string legacyPayload(const std::string& data){std::istringstream in(data);unsigned kind,seed;std::string path;in>>kind>>seed>>std::quoted(path);return std::string((std::istreambuf_iterator<char>(in)),{});}
}
int main(){
    auto folder=std::filesystem::temp_directory_path()/("scenario-tests-"+std::to_string(std::random_device{}()));
    try{
        std::filesystem::create_directories(folder);
        auto classic=Scenario::create(ScenarioType::Classic);
        for(const auto& row:classic.map.rows())require(row.find('#')==std::string::npos,"Arena classica livre");
        Dungeon2D original(31,23);original.generate(77);
        auto generated=Scenario::create(ScenarioType::Procedural,77);
        require(generated.map.rows()==original.rows(),"Reutiliza o gerador existente");
        require(Scenario::create(ScenarioType::Procedural,78).map.rows()!=generated.map.rows(),"Semente altera masmorra");
        auto custom=Scenario::create(ScenarioType::Custom);
        TileMap expected;std::string error;require(expected.loadCSV(Scenario::professorCSVPath(),&error),"CSV fornecido disponivel");
        require(custom.map.rows()==expected.rows(),"CSV nao e alterado");
        auto ownPath=folder/"mapa com espacos.csv";std::filesystem::copy_file(Scenario::professorCSVPath(),ownPath);
        auto own=Scenario::create(ScenarioType::Custom,0,ownPath.string());require(own.map.rows()==custom.map.rows(),"Caminho CSV proprio");
        for(auto type:{ScenarioType::Classic,ScenarioType::Procedural,ScenarioType::Custom}){
            auto scenario=Scenario::create(type,77, type==ScenarioType::Custom?ownPath.string():"");
            const auto matrix=scenario.map.rows();
            auto config=GameConfig::recommended();config.seed=13;
            config.weapon.damage=100;
            config.waves.initialEnemies=2;config.waves.additionalEnemies=1;config.waves.maxEnemies=5;
            config.waves.spawnInterval=.05f;config.waves.minSpawnInterval=.05f;config.waves.spawnIntervalDecrease=0;
            config.waves.enemy.speed=30;config.waves.healthPerRound=0;config.waves.speedPerRound=0;config.waves.advancedChance=1;
            config.drops.chance=1;config.drops.upgradeChance=1;
            config.asteroid.spawnInterval=.1f;config.asteroid.spawnChance=1;config.asteroid.maxActive=1;config.asteroid.speed=10;config.asteroid.lifetime=.8f;
            config.gravity.spawnInterval=.1f;config.gravity.spawnChance=1;config.gravity.accelerationMeters=.1f;config.gravity.duration=1000;
            Game game(std::move(scenario.map),config,scenario.info);
            bool rock=false,field=false,drop=false,portal=false;
            for(int i=0;i<14400 && game.waves().round()<3;++i){
                game.update(1.0f/60,controller(game));
                rock=rock || !game.asteroids().empty();field=field || bool(game.gravityField());drop=drop || !game.pickups().empty();portal=portal || game.portal().active;
                require(game.state()!=GameState::GameOver,"Robo de integracao sobrevive ao combate");
                require(game.map().isAreaFree(game.player().bounds()),"Nave respeita paredes");
                for(const auto& enemy:game.enemies())require(game.map().isAreaFree(enemy->bounds()),"Alien respeita paredes");
            }
            require(game.waves().round()==3 && game.stats().playerKills>=5,"Combate e dois portais em cada modo");
            require(rock && field && drop && portal,"Ambiente, drop e portal funcionam em cada modo");
            require(game.map().rows()==matrix,"Rounds preservam a matriz");
            SaveManager save(folder/std::to_string(static_cast<int>(type)));require(save.save(game,error),error.c_str());
            if(type==ScenarioType::Custom)std::filesystem::remove(ownPath);
            auto loaded=save.load(error);require(bool(loaded),error.c_str());
            require(loaded->scenario().type==type && loaded->map().rows()==matrix && loaded->scenario().seed==scenario.info.seed && loaded->scenario().csvPath==scenario.info.csvPath,"Persistencia do tipo, matriz e metadados");
            require(SaveManager::snapshot(*loaded)==SaveManager::snapshot(game),"Roundtrip exato");
            for(int i=0;i<120;++i){const auto input=controller(game);game.update(1.0f/60,input);loaded->update(1.0f/60,input);require(SaveManager::snapshot(*loaded)==SaveManager::snapshot(game),"RNG e mecanicas continuam identicos apos carga");}
            const auto legacy=legacyPayload(SaveManager::snapshot(game));LocalStorage::write(save.directory()/"session.sav","SPACE_ARENA_SAVE",legacy,1);
            auto migrated=save.load(error);require(migrated && migrated->map().rows()==matrix,"Leitura de saves v1 preserva a matriz");
            std::cout<<Scenario::name(type)<<": combate, drops, ambiente, rounds e save: OK\n";
        }
        Game game(arena(),GameConfig::recommended());Menu menu(game,folder/"menu");
        menu.activate();require(menu.screen()==Screen::ScenarioSelection,"Novo jogo oferece cenarios");menu.select(1);menu.activate();require(menu.screen()==Screen::Playing && game.scenario().type==ScenarioType::Procedural,"Escolha procedural");
        menu.escape();menu.select(1);menu.activate();require(menu.canContinue(),"Save do modo escolhido");menu.select(4);menu.activate();
        const auto before=SaveManager::snapshot(game);menu.select(0);menu.activate();menu.select(2);menu.activate();require(menu.screen()==Screen::CustomScenario,"Escolha de CSV");
        menu.csvPath().clear();menu.select(1);menu.activate();require(menu.screen()==Screen::CustomScenario && !menu.message().empty(),"Caminho vazio pede preenchimento");
        menu.csvPath()=(folder/"ausente.csv").string();menu.select(1);menu.activate();require(menu.screen()==Screen::CustomScenario && !menu.message().empty() && menu.canContinue() && SaveManager::snapshot(game)==before,"CSV invalido preserva partida e save");
        menu.select(0);menu.activate();require(menu.screen()==Screen::ConfirmNewGame,"CSV valido confirma substituicao");menu.select(1);menu.activate();require(menu.canContinue() && SaveManager::snapshot(game)==before,"Cancelar troca preserva partida");
        std::ofstream(folder/"isolado.csv")<<"1,1,1\n1,0,1\n1,1,1\n";
        bool rejected=false;try{(void)Scenario::create(ScenarioType::Custom,0,(folder/"isolado.csv").string());}catch(const std::exception&){rejected=true;}require(rejected,"CSV sem espaco de combate rejeitado");
        std::filesystem::remove_all(folder);
    }catch(const std::exception& e){std::filesystem::remove_all(folder);std::cerr<<"FALHA: "<<e.what()<<'\n';return 1;}
}
