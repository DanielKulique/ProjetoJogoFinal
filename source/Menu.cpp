#include "Menu.hpp"
#include <algorithm>
Menu::Menu(Game& game,std::filesystem::path directory):m_game(game),m_saves(directory),m_scores(std::move(directory)){m_csvPath=m_game.scenario().csvPath.empty()?Scenario::professorCSVPath():m_game.scenario().csvPath;refreshSave();}
void Menu::refreshSave(){m_canContinue=m_saves.valid();}
void Menu::change(Screen screen){if(screen==Screen::MainMenu)refreshSave();m_screen=screen;m_selected=0;}
std::vector<std::string> Menu::choices() const {
    switch(m_screen){
    case Screen::MainMenu:return {"NOVO JOGO","CONTINUAR JOGO","SCORES","CONTROLES","MECÂNICAS","SAIR"};
    case Screen::ScenarioSelection:return {"ARENA ESPACIAL CLASSICA","MASMORRA PROCEDURAL","MASMORRA PERSONALIZADA","VOLTAR"};
    case Screen::CustomScenario:return {"USAR MAZE DO PROFESSOR","CARREGAR CAMINHO DIGITADO","VOLTAR"};
    case Screen::Paused:return {"RETOMAR PARTIDA","SALVAR PARTIDA","CONTROLES","MECÂNICAS","MENU PRINCIPAL","SAIR DO JOGO"};
    case Screen::ConfirmNewGame:return {"SUBSTITUIR E INICIAR","CANCELAR"};
    case Screen::GameOver:return {m_recorded?"RESULTADO REGISTRADO":"REGISTRAR RESULTADO","NOVO JOGO (R)","MENU PRINCIPAL"};
    case Screen::Mechanics:return {"ANTERIOR","PROXIMA","VOLTAR"};
    case Screen::Controls:case Screen::Scores:return {"VOLTAR"};
    default:return {};
    }
}
bool Menu::enabled(std::size_t index)const{return !(m_screen==Screen::MainMenu && index==1 && !m_canContinue) && !(m_screen==Screen::GameOver && index==0 && m_recorded);}
void Menu::select(int index){auto c=choices();if(index>=0 && index<static_cast<int>(c.size()) && enabled(static_cast<std::size_t>(index)))m_selected=index;}
void Menu::navigate(int delta){auto n=static_cast<int>(choices().size());if(!n)return;for(int i=0;i<n;++i){m_selected=(m_selected+delta+n)%n;if(enabled(static_cast<std::size_t>(m_selected)))break;}}
void Menu::prepareGame(ScenarioType type,const std::string& csvPath) {
    try {
        auto scenario=Scenario::create(type,std::random_device{}(),csvPath);
        auto config=m_game.config();
        config.seed=std::random_device{}();
        auto candidate=std::make_unique<Game>(std::move(scenario.map),config,std::move(scenario.info));
        m_candidate=std::move(candidate);
        refreshSave();m_message.clear();
        if(m_canContinue)change(Screen::ConfirmNewGame);
        else newGame();
    } catch(const std::exception& error) {m_message="Nao foi possivel iniciar: "+std::string(error.what());}
}
void Menu::newGame() {
    if(!m_candidate)return;
    std::string error;
    if(!m_saves.invalidate(error)){m_message="Falha ao substituir: "+error;return;}
    m_game=std::move(*m_candidate);m_candidate.reset();
    m_recorded=false;m_canContinue=false;m_message.clear();change(Screen::Playing);
}
void Menu::activate(){
    const int choice=m_selected;if(!enabled(static_cast<std::size_t>(choice)))return;
    switch(m_screen){
    case Screen::MainMenu:
        if(choice==0){m_candidate.reset();m_message.clear();change(Screen::ScenarioSelection);}
        else if(choice==1){auto loaded=m_saves.load(m_message);if(loaded){m_game=std::move(*loaded);m_recorded=false;change(Screen::Playing);m_message.clear();}else{m_canContinue=false;m_message="Salvamento indisponivel: "+m_message;}}
        else if(choice==2)change(Screen::Scores);
        else if(choice==3 || choice==4){m_return=Screen::MainMenu;change(choice==3?Screen::Controls:Screen::Mechanics);}
        else m_quitting=true;
        break;
    case Screen::ScenarioSelection:
        if(choice==0)prepareGame(ScenarioType::Classic);
        else if(choice==1)prepareGame(ScenarioType::Procedural);
        else if(choice==2)change(Screen::CustomScenario);
        else change(Screen::MainMenu);
        break;
    case Screen::CustomScenario:
        if(choice==0)prepareGame(ScenarioType::Custom,Scenario::professorCSVPath());
        else if(choice==1){if(m_csvPath.empty())m_message="Digite o caminho de um arquivo CSV.";else prepareGame(ScenarioType::Custom,m_csvPath);}
        else change(Screen::ScenarioSelection);
        break;
    case Screen::ConfirmNewGame:if(choice==0)newGame();else{m_candidate.reset();change(Screen::ScenarioSelection);}break;
    case Screen::Paused:
        if(choice==0)change(Screen::Playing);
        else if(choice==1){std::string error;if(m_saves.save(m_game,error)){m_canContinue=true;m_message="Partida salva com seguranca.";}else m_message="Falha ao salvar: "+error;}
        else if(choice==2 || choice==3){m_return=Screen::Paused;change(choice==2?Screen::Controls:Screen::Mechanics);}
        else if(choice==4){refreshSave();change(Screen::MainMenu);}
        else m_quitting=true;
        break;
    case Screen::Mechanics:if(choice==0)turnPage(-1);else if(choice==1)turnPage(1);else change(m_return);break;
    case Screen::Controls:change(m_return);break;
    case Screen::Scores:change(Screen::MainMenu);break;
    case Screen::GameOver:
        if(choice==0){if(m_scores.record({m_playerName,m_game.score(),m_game.waves().round(),m_game.gameplayTime()},m_message)){m_recorded=true;m_message="Resultado registrado.";}}
        else if(choice==1){m_candidate.reset();change(Screen::ScenarioSelection);}
        else{refreshSave();change(Screen::MainMenu);}
        break;
    default:break;
    }
}
void Menu::escape(){switch(m_screen){case Screen::Playing:change(Screen::Paused);break;case Screen::Paused:change(Screen::Playing);break;case Screen::Controls:case Screen::Mechanics:change(m_return);break;case Screen::CustomScenario:change(Screen::ScenarioSelection);break;case Screen::ConfirmNewGame:m_candidate.reset();change(Screen::ScenarioSelection);break;case Screen::ScenarioSelection:case Screen::Scores:case Screen::GameOver:change(Screen::MainMenu);break;default:break;}}
void Menu::restartAfterDefeat(){if(m_screen==Screen::GameOver){m_candidate.reset();change(Screen::ScenarioSelection);}}
void Menu::update(float dt,const GameInput& input){
    if(m_screen!=Screen::Playing)return;
    const auto completed=m_game.stats().roundsCompleted;
    m_game.update(dt,input);
    if(m_game.state()==GameState::GameOver){std::string error;if(!m_saves.invalidate(error))m_message="Falha ao invalidar save: "+error;m_canContinue=false;change(Screen::GameOver);}
    else if(m_game.stats().roundsCompleted>completed){std::string error;if(m_saves.save(m_game,error)){m_canContinue=true;m_message="Round concluido. Salvamento automatico.";}else m_message="Falha no salvamento automatico: "+error;}
}
std::vector<ScoreEntry> Menu::scores(){std::string error;auto result=m_scores.load(error);if(!error.empty())m_message="Ranking indisponivel: "+error;return result;}
