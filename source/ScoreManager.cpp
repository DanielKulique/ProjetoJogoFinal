#include "ScoreManager.hpp"
#include "LocalStorage.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <locale>
#include <stdexcept>
#include <charconv>
std::vector<ScoreEntry> ScoreManager::load(std::string& error) const {
    try {
        if(!std::filesystem::exists(m_directory/"scores.dat")) {error.clear();return {};}
        auto data=LocalStorage::read(m_directory/"scores.dat","SPACE_ARENA_SCORES");
        std::istringstream in(data);in.imbue(std::locale::classic());std::size_t count;
        if(!(in>>count) || count>10) throw std::runtime_error("Ranking invalido");
        std::vector<ScoreEntry> result;
        for(std::size_t i=0;i<count;++i) {
            ScoreEntry e;std::string rawScore;
            if(!(in>>std::quoted(e.name)>>rawScore>>e.round>>e.seconds) || e.name.empty() || e.name.size()>32 || e.round<1 || !std::isfinite(e.seconds) || e.seconds<0) throw std::runtime_error("Resultado invalido");
            const auto parsed=std::from_chars(rawScore.data(),rawScore.data()+rawScore.size(),e.score);
            if(parsed.ec!=std::errc{} || parsed.ptr!=rawScore.data()+rawScore.size())throw std::runtime_error("Pontuacao invalida");
            for(unsigned char c:e.name)if(c<32 || c==127)throw std::runtime_error("Nome invalido");
            result.push_back(e);
        }
        in>>std::ws;if(!in.eof())throw std::runtime_error("Dados extras no ranking");
        std::stable_sort(result.begin(),result.end(),[](const auto& a,const auto& b){return a.score>b.score;});
        error.clear();return result;
    }catch(const std::exception& e){error=e.what();return {};}
}
bool ScoreManager::record(ScoreEntry entry,std::string& error) const {
    try {
        if(entry.name.empty() || entry.name.size()>32 || entry.round<1 || !std::isfinite(entry.seconds) || entry.seconds<0)throw std::runtime_error("Nome ou resultado invalido");
        for(unsigned char c:entry.name)if(c<32 || c==127)throw std::runtime_error("Nome invalido");
        auto scores=load(error);if(!error.empty())return false; // Nao sobrescreve ranking corrompido.
        scores.push_back(std::move(entry));std::stable_sort(scores.begin(),scores.end(),[](const auto& a,const auto& b){return a.score>b.score;});if(scores.size()>10)scores.resize(10);
        std::ostringstream out;out.imbue(std::locale::classic());out<<std::setprecision(17)<<scores.size()<<'\n';
        for(const auto& e:scores)out<<std::quoted(e.name)<<' '<<e.score<<' '<<e.round<<' '<<e.seconds<<'\n';
        LocalStorage::write(m_directory/"scores.dat","SPACE_ARENA_SCORES",out.str());error.clear();return true;
    }catch(const std::exception& e){error=e.what();return false;}
}
