#include "socket.hpp"

#include <iostream>
#include <unistd.h>
#include <sys/select.h>

FdHandler::~FdHandler()
{
    if (owner) close(sock);
}

EventSelector::~EventSelector()
{
    if (fds)
        delete[] fds;
}

void EventSelector::AddFd(FdHandler *fd)
{
    int sock = fd->GetSock();
    if (!fds) {
        fds_len = sock <= 15 ? 16 : sock+1;
        fds = new FdHandler*[fds_len];
        for (int i = 0; i < fds_len; i++)
            fds[i] = nullptr;
    }
    if (fds_len <= sock) {
        FdHandler **tmp = new FdHandler*[sock+1];
        for(int i = 0; i <= sock; i++)
            tmp[i] = i < fds_len ? fds[i] : nullptr;
        fds_len = sock+1;
        delete[] fds;
        fds = tmp;
    }
    fds[sock] = fd;
    if (max_fd < sock)
        max_fd = sock;
}

bool EventSelector::RemoveFd(FdHandler *fd)
{
    int sock = fd->GetSock();
    if (sock >= fds_len || fds[sock] != fd)
        return false;
    fds[sock] = nullptr;

    // Do we remove max fd?
    if (sock == max_fd)
        while (!fds[max_fd] && max_fd >= 0)
            max_fd--;

    return true;
}

void EventSelector::RunLoop()
{
    quit_flag = false;
    do {
        int i;
        fd_set rds, wrs;
        FD_ZERO(&rds);
        FD_ZERO(&wrs);

        // Fill the sets
        for (i = 0; i <= max_fd; i++) {
            if (fds[i]) {
                if (fds[i]->WantRead())
                    FD_SET(i, &rds);
                if (fds[i]->WantWrite())
                    FD_SET(i, &wrs);
            }
        }
        int res = select(max_fd + 1, &rds, &wrs, 0, 0);
        if (res < 0) {
            if (errno == EINTR)
                continue;
            else
                break;
        }
        if (res > 0) {
            for (i = 0; i < fds_len; i++) {
                if (!fds[i])
                    continue;
                bool r = FD_ISSET(i, &rds);
                bool w = FD_ISSET(i, &wrs);
                if (r || w)
                    fds[i]->Handle(r, w);
            }
        }
    } while (!quit_flag);
}
