/*
TrigaMirror is a software for GNU operating system to get the flux
data of TrigaServer share in network.
Copyright (C) 2024 Thalles Campagnani

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
//main.cpp
#include "TrigaMirror.h"
#include <cxxopts.hpp>
#include <fstream>
#include <chrono>
#include <csignal>

void showVersion()
{
    std::cout << "Development Version" << std::endl;
}

void showLicense()
{
    std::cout << "TrigaMirror         Copyright (C) 2024        Thalles Campagnani" << std::endl;
    std::cout << "This    program    comes    with    ABSOLUTELY    NO   WARRANTY;" << std::endl;
    std::cout << "This is free software,    and you are welcome to redistribute it" << std::endl;
    std::cout << "under certain conditions; For more details read the file LICENSE" << std::endl;
    std::cout << "that came together with the source code." << std::endl << std::endl;
}

struct CONFIG
{
    std::string server_ip        = "192.168.1.100"; //
    //int         server_port      = 123;
    //int         mirror_port      = 123;
    int         error_interval   = 2;//
};

CONFIG readConfigFile(std::string filename)
{
    CONFIG config;
    
    std::ifstream configFile(filename);
    
    if (!configFile.is_open()) 
    {
        std::cerr << "[readConfigFile] Erro ao abrir o arquivo de configuração: " << filename << std::endl;
        filename = "trigamirror.conf";
        std::ifstream configFile(filename);
        std::cerr << "[readConfigFile] Tentando abrir: " << filename << std::endl;
        if (!configFile.is_open()) 
        {
            std::cerr << "[readConfigFile] Erro ao abrir o arquivo de configuração: " << filename << std::endl;
            std::cerr << "[readConfigFile] Configurações setadas como padrão" << std::endl;
            return config;
        }
    }

    std::string line;
    while (std::getline(configFile, line)) {
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            // Remova espaços em branco extras
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            if      (key == "ip")               config.server_ip      = value.c_str();
            else if (key == "error_interval")   config.error_interval = std::stoi(value.c_str());
        }
    }

    configFile.close();
    return config;

}

int main(int argc, char* argv[])
{
    signal(SIGPIPE, SIG_IGN);
    std::string filename = "/etc/trigamirror.conf";
    cxxopts::Options options("TrigaMirror","TrigaMirror is a software for GNU operating system to get flux data from TrigaServer and share in network.");
    int port = 12345;

    options.add_options()
        ("v,version", "Show the program version")
        ("h,help",    "Show this help message")
        ("l,license", "Show info of the license")
        ("c,config",  "Change the configuration file")
        ("p,port",    "Port of TrigaServer to mirror");

    auto result = options.parse(argc, argv);

    if (argc >1)
    {
        if (result.count("version") || result.count("v")){
            showVersion();
            return 0;
        } else if (result.count("help") || result.count("h")){
            std::cout << options.help() << std::endl;
            return 0;
        } else if (result.count("license") || result.count("l")) {
            showLicense();
            return 0;
        } else if (result.count("config") || result.count("c")){
            if (result["config"].as<std::string>().empty()) {
                std::cerr << "Error: Please provide a filename for --config option." << std::endl;
                return 1;
            }
            filename = result["config"].as<std::string>();
        } else if (result.count("port") || result.count("p")){
            if (result["port"].as<std::string>().empty()) {
                std::cerr << "Error: Please provide a port for --port option." << std::endl;
                return 1;
            }
            port = result["port"].as<int>();
        } else {
            std::cout << options.help() << std::endl;
            return 1;
        }
    }

    CONFIG config = readConfigFile(filename);

    TrigaMirror mirror       (config.server_ip, port);
    std::thread mirrorThread (&TrigaMirror::createMirror, &mirror, port);

    mirrorThread.detach();

    while(true){}
    
    return 0;
}
