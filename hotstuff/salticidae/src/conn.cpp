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
      << "fd=" << std::to_string(fd) << " "
      << "addr=" << std::string(addr) << " "
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

void ConnPool::Conn::_send_data(const conn_t &conn, int fd, int events) {
    if (events & FdEvent::ERROR)
    {
        conn->cpool->worker_terminate(conn);
        return;
    }
    ssize_t ret = conn->recv_chunk_size;
    for (;;)
    {
        bytearray_t buff_seg = conn->send_buffer.move_pop();
        ssize_t size = buff_seg.size();
        if (!size) break;
        ret = send(fd, buff_seg.data(), size, 0);
        SALTICIDAE_LOG_DEBUG("socket(%d) sent %zd bytes", fd, ret);
        size -= ret;
        if (size > 0)
        {
            if (ret < 1) /* nothing is sent */
            {
                /* rewind the whole buff_seg */
                conn->send_buffer.rewind(std::move(buff_seg));
                if (ret < 0 && errno != EWOULDBLOCK)
                {
                    SALTICIDAE_LOG_INFO("send(%d) failure: %s", fd, strerror(errno));
                    conn->cpool->worker_terminate(conn);
                    return;
                }
            }
            else
                /* rewind the leftover */
                conn->send_buffer.rewind(
                    bytearray_t(buff_seg.begin() + ret, buff_seg.end()));
            /* wait for the next write callback */
            conn->ready_send = false;
            return;
        }
    }
    /* the send_buffer is empty though the kernel buffer is still available, so
     * temporarily mask the WRITE event and mark the `ready_send` flag */
    conn->ev_socket.del();
    conn->ev_socket.add(conn->ready_recv ? 0 : FdEvent::READ);
    conn->ready_send = true;
}

void ConnPool::Conn::_recv_data(const conn_t &conn, int fd, int events) {
    if (events & FdEvent::ERROR)
    {
        conn->cpool->worker_terminate(conn);
        return;
    }
    const size_t recv_chunk_size = conn->recv_chunk_size;
    ssize_t ret = recv_chunk_size;
    while (ret == (ssize_t)recv_chunk_size)
    {
        if (conn->recv_buffer.len() >= conn->max_recv_buff_size)
        {
            /* recv_buffer is full, temporarily mask the READ event */
            conn->ev_socket.del();
            conn->ev_socket.add(conn->ready_send ? 0 : FdEvent::WRITE);
            conn->ready_recv = true;
            return;
        }
        bytearray_t buff_seg;
        buff_seg.resize(recv_chunk_size);
        ret = recv(fd, buff_seg.data(), recv_chunk_size, 0);
        SALTICIDAE_LOG_DEBUG("socket(%d) read %zd bytes", fd, ret);
        if (ret < 0)
        {
            if (errno == EWOULDBLOCK) break;
            SALTICIDAE_LOG_INFO("recv(%d) failure: %s", fd, strerror(errno));
            /* connection err or half-opened connection */
            conn->cpool->worker_terminate(conn);
            return;
        }
        if (ret == 0)
        {
            /* the remote closes the connection */
            conn->cpool->worker_terminate(conn);
            return;
        }
        buff_seg.resize(ret);
        conn->recv_buffer.push(std::move(buff_seg));
    }
    /* wait for the next read callback */
    conn->ready_recv = false;
    conn->cpool->on_read(conn);
}


void ConnPool::Conn::_send_data_tls(const conn_t &conn, int fd, int events) {
    if (events & FdEvent::ERROR)
    {
        conn->cpool->worker_terminate(conn);
        return;
    }
    ssize_t ret = conn->recv_chunk_size;
    auto &tls = conn->tls;
    for (;;)
    {
        bytearray_t buff_seg = conn->send_buffer.move_pop();
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
                conn->send_buffer.rewind(std::move(buff_seg));
                if (ret < 0 && tls->get_error(ret) != SSL_ERROR_WANT_WRITE)
                {
                    SALTICIDAE_LOG_INFO("send(%d) failure: %s", fd, strerror(errno));
                    conn->cpool->worker_terminate(conn);
                    return;
                }
            }
            else
                /* rewind the leftover */
                conn->send_buffer.rewind(
                    bytearray_t(buff_seg.begin() + ret, buff_seg.end()));
            /* wait for the next write callback */
            conn->ready_send = false;
            return;
        }
    }
    conn->ev_socket.del();
    conn->ev_socket.add(conn->ready_recv ? 0 : FdEvent::READ);
    conn->ready_send = true;
}

void ConnPool::Conn::_recv_data_tls(const conn_t &conn, int fd, int events) {
    if (events & FdEvent::ERROR)
    {
        conn->cpool->worker_terminate(conn);
        return;
    }
    const size_t recv_chunk_size = conn->recv_chunk_size;
    ssize_t ret = recv_chunk_size;
    auto &tls = conn->tls;
    while (ret == (ssize_t)recv_chunk_size)
    {
        if (conn->recv_buffer.len() >= conn->max_recv_buff_size)
        {
            conn->ev_socket.del();
            conn->ev_socket.add(conn->ready_send ? 0 : FdEvent::WRITE);
            conn->ready_recv = true;
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
        conn->recv_buffer.push(std::move(buff_seg));
    }
    conn->ready_recv = false;
    conn->cpool->on_read(conn);
}

void ConnPool::Conn::_send_data_tls_handshake(const conn_t &conn, int fd, int events) {
    conn->ready_send = true;
    _recv_data_tls_handshake(conn, fd, events);
}

void ConnPool::Conn::_recv_data_tls_handshake(const conn_t &conn, int, int) {
    int ret;
    if (conn->tls->do_handshake(ret))
    {
        /* finishing TLS handshake */
        conn->send_data_func = _send_data_tls;
        /* do not start receiving data immediately */
        conn->recv_data_func = _recv_data_dummy;
        conn->ev_socket.del();
        //conn->ev_socket.add(FdEvent::WRITE);
        conn->peer_cert = new X509(conn->tls->get_peer_cert());
        conn->worker->enable_send_buffer(conn, conn->fd);
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
        conn->ev_socket.del();
        conn->ev_socket.add(ret == 0 ? FdEvent::READ : FdEvent::WRITE);
        SALTICIDAE_LOG_DEBUG("tls handshake %s", ret == 0 ? "read" : "write");
    }
}

void ConnPool::Conn::_recv_data_dummy(const conn_t &, int, int) {}

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

void ConnPool::accept_client(int fd, int) {
    int client_fd;
    struct sockaddr client_addr;
    try {
        socklen_t addr_size = sizeof(struct sockaddr_in);
        if ((client_fd = accept(fd, &client_addr, &addr_size)) < 0)
        {
            ev_listen.del();
            throw ConnPoolError(SALTI_ERROR_ACCEPT, errno);
        }
        else
        {
            int one = 1;
            if (setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one)) < 0 ||
                //setsockopt(client_fd, SOL_SOCKET, SO_REUSEPORT, (const char *)&one, sizeof(one)) < 0 ||
                setsockopt(client_fd, SOL_TCP, TCP_NODELAY, (const char *)&one, sizeof(one)) < 0)
                throw ConnPoolError(SALTI_ERROR_ACCEPT, errno);
            if (fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1)
                throw ConnPoolError(SALTI_ERROR_ACCEPT, errno);

            NetAddr addr((struct sockaddr_in *)&client_addr);
            conn_t conn = create_conn();
            conn->send_buffer.set_capacity(max_send_buff_size);
            conn->recv_chunk_size = recv_chunk_size;
            conn->max_recv_buff_size = max_recv_buff_size;
            conn->fd = client_fd;
            conn->cpool = this;
            conn->mode = Conn::PASSIVE;
            conn->addr = addr;
            add_conn(conn);
            SALTICIDAE_LOG_INFO("accepted %s", std::string(*conn).c_str());
            auto &worker = select_worker();
            conn->worker = &worker;
            worker.feed(conn, client_fd);
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
            throw SalticidaeError(SALTI_ERROR_CONNECT, errno);
        }
    } catch (...) {
        disp_terminate(conn);
        recoverable_error(std::current_exception(), -1);
    }
}

void ConnPool::_listen(NetAddr listen_addr) {
    int one = 1;
    if (listen_fd != -1)
    { /* reset the previous listen() */
        ev_listen.clear();
        close(listen_fd);
    }
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        throw ConnPoolError(SALTI_ERROR_LISTEN, errno);
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one)) < 0 ||
        //setsockopt(listen_fd, SOL_SOCKET, SO_REUSEPORT, (const char *)&one, sizeof(one)) < 0 ||
        setsockopt(listen_fd, SOL_TCP, TCP_NODELAY, (const char *)&one, sizeof(one)) < 0)
        throw ConnPoolError(SALTI_ERROR_LISTEN, errno);
    if (fcntl(listen_fd, F_SETFL, O_NONBLOCK) == -1)
        throw ConnPoolError(SALTI_ERROR_LISTEN, errno);

    struct sockaddr_in sockin;
    memset(&sockin, 0, sizeof(struct sockaddr_in));
    sockin.sin_family = AF_INET;
    sockin.sin_addr.s_addr = INADDR_ANY;
    sockin.sin_port = listen_addr.port;

    if (bind(listen_fd, (struct sockaddr *)&sockin, sizeof(sockin)) < 0)
        throw ConnPoolError(SALTI_ERROR_LISTEN, errno);
    if (::listen(listen_fd, max_listen_backlog) < 0)
        throw ConnPoolError(SALTI_ERROR_LISTEN, errno);
    ev_listen = FdEvent(disp_ec, listen_fd,
                std::bind(&ConnPool::accept_client, this, _1, _2));
    ev_listen.add(FdEvent::READ);
    SALTICIDAE_LOG_INFO("listening to %u", ntohs(listen_addr.port));
}

ConnPool::conn_t ConnPool::_connect(const NetAddr &addr) {
    int fd;
    int one = 1;
    if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        throw ConnPoolError(SALTI_ERROR_CONNECT, errno);
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one)) < 0 ||
        //setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (const char *)&one, sizeof(one)) < 0 ||
        setsockopt(fd, SOL_TCP, TCP_NODELAY, (const char *)&one, sizeof(one)) < 0)
        throw ConnPoolError(SALTI_ERROR_CONNECT, errno);
    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
        throw ConnPoolError(SALTI_ERROR_CONNECT, errno);
    conn_t conn = create_conn();
    conn->send_buffer.set_capacity(max_send_buff_size);
    conn->recv_chunk_size = recv_chunk_size;
    conn->max_recv_buff_size = max_recv_buff_size;
    conn->fd = fd;
    conn->cpool = this;
    conn->mode = Conn::ACTIVE;
    conn->addr = addr;
    add_conn(conn);

    struct sockaddr_in sockin;
    memset(&sockin, 0, sizeof(struct sockaddr_in));
    sockin.sin_family = AF_INET;
    sockin.sin_addr.s_addr = addr.ip;
    sockin.sin_port = addr.port;

    if (::connect(fd, (struct sockaddr *)&sockin,
                sizeof(struct sockaddr_in)) < 0 && errno != EINPROGRESS)
    {
        SALTICIDAE_LOG_INFO("cannot connect to %s", std::string(addr).c_str());
        disp_terminate(conn);
    }
    else
    {
        conn->ev_connect = TimedFdEvent(disp_ec, conn->fd, [this, conn](int fd, int events) {
            conn_server(conn, fd, events);
        });
        conn->ev_connect.add(FdEvent::WRITE, conn_server_timeout);
        SALTICIDAE_LOG_INFO("created %s", std::string(*conn).c_str());
    }
    return conn;
}

void ConnPool::del_conn(const conn_t &conn) {
    auto it = pool.find(conn->fd);
    assert(it != pool.end());
    pool.erase(it);
    update_conn(conn, false);
    release_conn(conn);
}

void ConnPool::release_conn(const conn_t &conn) {
    /* inform the upper layer the connection will be destroyed */
    conn->ev_connect.clear();
    conn->ev_socket.clear();
    on_dispatcher_teardown(conn);
    ::close(conn->fd);
}

ConnPool::conn_t ConnPool::add_conn(const conn_t &conn) {
    assert(pool.find(conn->fd) == pool.end());
    return pool.insert(std::make_pair(conn->fd, conn)).first->second;
}

}
