#include <iostream>

#include "socket.hpp"
#include "chat.hpp"

static const int port = 7777;

int main(const int argc, const char **argv)
{
    EventSelector sel;
    ChatServer *serv = ChatServer::StartServer(&sel, 
        argc == 2 ? atoi(argv[1]) : port); // port

    if (!serv) {
        std::cout << "SERVER CREATION ERROR." << std::endl;
        return 1;
    }
    sel.RunLoop();
    return 0;
}
