#include "chat.hpp"

// Network includes
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <iostream>
#include <cstring>
#include <unistd.h>

// Default server's messages
static const char welcome_msg[] = " <- your name";
static const char entered_msg[] = " has entered the chat";
static const char left_msg[] = " has left the chat";

// Generates message according the pattern
// e.g. "<UserName> has left the chat."
static char *GetUserStateMsg(const char *name, const char *msg)
{
    int name_len = strlen(name);
    int msg_len = strlen(msg);
    char *res = new char[name_len + msg_len + 2];
    sprintf(res, "%s%s\n", name, msg);
    return res;
}

// ChatServer implementation ===================================
ChatServer* ChatServer::StartServer(EventSelector *sel, const int port)
{
    int ls, opt, res;
    sockaddr_in addr;

    // Create socket
    ls = socket(AF_INET, SOCK_STREAM, 0);
    if (ls == -1)
        return nullptr;

    // Set socket as reusable
    opt = 1;
    setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Fill address struct
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind socket on address
    res = bind(ls, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (res == -1)
        return nullptr;

    // Start listening on socket
    res = listen(ls, qlen_for_listen);
    if (res == -1)
        return nullptr;

    std::cout << "Server has been started on port " << port << std::endl;
    // Return ChatServer object
    return new ChatServer(sel, ls);
}

ChatServer::ChatServer(EventSelector *sel, int sock)
    : FdHandler(sock, true), the_selector(sel), first(nullptr)
{
    the_selector->AddFd(this);
}

ChatServer::~ChatServer()
{
    while (first) {
        SessItem *tmp = first;
        first = first->next;
        the_selector->RemoveFd(tmp->sess);
        delete tmp->sess;
        delete tmp;
    }
    the_selector->RemoveFd(this);
}

void ChatServer::Handle(bool r, bool w)
{
    if (!r)
        return;
    int client;
    sockaddr_in addr;
    socklen_t len = sizeof(addr);

    client = accept(GetSock(), (sockaddr*)&addr, &len);
    if (client == -1)
        return;

    SessItem *tmp = new SessItem;
    tmp->sess = new ChatSession(this, client);
    tmp->next = first;
    first = tmp;

    the_selector->AddFd(tmp->sess);
}

void ChatServer::RemoveSession(ChatSession *s)
{
    the_selector->RemoveFd(s);
    SessItem **tmp;
    for (tmp = &first; *tmp; tmp = &((*tmp)->next)) {
        if ((*tmp)->sess == s) {
            SessItem *tmp1 = *tmp;
            *tmp = (*tmp)->next;
            delete tmp1->sess;
            delete tmp1;
            return;
        }
    }
}

void ChatServer::SendAll(const char *msg, ChatSession *exclude_fd = nullptr)
{
    for (SessItem *tmp = first; tmp; tmp = tmp->next)
        if (tmp->sess != exclude_fd)
            tmp->sess->Send(msg);
}
// =============================================================


// ChatSession implementation ==================================
ChatSession::ChatSession(ChatServer *sel, int sock)
    : FdHandler(sock, true), buff_inuse(0), ignoring(false),
    name(nullptr), the_master(sel)
{
    Send("Your name: ");
}

ChatSession::~ChatSession()
{
    if (name) delete name;
}

void ChatSession::Send(const char *msg)
{
    write(GetSock(), msg, strlen(msg));
}

void ChatSession::Handle(bool r, bool w)
{
    if (!r)
        return;
    if (buff_inuse >= (int)sizeof(buffer)) {
        buff_inuse = 0;
        ignoring = true;
    }
    if (ignoring)
        ReadAndIgnore();
    else
        ReadAndCheck();
}

void ChatSession::NotifyUserState(const char *state_msg)
{
    int nl = strlen(name);
    int ml = strlen(state_msg);
    char *msg = new char[nl + ml + 2];
    sprintf(msg, "%s%s\n", name, state_msg);
    the_master->SendAll(msg, this);
    delete[] msg;
}

void ChatSession::ReadAndIgnore()
{
    int rc = read(GetSock(), buffer, sizeof(buffer));
    if (rc < 1) {
        the_master->RemoveSession(this);
        return;
    }
    for (int i = 0; i < rc; i++) {
        if (buffer[i] == '\n') {
            ignoring = false;
            int rest = rc - 1 - i;
            if (rest > 0)
                memmove(buffer, buffer+i+1, rest);
            CheckLines();
        }
    }
}

void ChatSession::ReadAndCheck()
{
    int rc =
        read(GetSock(), buffer+buff_inuse, sizeof(buffer)-buff_inuse);
    if (rc < 1) {
        if (name) {
            char *lmsg = GetUserStateMsg(name, left_msg);
            the_master->SendAll(lmsg, this);
            delete[] lmsg;
        }
        the_master->RemoveSession(this);
        return;
    }
    buff_inuse += rc;
    CheckLines();
}

void ChatSession::CheckLines()
{
    if (buff_inuse <= 0)
        return;
    for (int i = 0; i < buff_inuse; i++) {
        if (buffer[i] == '\n') {
            buffer[i] = 0;
            if (i > 0 && buffer[i-1] == '\r')
                buffer[i] = 0;
            ProcessLine(buffer);
            int rest = buff_inuse-1 - i;
            memmove(buffer, buffer + 1 + i, rest);
            buff_inuse = rest;
            CheckLines();
            return;
        }
    }
}

void ChatSession::ProcessLine(const char *msg)
{
    int len = strlen(msg);
    if (!name) {
        // If has no name set first msg as name
        name = new char[len + 1];
        strncpy(name, msg, len+1);

        // Send to user that he is logged in
        char *wmsg = GetUserStateMsg(name, welcome_msg);
        Send(wmsg);
        delete[] wmsg;

        // Send to other users that new user logged in
        char *emsg = GetUserStateMsg(name, entered_msg);
        the_master->SendAll(emsg, this);
        delete[] emsg;
        return;
    }

    // Handle change name command
    if (!strncmp(msg, "/name ", 6)) {
        char *lmsg = GetUserStateMsg(name, left_msg);
        the_master->SendAll(lmsg, this);
        delete[] lmsg;

        delete[] name;
        name = nullptr;
        ProcessLine(msg+6);
        return;
    }

    // If name is already exists send msg as message from user
    int nl = strlen(name);
    char *msg_to_send = new char[nl + len + 6];
    sprintf(msg_to_send, "[%s]: %s\n", name, msg);
    the_master->SendAll(msg_to_send);
    delete[] msg_to_send;
}












// =============================================================
