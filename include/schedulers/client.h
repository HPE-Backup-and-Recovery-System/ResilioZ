#ifndef CLIENT_H_
#define CLIENT_H_

#include <iostream>
#include <cstring>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>

#include <nlohmann/json.hpp>

class Client{
    public:
        Client();

        void Send(const char* message);
        void Run();
    
    private:
        sockaddr_in serv_addr;
};

#endif