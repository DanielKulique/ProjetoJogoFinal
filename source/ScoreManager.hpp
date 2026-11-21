#pragma once
#include <filesystem>
#include <vector>
#include <string>
#include <cstdint>
struct ScoreEntry {
    std::string name;
    std::uint64_t score{0};
    int round{1};
    double seconds{0};
};
class ScoreManager {
public:
    explicit ScoreManager(std::filesystem::path directory):m_directory(std::move(directory)) {}
    std::vector<ScoreEntry> load(std::string& error) const;
    bool record(ScoreEntry entry,std::string& error) const;
private:
    std::filesystem::path m_directory;
};
