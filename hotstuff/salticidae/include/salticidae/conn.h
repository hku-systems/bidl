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

#ifndef _SALTICIDAE_CONN_H
#define _SALTICIDAE_CONN_H

#ifdef __cplusplus
#include <cassert>
#include <cstdint>
#include <arpa/inet.h>
#include <unistd.h>

#include <string>
#include <unordered_map>
#include <list>
#include <algorithm>
#include <exception>
#include <mutex>
#include <thread>
#include <fcntl.h>

#include "salticidae/type.h"
#include "salticidae/ref.h"
#include "salticidae/event.h"
#include "salticidae/util.h"
#include "salticidae/netaddr.h"
#include "salticidae/msg.h"
#include "salticidae/buffer.h"

namespace salticidae {

/** Abstraction for connection management. */
class ConnPool {
    protected:
    class Worker;

    public:
    class Conn;
    /** The handle to a bi-directional connection. */
    using conn_t = ArcObj<Conn>;
    /** The type of callback invoked when connection status is changed. */
    using conn_callback_t = std::function<bool(const conn_t &, bool)>;
    /** The type of callback invoked when an error occured (during async execution). */
    using error_callback_t = std::function<void(const std::exception_ptr, bool, int32_t)>;
    /** Abstraction for a bi-directional connection. */
    class Conn {
        friend ConnPool;
        public:
        enum ConnMode {
            ACTIVE, /**< the connection is established by connect() */
            PASSIVE, /**< the connection is established by accept() */
        };

        protected:
        std::atomic<bool> terminated;
        size_t recv_chunk_size;
        size_t max_recv_buff_size;
        int fd;
        Worker *worker;
        ConnPool *cpool;
        ConnMode mode;
        NetAddr addr;

        MPSCWriteBuffer send_buffer;
        SegBuffer recv_buffer;

        /* initialized and destroyed by the dispatcher */
        TimedFdEvent ev_connect;
        /* initialized and destroyed by the worker */
        FdEvent ev_socket;
        /** does not need to wait if true */
        bool ready_send;
        bool ready_recv;

        typedef void (socket_io_func)(const conn_t &, int, int);
        socket_io_func *send_data_func;
        socket_io_func *recv_data_func;
        BoxObj<TLS> tls;
        BoxObj<const X509> peer_cert;

        static socket_io_func _recv_data;
        static socket_io_func _send_data;

        static socket_io_func _recv_data_tls;
        static socket_io_func _send_data_tls;
        static socket_io_func _recv_data_tls_handshake;
        static socket_io_func _send_data_tls_handshake;
        static socket_io_func _recv_data_dummy;

        public:
        Conn(): terminated(false),
            // recv_chunk_size initialized later
            // max_recv_buff_size initialized later
            fd(-1),
            worker(nullptr),
            cpool(nullptr),
            mode(ConnMode::PASSIVE),
            ready_send(false), ready_recv(false),
            send_data_func(nullptr), recv_data_func(nullptr),
            tls(nullptr), peer_cert(nullptr) {}
        Conn(const Conn &) = delete;
        Conn(Conn &&other) = delete;

        virtual ~Conn() {
            SALTICIDAE_LOG_DEBUG("destroyed %s", std::string(*this).c_str());
        }

        bool is_terminated() const {
            return terminated.load(std::memory_order_acquire);
        }

        bool set_terminated() {
            return !terminated.exchange(true, std::memory_order_acq_rel);
        }

        operator std::string() const;
        const NetAddr &get_addr() const { return addr; }
        const X509 *get_peer_cert() const { return peer_cert.get(); }
        ConnMode get_mode() const { return mode; }
        ConnPool *get_pool() const { return cpool; }

        /** Write data to the connection (non-blocking). The data will be sent
         * whenever I/O is available. */
        bool write(bytearray_t &&data) {
            return send_buffer.push(std::move(data), !cpool->max_send_buff_size);
        }
    };

    protected:
    int system_state;
    EventContext ec;
    EventContext disp_ec;
    ThreadCall* disp_tcall;
    BoxObj<ThreadCall> user_tcall;
    RcObj<const X509> tls_cert;

    using worker_error_callback_t = std::function<void(const std::exception_ptr err)>;
    worker_error_callback_t disp_error_cb;
    worker_error_callback_t worker_error_cb;
    std::atomic<uint16_t> async_id;

    int32_t gen_async_id() { return async_id.fetch_add(1, std::memory_order_relaxed); }
    conn_t _connect(const NetAddr &addr);
    void _listen(NetAddr listen_addr);
    void recoverable_error(const std::exception_ptr err, int32_t id) const {
        user_tcall->async_call([this, err, id](ThreadCall::Handle &) {
            if (error_cb) error_cb(err, false, id);
        });
    }

    /** Terminate the connection (from the worker thread). */
    void worker_terminate(const conn_t &conn);
    /** Terminate the connection (from the dispatcher thread). */
    void disp_terminate(const conn_t &conn);

    /** Should be implemented by derived class to return a new Conn object. */
    virtual Conn *create_conn() = 0;
    /** Called when new data is available. */
    virtual void on_read(const conn_t &) {}
    /** Called when the underlying connection is established. */
    virtual void on_worker_setup(const conn_t &) {}
    /** Called when the underlying connection is established. */
    virtual void on_dispatcher_setup(const conn_t &) {}
    /** Called when the underlying connection breaks. */
    virtual void on_worker_teardown(const conn_t &conn) {
        if (conn->worker) conn->worker->unfeed();
        if (conn->tls) conn->tls->shutdown();
        conn->ev_socket.clear();
        conn->send_buffer.get_queue().unreg_handler();
    }
    /** Called when the underlying connection breaks. */
    virtual void on_dispatcher_teardown(const conn_t &) {}

    private:
    const int max_listen_backlog;
    const double conn_server_timeout;
    const size_t recv_chunk_size;
    const size_t max_recv_buff_size;
    const size_t max_send_buff_size;
    tls_context_t tls_ctx;

    conn_callback_t conn_cb;
    error_callback_t error_cb;

    /* owned by the dispatcher */
    FdEvent ev_listen;
    std::unordered_map<int, conn_t> pool;
    int listen_fd;  /**< for accepting new network connections */

    void update_conn(const conn_t &conn, bool connected) {
        user_tcall->async_call([this, conn, connected](ThreadCall::Handle &) {
            bool ret = !conn_cb || conn_cb(conn, connected);
            if (connected)
            {
                if (enable_tls)
                {
                    conn->worker->get_tcall()->async_call([this, conn, ret](ThreadCall::Handle &) {
                        if (conn->is_terminated()) return;
                        if (ret)
                        {
                            conn->recv_data_func = Conn::_recv_data_tls;
                            conn->ev_socket.del();
                            conn->ev_socket.add(FdEvent::READ | FdEvent::WRITE);
                        }
                        else worker_terminate(conn);
                    });
                }
                else
                    conn->worker->get_tcall()->async_call([conn](ThreadCall::Handle &) {
                        if (conn->is_terminated()) return;
                        conn->ev_socket.add(FdEvent::READ | FdEvent::WRITE);
                    });
            }
        });
    }

    protected:
    class Worker {
        EventContext ec;
        ThreadCall tcall;
        BoxObj<ThreadCall> exit_tcall; /** only used by the dispatcher thread */
        std::thread handle;
        bool disp_flag;
        std::atomic<size_t> nconn;
        ConnPool::worker_error_callback_t on_fatal_error;

        public:

        Worker(): tcall(ec), disp_flag(false), nconn(0) {}

        void set_error_callback(ConnPool::worker_error_callback_t _on_error) {
            on_fatal_error = std::move(_on_error);
        }

        void error_callback(const std::exception_ptr err) const {
            on_fatal_error(err);
        }

        /* the following functions are called by the dispatcher */
        void start() {
            handle = std::thread([this]() {
                sigset_t mask;
                sigfillset(&mask);
                pthread_sigmask(SIG_BLOCK, &mask, NULL);
                ec.dispatch();
            });
        }

        void enable_send_buffer(const conn_t &conn, int client_fd) {
            conn->send_buffer.get_queue()
                    .reg_handler(this->ec, [conn, client_fd]
                                (MPSCWriteBuffer::queue_t &) {
                if (conn->ready_send)
                {
                    conn->ev_socket.del();
                    conn->ev_socket.add((conn->ready_recv ? 0 : FdEvent::READ) |
                                        FdEvent::WRITE);
                    conn->send_data_func(conn, client_fd, FdEvent::WRITE);
                }
                return false;
            });
        }

        void feed(const conn_t &conn, int client_fd) {
            /* the caller should finalize all the preparation */
            tcall.async_call([this, conn, client_fd](ThreadCall::Handle &) {
                try {
                    conn->ev_socket = FdEvent(ec, client_fd, [this, conn](int fd, int what) {
                        try {
                            if (what & FdEvent::READ)
                                conn->recv_data_func(conn, fd, what);
                            else
                                conn->send_data_func(conn, fd, what);
                        } catch (...) {
                            conn->cpool->recoverable_error(std::current_exception(), -1);
                            conn->cpool->worker_terminate(conn);
                        }
                    });
                    auto cpool = conn->cpool;
                    if (cpool->enable_tls)
                    {
                        conn->tls = new TLS(
                                cpool->tls_ctx, client_fd,
                                conn->mode == Conn::ConnMode::PASSIVE);
                        conn->send_data_func = Conn::_send_data_tls_handshake;
                        conn->recv_data_func = Conn::_recv_data_tls_handshake;
                        conn->ev_socket.add(FdEvent::READ | FdEvent::WRITE);
                    }
                    else
                    {
                        conn->send_data_func = Conn::_send_data;
                        conn->recv_data_func = Conn::_recv_data;
                        enable_send_buffer(conn, client_fd);
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
                    assert(conn->fd != -1);
                    assert(conn->worker == this);
                    SALTICIDAE_LOG_DEBUG("worker %x got %s",
                            std::this_thread::get_id(),
                            std::string(*conn).c_str());
                    nconn++;
                } catch (...) { on_fatal_error(std::current_exception()); }
            });
        }

        void unfeed() { nconn--; }

        void stop() {
            tcall.async_call([this](ThreadCall::Handle &) { ec.stop(); });
        }

        void disp_stop() {
            assert(disp_flag && exit_tcall);
            exit_tcall->async_call([this](ThreadCall::Handle &) { ec.stop(); });
        }

        std::thread &get_handle() { return handle; }
        const EventContext &get_ec() { return ec; }
        ThreadCall *get_tcall() { return &tcall; }
        void set_dispatcher() {
            disp_flag = true;
            exit_tcall = new ThreadCall(ec);
        }
        bool is_dispatcher() const { return disp_flag; }
        size_t get_nconn() { return nconn; }
        void stop_tcall() { tcall.stop(); }
    };

    private:
    /* related to workers */
    size_t nworker;
    salticidae::BoxObj<Worker[]> workers;

    void accept_client(int, int);
    void conn_server(const conn_t &conn, int, int);
    conn_t add_conn(const conn_t &conn);
    void del_conn(const conn_t &conn);
    void release_conn(const conn_t &conn);

    Worker &select_worker() {
        size_t idx = 0;
        size_t best = workers[idx].get_nconn();
        for (size_t i = 0; i < nworker; i++)
        {
            size_t t = workers[i].get_nconn();
            if (t < best)
            {
                best = t;
                idx = i;
            }
        }
        return workers[idx];
    }

    public:
    const bool enable_tls;

    class Config {
        friend class ConnPool;
        int _max_listen_backlog;
        double _conn_server_timeout;
        size_t _recv_chunk_size;
        size_t _max_recv_buff_size;
        size_t _max_send_buff_size;
        size_t _nworker;
        bool _enable_tls;
        std::string _tls_cert_file;
        std::string _tls_key_file;
        RcObj<X509> _tls_cert;
        RcObj<PKey> _tls_key;
        bool _tls_skip_ca_check;
        SSL_verify_cb _tls_verify_callback;

        public:
        Config():
            _max_listen_backlog(10),
            _conn_server_timeout(2),
            _recv_chunk_size(4096),
            _max_recv_buff_size(4096),
            _max_send_buff_size(0),
            _nworker(1),
            _enable_tls(false),
            _tls_cert_file(""),
            _tls_key_file(""),
            _tls_cert(nullptr),
            _tls_key(nullptr),
            _tls_skip_ca_check(true),
            _tls_verify_callback(nullptr) {}

        Config &max_listen_backlog(int x) {
            _max_listen_backlog = x;
            return *this;
        }

        Config &conn_server_timeout(double x) {
            _conn_server_timeout = x;
            return *this;
        }

        Config &recv_chunk_size(size_t x) {
            _recv_chunk_size = x;
            return *this;
        }

        Config &nworker(size_t x) {
            _nworker = std::max((size_t)1, x);
            return *this;
        }

        Config &max_recv_buff_size(size_t x) {
            _max_recv_buff_size = x;
            return *this;
        }

        Config &max_send_buff_size(size_t x) {
            _max_send_buff_size = x;
            return *this;
        }

        Config &enable_tls(bool x) {
            _enable_tls = x;
            return *this;
        }

        Config &tls_cert_file(const std::string &x) {
            _tls_cert_file = x;
            return *this;
        }

        Config &tls_key_file(const std::string &x) {
            _tls_key_file = x;
            return *this;
        }

        Config &tls_cert(X509 *x) {
            _tls_cert = x;
            return *this;
        }

        Config &tls_key(PKey *x) {
            _tls_key = x;
            return *this;
        }

        Config &tls_skip_ca_check(bool *x) {
            _tls_skip_ca_check = x;
            return *this;
        }

        Config &tls_verify_callback(SSL_verify_cb x) {
            _tls_verify_callback = x;
            return *this;
        }
    };

    ConnPool(const EventContext &ec, const Config &config):
            system_state(0), ec(ec),
            async_id(0),
            max_listen_backlog(config._max_listen_backlog),
            conn_server_timeout(config._conn_server_timeout),
            recv_chunk_size(config._recv_chunk_size),
            max_recv_buff_size(config._max_recv_buff_size),
            max_send_buff_size(config._max_send_buff_size),
            tls_ctx(nullptr),
            listen_fd(-1),
            nworker(config._nworker),
            enable_tls(config._enable_tls) {
        if (enable_tls)
        {
            tls_ctx = new TLSContext();
            if (config._tls_cert)
                tls_cert = config._tls_cert;
            else
                tls_cert = new X509(X509::create_from_pem_file(config._tls_cert_file));
            tls_ctx->use_cert(*tls_cert);
            if (config._tls_key)
                tls_ctx->use_privkey(*config._tls_key);
            else
                tls_ctx->use_privkey_file(config._tls_key_file);
            tls_ctx->set_verify(config._tls_skip_ca_check, config._tls_verify_callback);
            if (!tls_ctx->check_privkey())
                throw SalticidaeError(SALTI_ERROR_TLS_KEY_NOT_MATCH);
        }
        workers = new Worker[nworker];
        user_tcall = new ThreadCall(ec);
        disp_ec = workers[0].get_ec();
        disp_tcall = workers[0].get_tcall();
        workers[0].set_dispatcher();
        disp_error_cb = [this](const std::exception_ptr err) {
            workers[0].stop_tcall();
            user_tcall->async_call([this, err](ThreadCall::Handle &) {
                for (size_t i = 1; i < nworker; i++)
                    workers[i].stop();
                if (error_cb) error_cb(err, true, -1);
            });
        };

        worker_error_cb = [this](const std::exception_ptr err) {
            disp_tcall->async_call([this, err](ThreadCall::Handle &) {
                // forward to the dispatcher
                disp_error_cb(err);
            });
        };
        for (size_t i = 0; i < nworker; i++)
        {
            auto &worker = workers[i];
            if (worker.is_dispatcher())
                worker.set_error_callback(disp_error_cb);
            else
                worker.set_error_callback(worker_error_cb);
        }
    }

    ~ConnPool() { stop(); }

    ConnPool(const ConnPool &) = delete;
    ConnPool(ConnPool &&) = delete;

    void start() {
        std::atomic_thread_fence(std::memory_order_acq_rel);
        if (system_state) return;
        SALTICIDAE_LOG_INFO("starting all threads...");
        for (size_t i = 0; i < nworker; i++)
            workers[i].start();
        system_state = 1;
    }

    void stop_workers() {
        if (system_state != 1) return;
        system_state = 2;
        SALTICIDAE_LOG_INFO("stopping all threads...");
        /* stop the dispatcher */
        workers[0].disp_stop();
        workers[0].get_handle().join();
        /* stop all workers */
        for (size_t i = 1; i < nworker; i++)
            workers[i].stop();
        /* join all worker threads */
        for (size_t i = 1; i < nworker; i++)
            workers[i].get_handle().join();
        for (auto it: pool)
        {
            auto &conn = it.second;
            on_worker_teardown(conn);
            //conn->stop();
            conn->set_terminated();
            release_conn(conn);
        }
    }

    void stop() {
        stop_workers();
        if (listen_fd != -1)
        {
            close(listen_fd);
            listen_fd = -1;
        }
    }

    /** Actively connect to remote addr. */
    conn_t connect_sync(const NetAddr &addr) {
        auto ret = *(static_cast<conn_t *>(
                    disp_tcall->call([this, addr](ThreadCall::Handle &h) {
            conn_t conn;
            conn = _connect(addr);
            h.set_result(conn);
        }).get()));
        return ret;
    }

    /** Actively connect to remote addr (async). */
    int32_t connect(const NetAddr &addr) {
        auto id = gen_async_id();
        disp_tcall->async_call([this, addr, id](ThreadCall::Handle &) {
            try {
                _connect(addr);
            } catch (...) {
                this->recoverable_error(std::current_exception(), id);
            }
        });
        return id;
    }


    /** Listen for passive connections (connection initiated from remote).
     * Does not need to be called if do not want to accept any passive
     * connections. */
    void listen(NetAddr listen_addr) {
        disp_tcall->call([this, listen_addr](ThreadCall::Handle &) {
            _listen(listen_addr);
        }).get();
    }

    template<typename Func>
    void reg_conn_handler(Func &&cb) { conn_cb = std::forward<Func>(cb); }

    template<typename Func>
    void reg_error_handler(Func &&cb) { error_cb = std::forward<Func>(cb); }

    void terminate(const conn_t &conn) {
        disp_tcall->async_call([this, conn](ThreadCall::Handle &) {
            try {
                disp_terminate(conn);
            } catch (...) {
                disp_error_cb(std::current_exception());
            }
        });
    }

    const X509 *get_cert() const { return tls_cert.get(); }
};

}

#endif

#endif
