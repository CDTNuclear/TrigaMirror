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
//TrigaMirror.cpp
#include "TrigaMirror.h"

TrigaMirror::TrigaMirror(std::string ip, 
                        int port, 
                        int read_tax, 
                        int kind, 
                        std::string logFolder, 
                        std::string privKeyPath)
{
    this->kind = kind;    
    this->privKeyPath = privKeyPath;
    this->logFolder = logFolder;
    
    std::thread readFromServerThread   (&TrigaMirror::readFromServer, this, ip, port, read_tax);
    readFromServerThread.detach();
}

TrigaMirror::~TrigaMirror() {}

void TrigaMirror::logConnection(struct sockaddr_in clientAddr, bool sucesses, int taxAmo)
{
    if (logFolder=="") return; //Se logFolder for nulo, não salve log de conexão.

    //Montar mensagem
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm *parts = std::localtime(&now_c);

    std::ostringstream message;
            message <<  std::put_time(parts, "%Y-%m-%d %H:%M:%S");
            message << ";";
            message << inet_ntoa(clientAddr.sin_addr);
            message << ";";
            message << std::to_string(clientAddr.sin_port);
            message << ";";
            message << std::to_string(sucesses);
            message << ";";
            message << std::to_string(taxAmo);
            message << ";\n";

    //Endereço da pasta + / + nome do arquivo
    std::string fileLocation = logFolder + "/" + inet_ntoa(clientAddr.sin_addr);

    //Verificar se arquivo já existe
    bool alreadyExist = std::filesystem::exists(fileLocation);

    //Abrir arquivo
    std::ofstream outfile(fileLocation,std::ios::app);
    if (outfile.is_open()) // Se o arquivo foi aberto com sucesso
    {
        if(!alreadyExist) outfile << "TIME;IP;PORT;SUCESSES;TaxAmo;\n";//Caso o arquivo esteja sendo criado agora, escreva o cabeçalho
        outfile << message.str(); // Escrever a linha no arquivo
        outfile.close(); // Fechar o arquivo
    }
    else
    {
        message.str("[ logConnection() ] Unable to create/open log file: ");
        message << fileLocation;
        message << "\n";
        std::cerr << message.str();
    }
}


void TrigaMirror::createMirror(int port)
{
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "[startServer] Error opening socket" << std::endl;
        return;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "[startServer] Error on binding" << std::endl;
        return;
    }

    listen(serverSocket, 5);

    while(true) 
    {
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientLen);
        if (clientSocket < 0)
        {
            logConnection(clientAddr, false, 0);
            continue;
        }        
        std::thread clientThread(&TrigaMirror::handleTCPClients, this, clientSocket,clientAddr);
        clientThread.detach();
    }
}

void TrigaMirror::handleTCPClients(int clientSocket, struct sockaddr_in clientAddr)
{
    bool sign=0; 
    const int timeout = 0;
    char buffer[1024];
    int n = recv(clientSocket, buffer, sizeof(buffer), timeout);
    if (n <= 0)
    {
        logConnection(clientAddr, false, 0);
        close(clientSocket);
        return;
    }
    if (buffer[0]=='s') //Caso o primeiro caractere seja s
    {
        if(privKeyPath == "") 
        {
            logConnection(clientAddr, false, 0);
            close(clientSocket);
            return;
        }
        //Se buffer[0]=='s' e privKeyPath != ""
        sign=1;//Assine a mensagem
        buffer[0]='0';//Troque o primeiro caractere por 0 para não dar erro ao interpretar um inteiro
    }
    //Caso algum digito não numero for recebido, encerre a execução
    for (int i = 0; i < n-1; ++i) if (!isdigit(buffer[i])) 
    {
        logConnection(clientAddr, false, 0);
        close(clientSocket);
        return;
    }
    // Parse received data (assuming it's a number)
    int interval = std::stoi(std::string(buffer, n));
    logConnection(clientAddr, true, interval);


    if (sign)
    {
        //Use a chave privada para criptografar a mensagem de 63 bytes em pacotes de 256 bytes
        int numPkgs;//Variavel para armazenar a quantidade de pacotes
        
        //Se for RAW envie a quantidade de bytes primeiro
        if (kind==0)
        {
            std::string dataSizeSigned = signMessage(std::to_string(qtdBytes)+"\n", &numPkgs); ;
            if(send(clientSocket, dataSizeSigned.c_str(), numPkgs*256, 0) <= 0) 
                return;
        }

        if (kind==1) //Se for CSV envie o cabeçalho primeiro
        {
            std::string dataHeaderSigned = signMessage(dataHeader, &numPkgs);
            send(clientSocket, dataHeaderSigned.c_str(), numPkgs*256, 0);
        }
        while(true)
        {
            std::string data = signMessage(*data_global.load(), &numPkgs); //Leia do vetor global os dados
            if(send(clientSocket, data.c_str(), numPkgs*256, 0) <= 0) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        }        
    }
    else
    {
        //Se for RAW envie a quantidade de bytes primeiro
        if (kind==0)
        {
            std::string dataSize = std::to_string(qtdBytes)+"\n";
            if(send(clientSocket, dataSize.c_str(), dataSize.length(), 0) <= 0) 
                return;
        }

        //Se for CSV envie o cabeçalho primeiro
        if (kind==1) send(clientSocket, dataHeader.c_str(), dataHeader.length(), 0);

        while(true)
        {
            std::string data = *data_global.load(); //Leia do vetor global os dados
            if(send(clientSocket, data.c_str(), (kind == 0) ? qtdBytes : data.length(), 0) <= 0) break;//Metodo length() não funciona com RAW pq caracter nulo (\0) interrompe contagem de tamanho, por isso qtdBytes tem que ser passado manualmente
            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        }
    }
}

std::string TrigaMirror::readLine(int clientSocket)
{
    //Loop para ler caractere por caractere até encontrar o fim de linha
    char c = ' ';
    std::string line="";
    while (c != 10)//'\n')
    {
        if(recv(clientSocket, &c, 1, 0) < 1)
        {
            std::cerr << "Erro: Nada recebido\n";
            line="";
            break;
        }
        //Ler 1 caractere e armazenar na variavel c
        line += c;
        //std::cout << "Caractere recebido: " << c << " (ASCII: " << static_cast<int>(c) << ")\n";
    }
    //line += '\n';
    //std::cout << line;
    return line;
}


// Ler dados do servidor
void TrigaMirror::readFromServer(std::string ip, int port, int read_tax) 
{
    int clientSocket;
    struct sockaddr_in serverAddr;
    // Configurar endereço do servidor
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());

    // Loop eterno para tentar conectar o tempo todo
    while (true)
    {
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket < 0)
        {
            std::cerr << "Erro ao criar socket.\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }
        // Conectar ao servidor
        //std::cout << "Tentando conectar!\n";
        if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Erro ao conectar ao servidor.\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            close(clientSocket);
            continue;
        }
        //std::cout << "Conectado!\n";

        // Enviar taxa de amostragem
        if(!send(clientSocket, std::to_string(read_tax).c_str(), std::to_string(read_tax).length(), 0))
        {
            std::cerr << "Erro ao enviar mensagem para servidor.\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            close(clientSocket);
            continue;
        }

        //Logica de ler o servidor varia com o tipo
        if (kind==0)//RAW
        {
            qtdBytes = std::stoi(readLine(clientSocket));
            auto bytes = std::shared_ptr <std::string> (new std::string);
            //Loop eterno para, caso esteja conectado, ler o tempo todo
            while (true)
            {
                char byte[qtdBytes];
                if(recv(clientSocket, byte, qtdBytes, 0) < 1)
                {
                    std::cerr << "Erro: Nada recebido\n";
                    break;
                }
                bytes->assign(byte, qtdBytes);
                data_global.store(bytes);
                //std::cout << *line;
            }
        }
        else if (kind==1)//CSV
        {
            //Salvar header
            if(kind==1) while (dataHeader == "") dataHeader = readLine(clientSocket);

            auto line = std::shared_ptr <std::string> (new std::string);
            //Loop eterno para, caso esteja conectado, ler o tempo todo
            while (true)
            {
                *line = readLine(clientSocket);
                data_global.store(line);
                //std::cout << *line;
            }
        }
        else//JSON
        {
            auto json = std::shared_ptr <std::string> (new std::string);
            //Loop eterno para, caso esteja conectado, ler o tempo todo
            while (true)
            {
                std::string line;
                while (line!="    }\n")
                {
                    line = readLine(clientSocket);
                    *json += line;
                }
                data_global.store(json);
                //std::cout << *json;
            }
        }
    }
}

std::string TrigaMirror::signMessage(const std::string message, int* numPkgs)
{
    *numPkgs=0;
    std::string signedMessage;
    size_t start = 0;
    size_t len = message.length();
    while (start < len) //percorre a mensagem em blocos de até 63 caracteres de cada vez, assinando cada bloco separadamente.
    {
        char bytes[256]= {0};
        size_t end = std::min(start + 63, len);
        std::string command  = "echo \'\'\'";
                    command += message.substr(start, end - start);
                    command += "\'\'\' | openssl pkeyutl -sign -inkey ";
                    command += privKeyPath;
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe)
        {
            std::cerr << "Falha ao abrir pipe.\n";
            return "";
        }   
        size_t bytesRead = fread(bytes, 1, 256, pipe);
        pclose(pipe);
        if (bytesRead != 256)
        {
            std::cerr << "Erro ao ler do pipe. Número de bytes lidos: " << bytesRead << "\n";
            return "";
        }
        signedMessage.append(bytes,256);
        (*numPkgs)++;
        start = end;
    }
    return signedMessage;
}
