#include <iostream>

#include "socket.hpp"
#include "chat.hpp"

static const int port = 7777;

int main()
{
    EventSelector sel;
    ChatServer *serv = ChatServer::StartServer(&sel, port);
    if (!serv) {
        std::cout << "SERVER CREATION ERROR." << std::endl;
        return 1;
    }
    sel.RunLoop();
    return 0;
}
