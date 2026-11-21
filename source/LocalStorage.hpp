#pragma once
#include <filesystem>
#include <string>
namespace LocalStorage {
std::string read(const std::filesystem::path& file, const std::string& magic, unsigned* version = nullptr, unsigned maximumVersion = 1);
void write(const std::filesystem::path& file, const std::string& magic, const std::string& payload, unsigned version = 1);
}
