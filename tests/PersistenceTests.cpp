#include "SaveManager.hpp"
#include "Menu.hpp"
#include "LocalStorage.hpp"
#include "Simulation.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <random>

struct GameTestAccess {
    static void settle(Game& g){while(g.m_waves.pending()){g.m_waves.update(100);g.m_waves.confirmSpawn();}g.m_enemies.clear();}
    static void target(Game& g,EnemyKind kind){auto e=std::make_unique<Enemy>(g.player().body.position+Vector2D{100,0},EnemyConfig{},kind,8);e->takeDamage(e->health()-1);g.m_enemies.push_back(std::move(e));g.m_projectiles.emplace_back(g.player().body.position,Vector2D{1,0},12000,50,2);}
    static void rock(Game& g,int depth){Asteroid a(g.player().body.position+Vector2D{100,0},{1,0},{},depth);a.takeDamage(a.health()-1);g.m_asteroids.push_back(std::move(a));g.m_projectiles.emplace_back(g.player().body.position,Vector2D{1,0},12000,50,2);}
    static void environment(Game& g){auto c=g.player().body.position;g.m_asteroids.emplace_back(c+Vector2D{300,200},Vector2D{0,1});g.m_gravityField.emplace(c+Vector2D{-250,0});g.m_pickups.emplace_back(c+Vector2D{120,-100},PickupKind::WeaponUpgrade,PickupConfig{},WeaponUpgrade{UpgradeKind::Damage,1,1});}
    static void environmentalKill(Game& g){auto c=g.player().body.position+Vector2D{250,0};g.m_enemies.push_back(std::make_unique<Enemy>(c,EnemyConfig{},EnemyKind::Advanced,99));g.m_asteroids.emplace_back(c,Vector2D{1,0});}
    static void lose(Game& g){g.m_player.takeDamage(4);}
};
namespace {
void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
TileMap arena(){return TileMap(TileMap::Matrix(23,std::string(31,'.')));}
GameConfig config(){auto c=GameConfig::recommended();c.seed=4242;c.reseedOnRestart=false;return c;}
}
int main(){
    auto dir=std::filesystem::temp_directory_path()/("space-arena-persistence-"+std::to_string(std::random_device{}()));
    try {
        std::filesystem::create_directories(dir);SaveManager saves(dir);std::string error;
        require(!saves.valid() && !saves.load(error),"Save ausente");
        Game game(arena(),config());SimulatedAgent bot(AgentConfig::forProfile(SkillProfile::Advanced),55);
        for(int i=0;i<900;++i)game.update(1.0f/60,bot.input(game,1.0f/60));
        GameTestAccess::environment(game);
        game.player().weapon().applyUpgrade({UpgradeKind::Burst,2,1});
        game.update(1.0f/60,{{},{1200,700},true}); // Rajada em andamento.
        auto payload=SaveManager::snapshot(game);
        require(saves.save(game,error),"Salvar estado complexo");auto loaded=saves.load(error);require(bool(loaded),error.c_str());
        require(SaveManager::snapshot(*loaded)==payload,"Roundtrip exato de todos os campos");
        for(int i=0;i<300;++i){GameInput input{{i%90<45?1.0f:-1.0f,.5f},{1100,600},i%3!=0};game.update(1.0f/60,input);loaded->update(1.0f/60,input);require(SaveManager::snapshot(game)==SaveManager::snapshot(*loaded),"Continuidade exata e RNG apos carga");}
        Game invalid(arena(),config());invalid.player().takeDamage(4);
        require(!saves.save(invalid,error) && saves.valid(),"Falha antes da escrita preserva save anterior");
        // Escrita atomica substitui uma versao valida; temporario nao promove estado incompleto.
        std::ofstream(dir/"session.sav.tmp.aborted")<<"incompleto";require(saves.valid(),"Temporario nao interfere no save");
        std::ifstream in(dir/"session.sav",std::ios::binary);std::string bytes((std::istreambuf_iterator<char>(in)),{});bytes.back()^=1;std::ofstream(dir/"session.sav",std::ios::binary)<<bytes;
        require(!saves.valid() && !saves.load(error),"Checksum rejeita corrupcao");
        LocalStorage::write(dir/"session.sav","SPACE_ARENA_SAVE","99999");require(!saves.load(error),"Payload invalido com checksum correto");
        require(saves.save(game,error),"Save anterior preservado para confirmacao");
        Menu menu(game,dir);require(menu.screen()==Screen::MainMenu && menu.canContinue(),"Menu inicial e continuar");
        menu.activate();require(menu.screen()==Screen::ScenarioSelection,"Escolha de cenario");menu.activate();require(menu.screen()==Screen::ConfirmNewGame,"Confirmar substituicao");menu.select(1);menu.activate();require(saves.valid(),"Cancelar preserva save");menu.escape();
        menu.select(1);menu.activate();require(menu.screen()==Screen::Playing,"Continuar restaura jogo");menu.escape();require(menu.screen()==Screen::Paused,"Esc pausa");
        const auto before=SaveManager::snapshot(game);for(int i=0;i<120;++i)menu.update(1.0f/60,{{1,0},{1,1},true});require(SaveManager::snapshot(game)==before,"Pausa congela fisica cooldowns RNG relogio");
        menu.select(2);menu.activate();require(menu.screen()==Screen::Controls,"Controles da pausa");menu.escape();require(menu.screen()==Screen::Paused,"Volta a pausa");menu.select(3);menu.activate();require(menu.screen()==Screen::Mechanics,"Mecanicas");menu.escape();menu.select(1);menu.activate();require(saves.valid(),"Salvar manual");menu.select(4);menu.activate();menu.select(0);menu.activate();menu.activate();menu.activate();require(game.player().engines()==4 && game.player().weapon().acquiredCount()==0 && game.waves().round()==1 && game.gameplayTime()==0,"Nova partida limpa estado");require(!saves.valid(),"Substituicao invalida save antigo");
        GameTestAccess::settle(game);GameTestAccess::target(game,EnemyKind::Normal);menu.update(1.0f/60,{});require(game.score()==config().scoring.normal+config().scoring.round,"Pontos tiro normal e round");const auto score=game.score();menu.update(1.0f/60,{});require(game.score()==score,"Sem pontos duplicados");require(saves.valid(),"Autosave round");
        auto check=saves.load(error);require(check && check->score()==score && check->portal().active,"Autosave inclui score e portal");
        GameTestAccess::lose(game);menu.update(1.0f/60,{});require(menu.screen()==Screen::GameOver && !saves.valid(),"Derrota invalida save");menu.activate();menu.activate();require(menu.scores().size()==1,"Registro apenas uma vez");menu.restartAfterDefeat();require(menu.screen()==Screen::ScenarioSelection,"R oferece modos");menu.activate();require(menu.screen()==Screen::Playing && game.score()==0,"R reinicia");
        ScoreManager scores(dir/"ranking");for(int i=0;i<15;++i)require(scores.record({"Piloto "+std::to_string(i),static_cast<std::uint64_t>(i*100),i+1,15.5},error),"Ranking persiste");auto list=scores.load(error);require(list.size()==10 && list.front().score==1400 && list.back().score==500,"Top 10 ordenado");
        const auto rankingFile=dir/"ranking"/"scores.dat";
        std::ifstream rankInput(rankingFile,std::ios::binary);std::string rankBytes((std::istreambuf_iterator<char>(rankInput)),{});rankBytes.back()^=1;
        std::ofstream(rankingFile,std::ios::binary)<<rankBytes;
        require(scores.load(error).empty() && !error.empty(),"Ranking corrompido tratado");
        require(!scores.record({"Novo",9999,2,30},error),"Ranking invalido nao e sobrescrito silenciosamente");
        std::ifstream rankAfter(rankingFile,std::ios::binary);require(std::string((std::istreambuf_iterator<char>(rankAfter)),{})==rankBytes,"Bytes do ranking preservados apos erro");
        Game rockGame(arena(),config());GameTestAccess::rock(rockGame,0);rockGame.update(1.0f/60,{});require(rockGame.score()==config().scoring.asteroid,"Pontos asteroid por jogador");
        Game fragmentGame(arena(),config());GameTestAccess::rock(fragmentGame,1);fragmentGame.update(.35f,{});
        require(fragmentGame.score()==config().scoring.fragment,"Pontos por fragmento destruido pelo jogador");
        Game advancedGame(arena(),config());GameTestAccess::settle(advancedGame);GameTestAccess::target(advancedGame,EnemyKind::Advanced);advancedGame.update(1.0f/60,{});
        require(advancedGame.score()==config().scoring.advanced+config().scoring.round,"Pontos por avancado");
        Game environmental(arena(),config());GameTestAccess::settle(environmental);GameTestAccess::environmentalKill(environmental);environmental.update(1.0f/60,{});
        require(environmental.score()==config().scoring.round,"Colisoes ambientais nao pontuam combate");
        require(saves.invalidate(error),"Invalidar save");
        std::filesystem::remove_all(dir);std::cout<<"Persistencia, RNG, menus, pausa, ranking e score: OK\n";
    }catch(const std::exception& e){std::filesystem::remove_all(dir);std::cerr<<"FALHA: "<<e.what()<<'\n';return 1;}
}
