/**
 * Copyright (c) 2018 Cornell University.
 *
 * Author: Ted Yin <tederminant@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <cstring>
#include <cassert>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <unistd.h>

#include "salticidae/util.h"
#include "salticidae/conn.h"

#if !defined(SOL_TCP) && defined(IPPROTO_TCP)
#define SOL_TCP IPPROTO_TCP
#endif

#if !defined(TCP_KEEPIDLE) && defined(TCP_KEEPALIVE)
#define TCP_KEEPIDLE TCP_KEEPALIVE
#endif

namespace salticidae {

ConnPool::Conn::operator std::string() const {
    DataStream s;
    s << "<Conn "
      << "fd_tcp=" << std::to_string(fd_tcp) << " "
      << "fd_udp=" << std::to_string(fd_udp) << " "
      << "addr_tcp=" << std::string(addr_tcp) << " "
      << "addr_udp=" << std::string(addr_udp) << " "
      << "mode=";
    switch (mode)
    {
        case Conn::ACTIVE: s << "active"; break;
        case Conn::PASSIVE: s << "passive"; break;
    }
    s << ">";
    return std::string(std::move(s));
}

/* the following functions are executed by exactly one worker per Conn object */

void ConnPool::Conn::_send_data(const conn_t &conn, int fd, int events, int send_type) {
    bool is_udp = send_type == ConnPool::SendType::UDP;
    
    if (events & FdEvent::ERROR) {
        SALTICIDAE_LOG_INFO("fd_udp %d error !!! exits _send_data, events = %d", fd, events);
        conn->cpool->worker_terminate(conn);
        return;
    }
    ssize_t ret;

    auto& send_buffer = (is_udp) ? conn->send_buffer_udp : conn->send_buffer_tcp ;

    try {
        for (;;) {
            bytearray_t buff_seg = send_buffer.move_pop(); // extract all data from send_buffer, since it has size 0
            ssize_t size = buff_seg.size(); // total remaining
            
            if (!size) break;

            if (!is_udp) 
                ret = send(fd, buff_seg.data(), size, 0);
            else {
                SALTICIDAE_LOG_INFO("UDP socket(%d) sending %zd bytes", fd, size);
                ret = sendto(fd, buff_seg.data(), size, 0, (struct sockaddr*)& conn->group_addr, sizeof(conn->group_addr));
            }

            size -= ret;

            if (size > 0) {
                if (ret < 1) { /* nothing is sent */
                        
                    /* rewind the whole buff_seg */
                    send_buffer.rewind(std::move(buff_seg));
                    SALTICIDAE_LOG_INFO("send ret < 1");

                    if (ret < 0 && errno != EWOULDBLOCK)
                    {
                        SALTICIDAE_LOG_INFO("send(%d) failure: %s", fd, strerror(errno));
                        conn->cpool->worker_terminate(conn);
                        return;
                    }
                }
                else { /* rewind the leftover */
                    SALTICIDAE_LOG_INFO("send rewind");
                    send_buffer.rewind(bytearray_t(buff_seg.begin() + ret, buff_seg.end()));
                }

                /* wait for the next write callback */
                if (!is_udp)
                    conn->ready_send_tcp = false;
                else
                    conn->ready_send_udp = false; // original false

                return;
            }
        }
    }
    catch (...) {
        SALTICIDAE_LOG_INFO("send error");
    }
    /* the send_buffer is empty though the kernel buffer is still available, so
     * temporarily mask the WRITE event and mark the `ready_send` flag */
    if (!is_udp) {
        conn->ev_socket_tcp.del();
        conn->ev_socket_tcp.add(conn->ready_recv_tcp ? 0 : FdEvent::READ);
        conn->ready_send_tcp = true;
    }
    else {
        conn->ev_socket_udp.del();
        conn->ev_socket_udp.add(conn->ready_recv_udp ? 0 : FdEvent::READ);
        conn->ready_send_udp = true;
    }
}

void ConnPool::Conn::_recv_data(const conn_t &conn, int fd, int events, int recv_type) {
    if (events & FdEvent::ERROR) {
        conn->cpool->worker_terminate(conn);
        return;
    }

    const size_t recv_chunk_size = conn->recv_chunk_size;
    ssize_t ret = recv_chunk_size;

    conn->recv_type = recv_type;
    bool is_udp = recv_type == ConnPool::SendType::UDP;

    while (ret == (ssize_t)recv_chunk_size) {
        /* if recv_buffer is full, temporarily mask the READ event */
        if (!is_udp) {
            if (conn->recv_buffer_tcp.len() >= conn->max_recv_buff_size) {
                SALTICIDAE_LOG_INFO("recv_buffer_tcp is full");
                conn->ev_socket_tcp.del();
                conn->ev_socket_tcp.add(conn->ready_send_tcp ? 0 : FdEvent::WRITE);
                conn->ready_recv_tcp = true;
                return;
            }
        }
        else if (conn->recv_buffer_udp.len() >= conn->max_recv_buff_size) {
            SALTICIDAE_LOG_INFO("recv_buffer_udp is full");
            conn->ev_socket_udp.del();
            conn->ev_socket_udp.add(conn->ready_send_udp ? 0 : FdEvent::WRITE);
            conn->ready_recv_udp = true;
            return;
        }
        
        bytearray_t buff_seg;
        buff_seg.resize(recv_chunk_size);

        ret = recv(fd, buff_seg.data(), recv_chunk_size, 0);
        
        if (is_udp)
            SALTICIDAE_LOG_INFO("socket(%d) read %zd bytes", fd, ret);

        if (ret < 0) {
            SALTICIDAE_LOG_INFO("ret < 0 - %d", errno == EWOULDBLOCK);
            if (errno == EWOULDBLOCK) break;
            SALTICIDAE_LOG_INFO("recv(%d) failure: %s", fd, strerror(errno));
            /* connection err or half-opened connection */
            conn->cpool->worker_terminate(conn);
            return;
        }
        if (ret == 0) {
            /* the remote closes the connection */
            conn->cpool->worker_terminate(conn);
            return;
        }

        buff_seg.resize(ret);

        if (!is_udp)
            conn->recv_buffer_tcp.push(std::move(buff_seg));
        else
            conn->recv_buffer_udp.push(std::move(buff_seg));
    }

    /* wait for the next read callback */
    if (!is_udp) {
        conn->ready_recv_tcp = false;
        conn->cpool->on_read(conn);
    }
    else {
        conn->ready_recv_udp = false;
        conn->cpool->on_read(conn);      
    }
}

void ConnPool::Conn::_send_data_tls(const conn_t &conn, int fd, int events, int) {
    if (events & FdEvent::ERROR)
    {
        conn->cpool->worker_terminate(conn);
        return;
    }
    ssize_t ret = conn->recv_chunk_size;
    auto &tls = conn->tls;
    for (;;)
    {
        bytearray_t buff_seg = conn->send_buffer_tcp.move_pop();
        ssize_t size = buff_seg.size();
        if (!size) break;
        ret = tls->send(buff_seg.data(), size);
        SALTICIDAE_LOG_DEBUG("ssl(%d) sent %zd bytes", fd, ret);
        size -= ret;
        if (size > 0)
        {
            if (ret < 1) /* nothing is sent */
            {
                /* rewind the whole buff_seg */
                conn->send_buffer_tcp.rewind(std::move(buff_seg));
                if (ret < 0 && tls->get_error(ret) != SSL_ERROR_WANT_WRITE)
                {
                    SALTICIDAE_LOG_INFO("send(%d) failure: %s", fd, strerror(errno));
                    conn->cpool->worker_terminate(conn);
                    return;
                }
            }
            else
                /* rewind the leftover */
                conn->send_buffer_tcp.rewind(bytearray_t(buff_seg.begin() + ret, buff_seg.end()));
            /* wait for the next write callback */
            conn->ready_send_tcp = false;
            return;
        }
    }
    conn->ev_socket_tcp.del();
    conn->ev_socket_tcp.add(conn->ready_recv_tcp ? 0 : FdEvent::READ);
    conn->ready_send_tcp = true;
}

void ConnPool::Conn::_recv_data_tls(const conn_t &conn, int fd, int events, int) {
    if (events & FdEvent::ERROR)
    {
        conn->cpool->worker_terminate(conn);
        return;
    }
    conn->recv_type = ConnPool::SendType::TCP;

    const size_t recv_chunk_size = conn->recv_chunk_size;
    ssize_t ret = recv_chunk_size;
    auto &tls = conn->tls;
    while (ret == (ssize_t)recv_chunk_size)
    {
        if (conn->recv_buffer_tcp.len() >= conn->max_recv_buff_size)
        {
            conn->ev_socket_tcp.del();
            conn->ev_socket_tcp.add(conn->ready_send_tcp ? 0 : FdEvent::WRITE);
            conn->ready_recv_tcp = true;
            return;
        }
        bytearray_t buff_seg;
        buff_seg.resize(recv_chunk_size);
        ret = tls->recv(buff_seg.data(), recv_chunk_size);
        SALTICIDAE_LOG_DEBUG("ssl(%d) read %zd bytes", fd, ret);
        if (ret < 0)
        {
            if (tls->get_error(ret) == SSL_ERROR_WANT_READ) break;
            SALTICIDAE_LOG_INFO("recv(%d) failure: %s", fd, strerror(errno));
            conn->cpool->worker_terminate(conn);
            return;
        }
        if (ret == 0)
        {
            conn->cpool->worker_terminate(conn);
            return;
        }
        buff_seg.resize(ret);
        conn->recv_buffer_tcp.push(std::move(buff_seg));
    }
    conn->ready_recv_tcp = false;
    conn->cpool->on_read(conn);
}

void ConnPool::Conn::_send_data_tls_handshake(const conn_t &conn, int fd, int events, int) {
    conn->ready_send_tcp = true;
    _recv_data_tls_handshake(conn, fd, events, ConnPool::SendType::TCP);
}

void ConnPool::Conn::_recv_data_tls_handshake(const conn_t &conn, int, int, int) {
    int ret;
    if (conn->tls->do_handshake(ret))
    {
        /* finishing TLS handshake */
        conn->send_data_func = _send_data_tls;
        /* do not start receiving data immediately */
        conn->recv_data_func = _recv_data_dummy;
        conn->ev_socket_tcp.del();
        conn->peer_cert = new X509(conn->tls->get_peer_cert());
        conn->worker->enable_send_buffer_tcp(conn, conn->fd_tcp);
        auto cpool = conn->cpool;
        cpool->on_worker_setup(conn);
        cpool->disp_tcall->async_call([cpool, conn](ThreadCall::Handle &) {
            try {
                cpool->on_dispatcher_setup(conn);
                cpool->update_conn(conn, true);
            } catch (...) {
                cpool->recoverable_error(std::current_exception(), -1);
                cpool->disp_terminate(conn);
            }
        });
    }
    else
    {
        conn->ev_socket_tcp.del();
        conn->ev_socket_tcp.add(ret == 0 ? FdEvent::READ : FdEvent::WRITE);
        SALTICIDAE_LOG_DEBUG("tls handshake %s", ret == 0 ? "read" : "write");
    }
}

void ConnPool::Conn::_recv_data_dummy(const conn_t &, int, int, int) {}

void ConnPool::worker_terminate(const conn_t &conn) {
    conn->worker->get_tcall()->async_call([this, conn](ThreadCall::Handle &) {
        if (!conn->set_terminated()) return;
        on_worker_teardown(conn);
        //conn->stop();
        disp_tcall->async_call([this, conn](ThreadCall::Handle &) {
            del_conn(conn);
        });
    });
}

/****/

int ConnPool::_create_fd_tcp() {
    int _fd_tcp;
    int one = 1;

    if ((_fd_tcp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        SALTICIDAE_LOG_INFO("_create_fd_tcp create socket problem");
        throw ConnPoolError(SALTI_ERROR_CONNECT, errno);
    }
    if (setsockopt(_fd_tcp, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one)) < 0) {
        SALTICIDAE_LOG_INFO("_create_fd_tcp SO_REUSEADDR problem");
        throw ConnPoolError(SALTI_ERROR_CONNECT, errno);
    }
    if (setsockopt(_fd_tcp, SOL_TCP, TCP_NODELAY, (const char *)&one, sizeof(one)) < 0) {
        SALTICIDAE_LOG_INFO("_create_fd_tcp TCP_NODELAY problem");
        throw ConnPoolError(SALTI_ERROR_CONNECT, errno);
    }
    if (fcntl(_fd_tcp, F_SETFL, O_NONBLOCK) == -1) { // Non-blocking read / write fd
        SALTICIDAE_LOG_INFO("_create_fd_tcp O_NONBLOCK problem");
        throw ConnPoolError(SALTI_ERROR_CONNECT, errno);
    }

    return _fd_tcp;
}

int ConnPool::_create_fd_udp(const bool is_sender) {
    int _fd_udp = -1;

    if ((_fd_udp = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        SALTICIDAE_LOG_INFO("_create_fd_udp create socket problem");
        throw ConnPoolError(SALTI_ERROR_CONNECT, errno);
    }
    if (fcntl(_fd_udp, F_SETFL, O_NONBLOCK) == -1) { // Non-blocking read / write fd
        SALTICIDAE_LOG_INFO("_create_fd_udp O_NONBLOCK problem");
        throw ConnPoolError(SALTI_ERROR_CONNECT, errno);
    }

    if (is_sender)
        _multicast_setup_send_fd(_fd_udp);
    else
        _multicast_setup_recv_fd(_fd_udp);

    return _fd_udp;
}

sockaddr_in ConnPool::_set_group_addr() {
    struct sockaddr_in _group_sock;

    memset((char *) &_group_sock, 0, sizeof(_group_sock));
    _group_sock.sin_family = AF_INET;
    _group_sock.sin_addr.s_addr = multicast_addr.ip;
    _group_sock.sin_port = multicast_addr.port;

    return _group_sock;
}

void ConnPool::_multicast_setup_recv_fd(int &fd_udp) {
    // (1) allow multiple processes receive data packets using the same local port
    // (2) bind socket to INADDR_ANY:Port
    // (3) join the multicast group on the interface
    struct sockaddr_in multicast_sock;
    struct ip_mreq multicast_group;
    int reuse = 1;

    // (1) allow multiple processes receive data packets using the same local port
    if (setsockopt(fd_udp, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) {
        SALTICIDAE_LOG_INFO("_multicast_setup_recv_fd SO_REUSEADDR problem");
        throw ConnPoolError(SALTI_ERROR_CONNECT, errno);
    }

    // (2) bind socket to INADDR_ANY:Port
    memset((char *) &multicast_sock, 0, sizeof(multicast_sock));
    multicast_sock.sin_family = AF_INET;
    multicast_sock.sin_addr.s_addr = INADDR_ANY;
    multicast_sock.sin_port = multicast_addr.port;
    if (bind(fd_udp, (struct sockaddr*)&multicast_sock, sizeof(multicast_sock))) {
        SALTICIDAE_LOG_INFO("_multicast_setup_recv_fd bind problem");
        throw ConnPoolError(SALTI_ERROR_CONNECT, errno);
    }

    // (3) join the multicast group on the local interface
    multicast_group.imr_multiaddr.s_addr = multicast_addr.ip;
    multicast_group.imr_interface.s_addr = local_interface.ip;
    if (setsockopt(fd_udp, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&multicast_group, sizeof(multicast_group)) < 0) {
        SALTICIDAE_LOG_INFO("_multicast_setup_recv_fd IP_ADD_MEMBERSHIP problem");
        throw ConnPoolError(SALTI_ERROR_CONNECT, errno);
    }

}

void ConnPool::_multicast_setup_send_fd(int &fd_udp) {
    // (1) set Interface for sending data packets
    // (2) disable loopback data packets

    struct in_addr interface;
    char loopch = 0;

    // (1) Set local interface for outbound multicast datagrams.
    memset(&interface, 0, sizeof(interface));
    interface.s_addr = local_interface.ip;
    if (setsockopt(fd_udp, IPPROTO_IP, IP_MULTICAST_IF, (char *)&interface, sizeof(interface)) < 0) {
        SALTICIDAE_LOG_INFO("_multicast_setup_send_fd IP_MULTICAST_IF problem");
        throw ConnPoolError(SALTI_ERROR_CONNECT, errno);
    }

    // (2) Disable loopback data packets
    if (setsockopt(fd_udp, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopch, sizeof(loopch)) < 0) {
        SALTICIDAE_LOG_INFO("_multicast_setup_send_fd IP_MULTICAST_LOOP problem");
        throw ConnPoolError(SALTI_ERROR_CONNECT, errno);
    }

}

void ConnPool::_set_main_udp() {
    udp_conn_set = true;
    SALTICIDAE_LOG_INFO("set_main_udp");

    // Send Thread - Main Conn Send 
    main_conn_send = create_conn();
    main_conn_send->send_buffer_udp.set_capacity(max_send_buff_size);
    main_conn_send->cpool = this;
    main_conn_send->mode = Conn::ACTIVE;
    main_conn_send->fd_udp = _create_fd_udp(true);
    main_conn_send->group_addr = _set_group_addr();
    main_conn_send->addr_udp = multicast_addr;

    auto &worker_send = get_worker_udp(1);
    main_conn_send->worker = &worker_send;
    worker_send.feed_udp(main_conn_send, main_conn_send->fd_udp, true); // is_sender = true

    add_conn(main_conn_send);

    // Recv Thread - Main Conn Recv 
    main_conn_recv = create_conn();
    main_conn_recv->recv_chunk_size = recv_chunk_size * 16; // 64KB
    main_conn_recv->max_recv_buff_size = max_recv_buff_size * 16; // 64KB
    main_conn_recv->cpool = this;
    main_conn_recv->mode = Conn::PASSIVE;
    main_conn_recv->fd_udp = _create_fd_udp(false);
    main_conn_recv->addr_udp= multicast_addr;

    auto &worker_recv = get_worker_udp(2);
    main_conn_recv->worker = &worker_recv;
    worker_recv.feed_udp(main_conn_recv, main_conn_recv->fd_udp, false); // is_sender = false

    add_conn(main_conn_recv);
}

void ConnPool::disp_terminate(const conn_t &conn) {
    auto worker = conn->worker;
    if (worker)
        worker_terminate(conn);
    else
        disp_tcall->async_call([this, conn](ThreadCall::Handle &) {
            if (!conn->set_terminated()) return;
            on_worker_teardown(conn);
            //conn->stop();
            del_conn(conn);
        });
}

void ConnPool::accept_client(int fd, int, const bool is_p2p) {
    int client_fd_tcp;
    struct sockaddr client_addr;
    int one = 1;
    try {
        socklen_t addr_size = sizeof(struct sockaddr_in);
        if ((client_fd_tcp = accept(fd, &client_addr, &addr_size)) < 0)
        {
            ev_listen.del();
            throw ConnPoolError(SALTI_ERROR_ACCEPT, errno);
        }
        else
        {
            // set up TCP fd
            NetAddr addr_tcp((struct sockaddr_in *)& client_addr);
            if (setsockopt(client_fd_tcp, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one)) < 0)
                throw ConnPoolError(SALTI_ERROR_ACCEPT, errno);
            if (setsockopt(client_fd_tcp, SOL_TCP, TCP_NODELAY, (const char *)&one, sizeof(one)) < 0)
                throw ConnPoolError(SALTI_ERROR_ACCEPT, errno);
            if (fcntl(client_fd_tcp, F_SETFL, O_NONBLOCK) == -1) 
                throw ConnPoolError(SALTI_ERROR_ACCEPT, errno);

            conn_t conn = create_conn();
            conn->send_buffer_tcp.set_capacity(max_send_buff_size);
            conn->recv_chunk_size = recv_chunk_size;
            conn->max_recv_buff_size = max_recv_buff_size;
            conn->fd_tcp = client_fd_tcp;
            conn->cpool = this;
            conn->mode = Conn::PASSIVE;
            conn->addr_tcp = addr_tcp;

            // set up UDP fd
            if (!udp_conn_set && is_p2p) _set_main_udp();

            add_conn(conn);

            SALTICIDAE_LOG_INFO("accepted %s", std::string(*conn).c_str());
            auto &worker = select_worker();
            conn->worker = &worker;

            worker.feed(conn, client_fd_tcp);
        }
    } catch (...) { recoverable_error(std::current_exception(), -1); }
}

void ConnPool::conn_server(const conn_t &conn, int fd, int events) {
    try {
        if (send(fd, "", 0, 0) == 0)
        {
            conn->ev_connect.del();
            SALTICIDAE_LOG_INFO("connected to remote %s", std::string(*conn).c_str());
            auto &worker = select_worker();
            conn->worker = &worker;
            worker.feed(conn, fd);
        }
        else
        {
            if (events & TimedFdEvent::TIMEOUT)
                SALTICIDAE_LOG_INFO("%s connect timeout", std::string(*conn).c_str());

            SALTICIDAE_LOG_INFO("conn_server fd = %d problem", fd);

            throw SalticidaeError(SALTI_ERROR_CONNECT, errno);
        }
    } 
    catch (...) {
        disp_terminate(conn);
        recoverable_error(std::current_exception(), -1);
    }
}

void ConnPool::_listen(NetAddr listen_addr) {
    if (listen_fd != -1)
    { /* reset the previous listen() */
        ev_listen.clear();
        close(listen_fd);
    }
    int listen_fd = _create_fd_tcp();

    struct sockaddr_in sockin;
    memset(&sockin, 0, sizeof(struct sockaddr_in));
    sockin.sin_family = AF_INET;
    sockin.sin_addr.s_addr = INADDR_ANY;
    sockin.sin_port = listen_addr.port;

    if (bind(listen_fd, (struct sockaddr *)&sockin, sizeof(sockin)) < 0)
        throw ConnPoolError(SALTI_ERROR_LISTEN, errno);
    if (::listen(listen_fd, max_listen_backlog) < 0)
        throw ConnPoolError(SALTI_ERROR_LISTEN, errno);
    ev_listen = FdEvent(disp_ec, listen_fd, std::bind(&ConnPool::accept_client, this, _1, _2, is_peer_to_peer));
    ev_listen.add(FdEvent::READ);
    SALTICIDAE_LOG_INFO("listening to %u, P2P = %d", ntohs(listen_addr.port), is_peer_to_peer);
}

ConnPool::conn_t ConnPool::_connect(const NetAddr &addr_tcp) {
    conn_t conn = create_conn();
    conn->send_buffer_tcp.set_capacity(max_send_buff_size);
    conn->recv_chunk_size = recv_chunk_size;
    conn->max_recv_buff_size = max_recv_buff_size;
    conn->fd_tcp = _create_fd_tcp();
    conn->cpool = this;
    conn->mode = Conn::ACTIVE;
    conn->addr_tcp = addr_tcp;

    if (!udp_conn_set && is_peer_to_peer) _set_main_udp();

    add_conn(conn);

    struct sockaddr_in sockin;
    memset(&sockin, 0, sizeof(struct sockaddr_in));
    sockin.sin_family = AF_INET;
    sockin.sin_addr.s_addr = addr_tcp.ip;
    sockin.sin_port = addr_tcp.port;

    if (::connect(conn->fd_tcp, (struct sockaddr *)&sockin,sizeof(struct sockaddr_in)) < 0 && errno != EINPROGRESS)
    {
        SALTICIDAE_LOG_INFO("cannot connect to %s", std::string(addr_tcp).c_str());
        disp_terminate(conn);
    }
    else
    {
        conn->ev_connect = TimedFdEvent(disp_ec, conn->fd_tcp, [this, conn](int fd_tcp, int events) {
            conn_server(conn, fd_tcp, events);
        });
        conn->ev_connect.add(FdEvent::WRITE, conn_server_timeout);
        SALTICIDAE_LOG_INFO("created %s", std::string(*conn).c_str());
    }
    return conn;
}

void ConnPool::del_conn(const conn_t &conn) {
    auto it = pool.find(conn->fd_tcp);
    assert(it != pool.end());
    pool.erase(it);
    update_conn(conn, false);
    release_conn(conn);
}

void ConnPool::release_conn(const conn_t &conn) {
    /* inform the upper layer the connection will be destroyed */
    conn->ev_connect.clear();
    conn->ev_socket_tcp.clear();
    conn->ev_socket_udp.clear();
    on_dispatcher_teardown(conn);
    ::close(conn->fd_tcp);

    if (conn->fd_udp != -1) ::close(conn->fd_udp);
}

ConnPool::conn_t ConnPool::add_conn(const conn_t &conn) {
    assert(pool.find(conn->fd_tcp) == pool.end());
    return pool.insert(std::make_pair(conn->fd_tcp, conn)).first->second;
}

}
