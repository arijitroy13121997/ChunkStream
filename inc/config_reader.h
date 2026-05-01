#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

struct Config
{
    std::string ip;
    int port;
    std::string file;
    std::string output;
    size_t chunk_size;
};

Config load_cfg(){
    std::ifstream f(CONFIG_PATH);
    if(!f){
        throw std::runtime_error("Could not open config.json");
    }
    json j;
    f >> j;

    Config cfg;
    cfg.ip = j["ip"];
    cfg.port = j["port"];
    cfg.file = j["file"];
    cfg.chunk_size = j["chunk_size"];
 
    return cfg;
}
