#include "LocalStorage.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
namespace {
std::uint64_t checksum(const std::string& s) {
    std::uint64_t n=14695981039346656037ULL;
    for (unsigned char c:s) { n^=c; n*=1099511628211ULL; }
    return n;
}
constexpr std::size_t limit=8*1024*1024;
}
std::string LocalStorage::read(const std::filesystem::path& file,const std::string& magic,unsigned* actualVersion,unsigned maximumVersion) {
    if (std::filesystem::file_size(file)>limit) throw std::runtime_error("Arquivo excede 8 MiB");
    std::ifstream in(file,std::ios::binary);
    std::string header; std::getline(in,header);
    std::istringstream h(header); std::string tag,extra; unsigned version=0; std::size_t size=0; std::uint64_t sum=0;
    if (!(h>>tag>>version>>size>>sum) || h>>extra || tag!=magic || (version<1 || version>maximumVersion) || size>limit) throw std::runtime_error("Cabecalho ou versao invalida");
    std::string payload(size,'\0');
    if (!in.read(payload.data(),static_cast<std::streamsize>(size)) || in.peek()!=std::char_traits<char>::eof() || checksum(payload)!=sum) throw std::runtime_error("Arquivo truncado ou corrompido");
    if(actualVersion)*actualVersion=version;
    return payload;
}
void LocalStorage::write(const std::filesystem::path& file,const std::string& magic,const std::string& payload,unsigned version) {
    if (payload.size()>limit) throw std::runtime_error("Estado excede 8 MiB");
    std::filesystem::create_directories(file.parent_path());
    std::string pattern=(file.string()+".tmp.XXXXXX");
    int fd=::mkstemp(pattern.data());
    if(fd<0) throw std::runtime_error("Nao foi possivel criar arquivo temporario");
    try {
        const std::string data=magic+" "+std::to_string(version)+" "+std::to_string(payload.size())+" "+std::to_string(checksum(payload))+"\n"+payload;
        std::size_t done=0;
        while(done<data.size()) { auto n=::write(fd,data.data()+done,data.size()-done); if(n<0 && errno==EINTR) continue; if(n<=0) throw std::runtime_error("Falha na gravacao"); done+=static_cast<std::size_t>(n); }
        if(::fsync(fd)!=0) throw std::runtime_error("Falha ao sincronizar arquivo");
        if(::close(fd)!=0) { fd=-1; throw std::runtime_error("Falha ao fechar arquivo"); } fd=-1;
        std::filesystem::rename(pattern,file);
        const int dir=::open(file.parent_path().c_str(),O_RDONLY|O_DIRECTORY);
        if(dir>=0) { ::fsync(dir); ::close(dir); }
    } catch(...) { if(fd>=0) ::close(fd); std::filesystem::remove(pattern); throw; }
}
