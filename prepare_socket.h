#ifndef PREPARE_SOCKET_H_
#define PREPARE_SOCKET_H_

#include <iostream>
#include <cstdint>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <atomic>
#include <cstring>
#include <unistd.h>
#include <thread>
#include <fcntl.h>
#include <signal.h>

#include "utils.h"
#include "rt_assert.h"

const int SUBMIT_FD_N = 6;
int submit_fds[SUBMIT_FD_N];
const int RECV_FD_N = 8;
int recv_fds[RECV_FD_N];

uint8_t tmp_buffer[1024];

int next_submit_fd_pos = 0;

int get_send_socket()
{
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    rt_assert(socket_fd >= 0);
    // no delay
    int flag = 1;
    int result = setsockopt(socket_fd,     /* socket affected */
                            IPPROTO_TCP,   /* set option at TCP level */
                            TCP_NODELAY,   /* name of option */
                            (char *)&flag, /* the cast is historical cruft */
                            sizeof(int));  /* length of option value */
    rt_assert_eq(result, 0);

    // connect
    struct sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    struct sockaddr_in *addr_v4 = (struct sockaddr_in *)&addr;
    addr_v4->sin_family = AF_INET;
    addr_v4->sin_port = htons(10002);
    rt_assert_eq(inet_pton(AF_INET, "172.1.1.119", &addr_v4->sin_addr), 1);
    rt_assert_eq(connect(socket_fd, (struct sockaddr *)addr_v4, sizeof(*addr_v4)), 0);
    return socket_fd;
}

int get_recv_socket()
{
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    struct sockaddr_in *addr_v4 = (struct sockaddr_in *)&addr;
    addr_v4->sin_family = AF_INET;
    addr_v4->sin_port = htons(10001);
    rt_assert_eq(inet_pton(AF_INET, "172.1.1.119", &addr_v4->sin_addr), 1);
    rt_assert_eq(connect(socket_fd, (struct sockaddr *)addr_v4, sizeof(*addr_v4)), 0);
    // skip http header
    char tmp[1024];
    while (true)
    {
        memset(tmp, 0, sizeof(tmp));
        ssize_t n = read(socket_fd, tmp, sizeof(tmp));
        rt_assert(n > 0);
        if (strstr(tmp, "\r\n"))
            break;
    }
    rt_assert_eq(fcntl(socket_fd, F_SETFL, fcntl(socket_fd, F_GETFL) | O_NONBLOCK), 0);
    return socket_fd;
}

void prepare_socket()
{
    signal(SIGPIPE, SIG_IGN);

    // send
    for (int i = 0; i < SUBMIT_FD_N; i++)
    {
        submit_fds[i] = get_send_socket();
    }

    // recv
    for (int i = 0; i < RECV_FD_N; i++)
    {
        recv_fds[i] = get_recv_socket();
    }

    // sync recv
    double last_update = get_timestamp();
    while (true)
    {
        if (get_timestamp() - last_update > 0.2)
            break;
        for (int i = 0; i < RECV_FD_N; i++)
        {
            ssize_t n = read(recv_fds[i], tmp_buffer, 1024);
            if (n > 0)
            {
                last_update = get_timestamp();
            }
        }
    }
}
#endif /*PREPARE_SOCKET_H_*/