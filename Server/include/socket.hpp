#ifndef _SOCKET_HPP_SENTRY_
#define _SOCKET_HPP_SENTRY_

#include <iostream>

/*
Abstract class to represent socket descriptor.

Contains socket's descriptor and setup handler for it.
Informs about interesting events (read, write).
*/
class FdHandler {
    int  sock;
    bool owner;

// Constructor & Destructor
public:
    FdHandler(int a_fd, bool own = true)
        : sock(a_fd), owner(own) {}
    virtual ~FdHandler();

// Accessors
public:
    int  GetSock() const { return sock;  }
    bool IsOwner() const { return owner; }

// Interface
public:
    virtual bool WantRead()  const { return true; }
    virtual bool WantWrite() const { return false; }

    virtual void Handle(bool r, bool w) = 0;
};

/*
Class server-manager.

Contains list of sockets and
constrols messaging between sockets.
Gets events on sockets and calls socket's handler.
*/
class EventSelector {
public:
    FdHandler **fds;
    int fds_len;
    int max_fd;
    bool quit_flag;

// Constructors
public:
    EventSelector()
        : fds(nullptr), fds_len(0), max_fd(-1) {}
    ~EventSelector();

// Interface
public:
    void AddFd(FdHandler *);
    bool RemoveFd(FdHandler *);
    void RunLoop();
    void BreakLoop() { quit_flag = true; }

};

#endif //_SOCKET_HPP_SENTRY_
