#ifndef _CHAT_HPP_SENTRY_
#define _CHAT_HPP_SENTRY_

#include "socket.hpp"

// Constant values for class's insinde functionality.
enum {
    max_line_len = 1023,
    qlen_for_listen = 16
};


class ChatSession;

/*
Representation of listener socket.

Accepts connections as new
ChatSession objects, when they appear.
Also stores ChatSession objects and
provides messaging between them.
*/
class ChatServer: public FdHandler {
    EventSelector *the_selector;
    struct SessItem {
        ChatSession *sess;
        SessItem *next;
    } *first;

// Constructor & Destructor
private:
    ChatServer(EventSelector *, int sock);
public:
    // Creates ChatServer object
    static ChatServer* StartServer(EventSelector *, const int port);
    ~ChatServer();

// Interface
public:
    void RemoveSession(ChatSession *);
    void SendAll(const char *, ChatSession *exclude_fd);
private:
    virtual void Handle(bool r, bool w);
};

/*
Representation of client's socket.

Handles message from user and sends notify
to ChatServer object about it.
*/
class ChatSession: public FdHandler {
// Friends
    friend class ChatServer;

// Members
    char buffer[max_line_len+1]; // message buffer
    int buff_inuse;
    bool ignoring; // Send or not?
    char *name; // Username
    ChatServer *the_master;

// Constructor & Destructor
    ChatSession(ChatServer *a_master, int sock);
    ~ChatSession();

// Interface
    void Send(const char *);
    void NotifyUserState(const char *state_msg);
    virtual void Handle(bool r, bool w);

// Buffer processing
    void ReadAndIgnore();
    void ReadAndCheck();
    void CheckLines();
    void ProcessLine(const char *);
};

#endif //_CHAT_HPP_SENTRY_
