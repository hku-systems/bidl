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

#ifndef _SALTICIDAE_EVENT_H
#define _SALTICIDAE_EVENT_H

#ifdef __cplusplus
#include <condition_variable>
#include <unistd.h>
#include <uv.h>

#include "salticidae/type.h"
#include "salticidae/queue.h"
#include "salticidae/util.h"
#include "salticidae/ref.h"

namespace salticidae {

static void _on_uv_handle_close(uv_handle_t *h) { if (h) delete h; }

struct _event_context_deleter {
    constexpr _event_context_deleter() = default;
    static void _on_uv_walk(uv_handle_t *handle, void *) {
        if (!uv_is_closing(handle))
            uv_close(handle, _on_uv_handle_close);
    }
    void operator()(uv_loop_t *ptr) {
        if (ptr != nullptr)
        {
            uv_walk(ptr, _on_uv_walk, nullptr);
            uv_run(ptr, UV_RUN_DEFAULT);
            if (uv_loop_close(ptr))
                SALTICIDAE_LOG_WARN("failed to close libuv loop");
            delete ptr;
        }
    }
};

using _event_context_ot = ArcObj<uv_loop_t, _event_context_deleter>;

class EventContext: public _event_context_ot {
    public:
    EventContext(): _event_context_ot(new uv_loop_t()) {
        if (uv_loop_init(get()) < 0)
        {
            delete obj;
            obj = nullptr;
            throw SalticidaeError(SALTI_ERROR_LIBUV_INIT);
        }
    }
    EventContext(uv_loop_t *eb): _event_context_ot(eb) {}
    EventContext(const EventContext &) = default;
    EventContext(EventContext &&) = default;
    EventContext &operator=(const EventContext &) = default;
    EventContext &operator=(EventContext &&) = default;
    void dispatch() const {
        // TODO: improve this loop
        uv_run(get(), UV_RUN_DEFAULT);
    }
    void stop() const { uv_stop(get()); }
};

class FdEvent {
    public:
    using callback_t = std::function<void(int fd, int events)>;
    static const int READ = UV_READABLE;
    static const int WRITE = UV_WRITABLE;
    static const int ERROR = 1 << 30;

    protected:
    EventContext ec;
    int fd;
    uv_poll_t *ev_fd;
    callback_t callback;

    static inline void fd_then(uv_poll_t *h, int status, int events) {
        if (status != 0)
            events |= ERROR;
        auto event = static_cast<FdEvent *>(h->data);
        event->callback(event->fd, events);
    }

    public:
    FdEvent(): ec(nullptr), ev_fd(nullptr) {}
    FdEvent(const EventContext &ec, int fd, callback_t callback):
            ec(ec), fd(fd), ev_fd(new uv_poll_t()),
            callback(std::move(callback)) {
        if (uv_poll_init(ec.get(), ev_fd, fd) < 0)
            throw SalticidaeError(SALTI_ERROR_LIBUV_INIT);
        ev_fd->data = this;
    }

    FdEvent(const FdEvent &) = delete;
    FdEvent(FdEvent &&other):
            ec(std::move(other.ec)), fd(other.fd), ev_fd(other.ev_fd),
            callback(std::move(other.callback)) {
        other.ev_fd = nullptr;
        if (ev_fd != nullptr)
            ev_fd->data = this;
    }
    
    void swap(FdEvent &other) {
        std::swap(ec, other.ec);
        std::swap(fd, other.fd);
        std::swap(ev_fd, other.ev_fd);
        std::swap(callback, other.callback);
        if (ev_fd != nullptr)
            ev_fd->data = this;
        if (other.ev_fd != nullptr)
            other.ev_fd->data = &other;
    }

    FdEvent &operator=(FdEvent &&other) {
        if (this != &other)
        {
            FdEvent tmp(std::move(other));
            tmp.swap(*this);
        }
        return *this;
    }

    ~FdEvent() { clear(); }

    void clear() {
        if (ev_fd != nullptr)
        {
            uv_poll_stop(ev_fd);
            uv_close((uv_handle_t *)ev_fd, _on_uv_handle_close);
            ev_fd = nullptr;
        }
        callback = nullptr;
    }

    void set_callback(callback_t _callback) {
        callback = _callback;
    }

    void add(int events) {
        assert(ev_fd != nullptr);
        if (uv_poll_start(ev_fd, events, FdEvent::fd_then) < 0)
            throw SalticidaeError(SALTI_ERROR_LIBUV_START);
    }

    void del() {
        if (ev_fd != nullptr) uv_poll_stop(ev_fd);
    }

    operator bool() const { return ev_fd != nullptr; }
};


class TimerEvent {
    public:
    using callback_t = std::function<void(TimerEvent &)>;

    protected:
    EventContext ec;
    uv_timer_t *ev_timer;
    callback_t callback;

    static inline void timer_then(uv_timer_t *h) {
        auto event = static_cast<TimerEvent *>(h->data);
        event->callback(*event);
    }

    public:
    TimerEvent(): ec(nullptr), ev_timer(nullptr) {}
    TimerEvent(const EventContext &ec, callback_t callback):
            ec(ec), ev_timer(new uv_timer_t()),
            callback(std::move(callback)) {
        if (uv_timer_init(ec.get(), ev_timer) < 0)
            throw SalticidaeError(SALTI_ERROR_LIBUV_INIT);
        ev_timer->data = this;
    }

    TimerEvent(const TimerEvent &) = delete;
    TimerEvent(TimerEvent &&other):
            ec(std::move(other.ec)), ev_timer(other.ev_timer),
            callback(std::move(other.callback)) {
        other.ev_timer = nullptr;
        if (ev_timer != nullptr)
            ev_timer->data = this;
    }

    void swap(TimerEvent &other) {
        std::swap(ec, other.ec);
        std::swap(ev_timer, other.ev_timer);
        std::swap(callback, other.callback);
        if (ev_timer != nullptr)
            ev_timer->data = this;
        if (other.ev_timer != nullptr)
            other.ev_timer->data = &other;
    }

    TimerEvent &operator=(TimerEvent &&other) {
        if (this != &other)
        {
            TimerEvent tmp(std::move(other));
            tmp.swap(*this);
        }
        return *this;
    }

    ~TimerEvent() { clear(); }

    void clear() {
        if (ev_timer != nullptr)
        {
            uv_timer_stop(ev_timer);
            uv_close((uv_handle_t *)ev_timer, _on_uv_handle_close);
            ev_timer = nullptr;
        }
        callback = nullptr;
    }

    void set_callback(callback_t _callback) {
        callback = _callback;
    }

    void add(double t_sec) {
        assert(ev_timer != nullptr);
        if (uv_timer_start(ev_timer, TimerEvent::timer_then, uint64_t(t_sec * 1000), 0) < 0)
            throw SalticidaeError(SALTI_ERROR_LIBUV_START);
    }

    void del() {
        if (ev_timer != nullptr) uv_timer_stop(ev_timer);
    }

    operator bool() const { return ev_timer != nullptr; }

    const EventContext &get_ec() const { return ec; }
};

class TimedFdEvent: public FdEvent, public TimerEvent {
    public:
    static const int TIMEOUT = 1 << 29;

    private:
    FdEvent::callback_t callback;
    uint64_t timeout;

    static inline void timer_then(uv_timer_t *h) {
        auto event = static_cast<TimedFdEvent *>(h->data);
        event->FdEvent::del();
        event->callback(event->fd, TIMEOUT);
    }

    static inline void fd_then(uv_poll_t *h, int status, int events) {
        if (status != 0)
            events |= ERROR;
        auto event = static_cast<TimedFdEvent *>(h->data);
        event->TimerEvent::del();
        if (uv_timer_start(event->ev_timer, TimedFdEvent::timer_then,
                            event->timeout, 0) < 0)
            throw SalticidaeError(SALTI_ERROR_LIBUV_START);
        event->callback(event->fd, events);
    }

    public:
    TimedFdEvent() = default;
    TimedFdEvent(const EventContext &ec, int fd, FdEvent::callback_t callback):
            FdEvent(ec, fd, FdEvent::callback_t()),
            TimerEvent(ec, TimerEvent::callback_t()),
            callback(std::move(callback)) {
        ev_fd->data = this;
        ev_timer->data = this;
    }

    TimedFdEvent(TimedFdEvent &&other):
            FdEvent(static_cast<FdEvent &&>(other)),
            TimerEvent(static_cast<TimerEvent &&>(other)),
            callback(std::move(other.callback)),
            timeout(other.timeout) {
        if (ev_fd != nullptr)
        {
            ev_timer->data = this;
            ev_fd->data = this;
        }
    }

    void swap(TimedFdEvent &other) {
        std::swap(static_cast<FdEvent &>(*this), static_cast<FdEvent &>(other));
        std::swap(static_cast<TimerEvent &>(*this), static_cast<TimerEvent &>(other));
        std::swap(callback, other.callback);
        std::swap(timeout, other.timeout);
        if (ev_fd != nullptr)
            ev_fd->data = ev_timer->data = this;
        if (other.ev_fd != nullptr)
            other.ev_fd->data = other.ev_timer->data = &other;
    }

    TimedFdEvent &operator=(TimedFdEvent &&other) {
        if (this != &other)
        {
            TimedFdEvent tmp(std::move(other));
            tmp.swap(*this);
        }
        return *this;
    }

    ~TimedFdEvent() { clear(); }

    void clear() {
        TimerEvent::clear();
        FdEvent::clear();
        callback = nullptr;
    }

    using FdEvent::set_callback;

    void add(int events) = delete;
    void add(double t_sec) = delete;

    void set_callback(FdEvent::callback_t _callback) {
        callback = _callback;
    }

    void add(int events, double t_sec) {
        assert(ev_fd != nullptr && ev_timer != nullptr);
        auto ret1 = uv_timer_start(ev_timer, TimedFdEvent::timer_then,
                                    timeout = uint64_t(t_sec * 1000), 0);
        auto ret2 = uv_poll_start(ev_fd, events, TimedFdEvent::fd_then);
        if (ret1 < 0 || ret2 < 0)
            throw SalticidaeError(SALTI_ERROR_LIBUV_START);
    }

    void del() {
        TimerEvent::del();
        FdEvent::del();
    }

    operator bool() const { return ev_fd != nullptr; }
};

class SigEvent {
    public:
    using callback_t = std::function<void(int signum)>;
    private:
    EventContext ec;
    uv_signal_t *ev_sig;
    callback_t callback;
    static inline void sig_then(uv_signal_t *h, int signum) {
        auto event = static_cast<SigEvent *>(h->data);
        event->callback(signum);
    }

    public:
    SigEvent(): ec(nullptr), ev_sig(nullptr) {}
    SigEvent(const EventContext &ec, callback_t callback):
            ec(ec), ev_sig(new uv_signal_t()),
            callback(std::move(callback)) {
        if (uv_signal_init(ec.get(), ev_sig) < 0)
            throw SalticidaeError(SALTI_ERROR_LIBUV_INIT);
        ev_sig->data = this;
    }

    SigEvent(const SigEvent &) = delete;
    SigEvent(SigEvent &&other):
            ec(std::move(other.ec)), ev_sig(other.ev_sig),
            callback(std::move(other.callback)) {
        other.ev_sig = nullptr;
        if (ev_sig != nullptr)
            ev_sig->data = this;
    }

    void swap(SigEvent &other) {
        std::swap(ec, other.ec);
        std::swap(ev_sig, other.ev_sig);
        std::swap(callback, other.callback);
        if (ev_sig != nullptr)
            ev_sig->data = this;
        if (other.ev_sig != nullptr)
            other.ev_sig->data = &other;
    }

    SigEvent &operator=(SigEvent &&other) {
        if (this != &other)
        {
            SigEvent tmp(std::move(other));
            tmp.swap(*this);
        }
        return *this;
    }

    ~SigEvent() { clear(); }

    void clear() {
        if (ev_sig != nullptr)
        {
            uv_signal_stop(ev_sig);
            uv_close((uv_handle_t *)ev_sig, _on_uv_handle_close);
            ev_sig = nullptr;
        }
        callback = nullptr;
    }

    void set_callback(callback_t _callback) {
        callback = _callback;
    }

    void add(int signum) {
        assert(ev_sig != nullptr);
        if (uv_signal_start(ev_sig, SigEvent::sig_then, signum) < 0)
            throw SalticidaeError(SALTI_ERROR_LIBUV_START);
    }

    void add_once(int signum) {
        assert(ev_sig != nullptr);
        if (uv_signal_start_oneshot(ev_sig, SigEvent::sig_then, signum) < 0)
            throw SalticidaeError(SALTI_ERROR_LIBUV_START);
    }

    void del() {
        if (ev_sig != nullptr) uv_signal_stop(ev_sig);
    }

    operator bool() const { return ev_sig != nullptr; }

    const EventContext &get_ec() const { return ec; }
};

class CheckEvent {
    public:
    using callback_t = std::function<void()>;
    private:
    EventContext ec;
    uv_check_t *ev_check;
    callback_t callback;
    static inline void check_then(uv_check_t *h) {
        auto event = static_cast<CheckEvent *>(h->data);
        event->callback();
    }

    public:
    CheckEvent(): ec(nullptr), ev_check(nullptr) {}
    CheckEvent(const EventContext &ec, callback_t callback):
            ec(ec), ev_check(new uv_check_t()),
            callback(std::move(callback)) {
        if (uv_check_init(ec.get(), ev_check) < 0)
            throw SalticidaeError(SALTI_ERROR_LIBUV_INIT);
        ev_check->data = this;
    }

    CheckEvent(const CheckEvent &) = delete;
    CheckEvent(CheckEvent &&other):
            ec(std::move(other.ec)), ev_check(other.ev_check),
            callback(std::move(other.callback)) {
        other.ev_check = nullptr;
        if (ev_check != nullptr)
            ev_check->data = this;
    }

    void swap(CheckEvent &other) {
        std::swap(ec, other.ec);
        std::swap(ev_check, other.ev_check);
        std::swap(callback, other.callback);
        if (ev_check != nullptr)
            ev_check->data = this;
        if (other.ev_check != nullptr)
            other.ev_check->data = &other;
    }

    CheckEvent &operator=(CheckEvent &&other) {
        if (this != &other)
        {
            CheckEvent tmp(std::move(other));
            tmp.swap(*this);
        }
        return *this;
    }

    ~CheckEvent() { clear(); }

    void clear() {
        if (ev_check != nullptr)
        {
            uv_check_stop(ev_check);
            uv_close((uv_handle_t *)ev_check, _on_uv_handle_close);
            ev_check = nullptr;
        }
        callback = nullptr;
    }

    void set_callback(callback_t _callback) {
        callback = _callback;
    }

    void add() {
        assert(ev_check != nullptr);
        if (uv_check_start(ev_check, CheckEvent::check_then) < 0)
            throw SalticidaeError(SALTI_ERROR_LIBUV_START);
    }

    void del() {
        if (ev_check != nullptr) uv_check_stop(ev_check);
    }

    operator bool() const { return ev_check != nullptr; }

    const EventContext &get_ec() const { return ec; }
};

template<typename T>
class ThreadNotifier {
    std::condition_variable cv;
    std::mutex mlock;
    mutex_ul_t ul;
    bool ready;
    T data;
    public:
    ThreadNotifier(): ul(mlock), ready(false) {}
    T wait() {
        cv.wait(ul, [this]{ return ready; });
        return std::move(data);
    }
    void notify(T &&_data) {
        mutex_lg_t _(mlock);
        ready = true;
        data = std::move(_data);
        cv.notify_all();
    }
};

#if defined(__linux__)
#include <sys/eventfd.h>

class NotifyFd {
    int fd;
    static const uint64_t dummy;
    public:
    NotifyFd(): fd(eventfd(0, EFD_NONBLOCK)) {
        if (fd < 0) throw SalticidaeError(SALTI_ERROR_FD);
    }
    bool reset() {
        uint64_t _;
        return read(fd, &_, 8) == 8;
    }
    void notify() { write(fd, &dummy, 8); }
    int read_fd() { return fd; }
    ~NotifyFd() { close(fd); }
};

#elif defined(__APPLE__)
// NOTE: using kqueue/kevent with EVFILT_USER is optimal, but libuv doesn't
// seem to offer such interface for such user level kevent (and its
// identifier). Thus, we downgrade to pipe-based solution on OSX/BSD system.

class NotifyFd {
    int fds[0];
    uint8_t dummy[8];
    public:
    NotifyFd() {
        if (pipe(fds) < 0 ||
            fcntl(fds[0], F_SETFL, O_NONBLOCK))
            throw SalticidaeError(SALTI_ERROR_FD);
    }
    bool reset() {
        // clear the pipe buffer (not atomically)
        while (read(fds[0], dummy, 8) > 0);
        // may not work for MPMC, but salticidae currently doesn't use that
        return true;
    }
    void notify() {
        write(fds[1], dummy, 8);
    }
    int read_fd() { return fds[0]; }
    ~NotifyFd() {
        close(fds[0]);
        close(fds[1]);
    }
};

#else
#warning "platform not supported!"
#endif

template<typename T>
class MPSCQueueEventDriven: public MPSCQueue<T> {
    private:
    std::atomic<bool> wait_sig;
    NotifyFd nfd;
    FdEvent ev;

    public:
    MPSCQueueEventDriven():
            wait_sig(true) {}
    ~MPSCQueueEventDriven() { unreg_handler(); }

    template<typename Func>
    void reg_handler(const EventContext &ec, Func &&func) {
        ev = FdEvent(ec, nfd.read_fd(),
                    [this, func=std::forward<Func>(func)](int, int) {
                    nfd.reset();
                    // the only undesirable case is there are some new items
                    // enqueued before recovering wait_sig to true, so the consumer
                    // won't be notified. In this case, no enqueuing thread will
                    // get to write(fd). Then store(true) must happen after all exchange(false),
                    // since all enqueue operations are finalized, the dequeue should be able
                    // to see those enqueued values in func()
                    wait_sig.exchange(true, std::memory_order_acq_rel);
                    if (func(*this))
                        nfd.notify();
                });
        ev.add(FdEvent::READ);
    }

    void unreg_handler() { ev.clear(); }

    template<typename U>
    bool enqueue(U &&e, bool unbounded = true) {
        if (!MPSCQueue<T>::enqueue(std::forward<U>(e), unbounded))
            return false;
        // memory barrier here, so any load/store in enqueue must be finialized
        if (wait_sig.exchange(false, std::memory_order_acq_rel))
        {
            //SALTICIDAE_LOG_DEBUG("mpsc notify");
            nfd.notify();
        }
        return true;
    }

    template<typename U> bool try_enqueue(U &&e) = delete;
};

// NOTE: the MPMC implementation below hasn't been heavily tested.
template<typename T>
class MPMCQueueEventDriven: public MPMCQueue<T> {
    private:
    std::atomic<bool> wait_sig;
    NotifyFd nfd;
    std::vector<FdEvent> evs;

    public:
    MPMCQueueEventDriven():
            wait_sig(true) {}
    ~MPMCQueueEventDriven() { unreg_handlers(); }

    // this function is *NOT* thread-safe
    template<typename Func>
    void reg_handler(const EventContext &ec, Func &&func) {
        FdEvent ev(ec, nfd.read_fd(), [this, func=std::forward<Func>(func)](int, int) {
            if (!nfd.reset()) return;
            // only one consumer should be here a a time
            wait_sig.exchange(true, std::memory_order_acq_rel);
            if (func(*this))
                nfd.notify();
        });
        ev.add(FdEvent::READ);
        evs.push_back(std::move(ev));
    }

    void unreg_handlers() { evs.clear(); }

    template<typename U>
    bool enqueue(U &&e, bool unbounded = true) {
        if (!MPMCQueue<T>::enqueue(std::forward<U>(e), unbounded))
            return false;
        // memory barrier here, so any load/store in enqueue must be finialized
        if (wait_sig.exchange(false, std::memory_order_acq_rel))
        {
            //SALTICIDAE_LOG_DEBUG("mpmc notify");
            nfd.notify();
        }
        return true;
    }

    template<typename U> bool try_enqueue(U &&e) = delete;
};

class ThreadCall {
    public: class Handle;
    private:
    EventContext ec;
    using queue_t = MPSCQueueEventDriven<Handle *>;
    queue_t q;
    bool stopped;

    public:
    struct Result {
        void *data;
        std::exception_ptr error;
        std::function<void(void *)> deleter;
        Result(): data(nullptr) {}
        Result(void *data, std::function<void(void *)> &&deleter):
            data(data), deleter(std::move(deleter)) {}
        ~Result() { if (data != nullptr) deleter(data); }
        Result(const Result &) = delete;
        Result(Result &&other):
                data(other.data),
                error(std::move(other.error)),
                deleter(std::move(other.deleter)) {
            other.data = nullptr;
        }
        void swap(Result &other) {
            std::swap(data, other.data);
            std::swap(error, other.error);
            std::swap(deleter, other.deleter);
        }
        Result &operator=(const Result &other) = delete;
        Result &operator=(Result &&other) {
            if (this != &other)
            {
                Result tmp(std::move(other));
                tmp.swap(*this);
            }
            return *this;
        }
        void *get() {
            if (error) std::rethrow_exception(error);
            return data;
        }
    };
    class Handle {
        std::function<void(Handle &)> callback;
        ThreadNotifier<Result> * notifier;
        Result result;
        friend ThreadCall;
        public:
        Handle(): notifier(nullptr) {}
        Handle(const Handle &) = delete;
        void return_sync() {
            if (notifier)
                notifier->notify(std::move(result));
        }
        void exec() {
            callback(*this);
            return_sync();
        }
        template<typename T>
        Result &set_result(T &&data) {
            using _T = std::remove_reference_t<T>;
            return result = Result(new _T(std::forward<T>(data)),
                            [](void *ptr) {delete static_cast<_T *>(ptr);});
        }
    };

    ThreadCall(const ThreadCall &) = delete;
    ThreadCall(ThreadCall &&) = delete;
    ThreadCall(EventContext ec, size_t burst_size = 128): ec(ec), stopped(false) {
        q.reg_handler(ec, [this, burst_size=burst_size](queue_t &q) {
            size_t cnt = 0;
            Handle *h;
            while (q.try_dequeue(h))
            {
                try {
                    if (!stopped) h->exec();
                    else throw SalticidaeError(SALTI_ERROR_NOT_AVAIL);
                } catch (...) {
                    h->set_result(0).error = std::current_exception();
                    h->return_sync();
                }
                delete h;
                if (++cnt == burst_size) return true;
            }
            return false;
        });
    }

    ~ThreadCall() {
        Handle *h;
        while (q.try_dequeue(h)) delete h;
    }

    template<typename Func>
    bool async_call(Func &&callback) {
        auto h = new Handle();
        h->callback = std::forward<Func>(callback);
        q.enqueue(h);
        return true;
    }

    template<typename Func>
    Result call(Func &&callback) {
        auto h = new Handle();
        h->callback = std::forward<Func>(callback);
        ThreadNotifier<Result> notifier;
        h->notifier = &notifier;
        q.enqueue(h);
        return notifier.wait();
    }

    const EventContext &get_ec() const { return ec; }
    void stop() { stopped = true; }
    bool is_stopped() { return stopped; }
};

}

#ifdef SALTICIDAE_CBINDINGS
using eventcontext_t = salticidae::EventContext;
using threadcall_t = salticidae::ThreadCall;
using threadcall_handle_t = salticidae::ThreadCall::Handle;
using sigev_t = salticidae::SigEvent;
using timerev_t = salticidae::TimerEvent;
using mpscqueue_t = salticidae::MPSCQueueEventDriven<void *>;
#endif

#else

#include <stdbool.h>
#include "config.h"

#ifdef SALTICIDAE_CBINDINGS
typedef struct eventcontext_t eventcontext_t;
typedef struct threadcall_t threadcall_t;
typedef struct threadcall_handle_t threadcall_handle_t;
typedef struct sigev_t sigev_t;
typedef struct timerev_t timerev_t;
typedef struct mpscqueue_t mpscqueue_t;
#endif

#endif

#ifdef SALTICIDAE_CBINDINGS
#ifdef __cplusplus
extern "C" {
#endif

eventcontext_t *eventcontext_new();
void eventcontext_free(eventcontext_t *self);
void eventcontext_dispatch(eventcontext_t *self);
void eventcontext_stop(eventcontext_t *self);

typedef void (*threadcall_callback_t)(threadcall_handle_t *handle, void *userdata);
threadcall_t *threadcall_new(const eventcontext_t *ec);
void threadcall_free(threadcall_t *self);
void threadcall_async_call(threadcall_t *self, threadcall_callback_t callback, void *userdata);
const eventcontext_t *threadcall_get_ec(const threadcall_t *self);

typedef void (*sigev_callback_t)(int signum, void *);
sigev_t *sigev_new(const eventcontext_t *ec, sigev_callback_t cb, void *userdata);
void sigev_free(sigev_t *self);
void sigev_add(sigev_t *self, int signum);
void sigev_del(sigev_t *self);
void sigev_clear(sigev_t *self);
const eventcontext_t *sigev_get_ec(const sigev_t *self);

typedef void (*timerev_callback_t)(timerev_t *self, void *userdata);
timerev_t *timerev_new(const eventcontext_t *ec, timerev_callback_t callback, void *);
void timerev_set_callback(timerev_t *self, timerev_callback_t callback, void *);
void timerev_free(timerev_t *self);
void timerev_add(timerev_t *self, double t_sec);
void timerev_del(timerev_t *self);
void timerev_clear(timerev_t *self);
const eventcontext_t *timerev_get_ec(const timerev_t *self);

typedef bool (*mpscqueue_callback_t)(mpscqueue_t *self, void *userdata);
mpscqueue_t *mpscqueue_new();
void mpscqueue_free(mpscqueue_t *self);
void mpscqueue_reg_handler(mpscqueue_t *self, const eventcontext_t *ec, mpscqueue_callback_t callback, void *);
void mpscqueue_unreg_handler(mpscqueue_t *self);
bool mpscqueue_enqueue(mpscqueue_t *self, void *elem, bool unbounded);
bool mpscqueue_try_dequeue(mpscqueue_t *self, void **elem);
void mpscqueue_set_capacity(mpscqueue_t *self, size_t cap);

#ifdef __cplusplus
}
#endif
#endif

#endif
