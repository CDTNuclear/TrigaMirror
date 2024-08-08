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
    std::string server_ip        = "localhost";
    int         server_port      = 123;
    int         read_tax         = 1000;
    int         kind             = 1;
    std::string kind_aux         = "";
    int         mirror_port      = 0;    //Caso mirror_port não seja alterado pela linha de comando, será igualado a server_port
    int         close            = false;
    std::string log_folder       = "";
    std::string key_path         = "";
};

CONFIG configOptions(int argc, char* argv[])
{
    CONFIG config;

    cxxopts::Options options("trigamirror","TrigaMirror is a software for GNU operating system to get flux data from TrigaServer and share in network.\n");
    options.add_options()
        ("v,version", "Show the program version")
        ("h,help",    "Show this help message")
        ("l,license", "Show info of the license")
        ("i,ip",      "Ip of TrigaServer", cxxopts::value<std::string>())
        ("p,port",    "Port of TrigaServer and TrigaMirror",cxxopts::value<int>())
        ("t,tax",     "Read tax of TrigaServer in ms",cxxopts::value<int>())
        ("m,mirror",  "Change port of TrigaMirror",cxxopts::value<int>())
        ("g,log",     "Save log of connections and choose a place to save",cxxopts::value<std::string>())
        ("s,sing",    "Sing (encrypt) data before send",cxxopts::value<std::string>())
        ("RAW",       "Mirror data in RAW format. Should specify the size in bytes of data")
        ("CSV",       "Mirror data in CSV format")
        ("JSON",      "Mirror data in JSON format. Should specify the last line of JSON file");

    auto result = options.parse(argc, argv);

    if (result.count("version") || result.count("v"))
    {
        showVersion();
        config.close = 1;
        return config;
    } 

    if (result.count("help") || result.count("h"))
    {
        std::cout << options.help() << std::endl;
        config.close = 1;
        return config;
    } 

    if (result.count("license") || result.count("l")) 
    {
        showLicense();
        config.close = 1;
        return config;
    } 

    if (result.count("ip")     || result.count("i")) config.server_ip   = result["ip"].as<std::string>();
    if (result.count("port")   || result.count("p")) config.server_port = result["port"].as<int>();
    if (result.count("tax")    || result.count("t")) config.read_tax    = result["tax"].as<int>();
    if (result.count("mirror") || result.count("m")) config.mirror_port = result["mirror"].as<int>();
    if (result.count("log")    || result.count("g")) config.log_folder  = result["log"].as<std::string>();
    if (result.count("sing")   || result.count("s")) config.key_path    = result["sing"].as<std::string>();
    if (result.count("RAW"))                         config.kind        = 0;
    if (result.count("CSV"))                         config.kind        = 1;
    if (result.count("JSON"))                        config.kind        = 2;
    
    if(config.mirror_port==0) //Se não foi selecionada uma porta para o mirror
    config.mirror_port = config.server_port; //Replique a mesma porta do server
    return config;
}

int main(int argc, char* argv[])
{
    signal(SIGPIPE, SIG_IGN); //Ignorar sinal SIGPIPE quando cliente se desconecta

    CONFIG config = configOptions(argc, argv); //Ler configurações da linha de comando
    if(config.close) return config.close;      //Em caso de erro ou opções extras, encerre o programa.

    TrigaMirror mirror( config.server_ip, 
                        config.server_port, 
                        config.read_tax, 
                        config.kind, 
                        config.log_folder,
                        config.key_path); //Criar objeto e conectar ao servidor
    std::thread mirrorThread (&TrigaMirror::createMirror, &mirror, config.mirror_port); //Criar servidor espelho
    mirrorThread.detach(); //Desacoplar Thread
    
    while(true){} //Loop infinito
    
    return 0;
}
