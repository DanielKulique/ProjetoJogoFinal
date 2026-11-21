#pragma once
#include "Game.hpp"
#include <filesystem>
#include <memory>
#include <string>

class SaveManager {
public:
    explicit SaveManager(std::filesystem::path directory);
    bool save(const Game& game, std::string& error) const;
    std::unique_ptr<Game> load(std::string& error) const;
    bool valid() const;
    bool invalidate(std::string& error) const;
    const std::filesystem::path& directory() const { return m_directory; }
    static std::filesystem::path defaultDirectory();
    // Representacao canonica tambem permite testar continuidade exata sem janela.
    static std::string snapshot(const Game& game);
    static std::unique_ptr<Game> restore(const std::string& payload, unsigned version = 2);
private:
    std::filesystem::path m_directory;
};
