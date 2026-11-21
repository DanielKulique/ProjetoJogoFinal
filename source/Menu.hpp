#pragma once
#include "Game.hpp"
#include "SaveManager.hpp"
#include "ScoreManager.hpp"
#include <string>
#include <vector>

enum class Screen { MainMenu, Playing, Paused, Scores, Controls, Mechanics, GameOver, ConfirmNewGame, ScenarioSelection, CustomScenario };
class Menu {
public:
    Menu(Game& game,std::filesystem::path directory);
    [[nodiscard]] Screen screen() const {return m_screen;}
    [[nodiscard]] std::vector<std::string> choices() const;
    [[nodiscard]] bool enabled(std::size_t index) const;
    [[nodiscard]] int selected() const {return m_selected;}
    void select(int index);
    void navigate(int delta);
    void activate();
    void escape();
    void update(float dt,const GameInput& input);
    void restartAfterDefeat();
    bool quitting() const {return m_quitting;}
    bool canContinue() const {return m_canContinue;}
    const std::string& message() const {return m_message;}
    std::string& playerName() {return m_playerName;}
    bool scoreRecorded() const {return m_recorded;}
    std::vector<ScoreEntry> scores();
    void refreshSave();
    std::string& csvPath() {return m_csvPath;}
    int mechanicsPage() const {return m_mechanicsPage;}
    void turnPage(int delta){m_mechanicsPage=(m_mechanicsPage+delta+3)%3;}
private:
    Game& m_game;
    SaveManager m_saves;
    ScoreManager m_scores;
    Screen m_screen{Screen::MainMenu},m_return{Screen::MainMenu};
    int m_selected{0};
    int m_mechanicsPage{0};
    bool m_canContinue{false},m_quitting{false},m_recorded{false};
    std::string m_message,m_playerName{"Piloto"};
    std::string m_csvPath;
    std::unique_ptr<Game> m_candidate;
    void change(Screen screen);
    void newGame();
    void prepareGame(ScenarioType type,const std::string& csvPath = {});
};
