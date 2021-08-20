#include "salticidae/config.h"
#ifdef SALTICIDAE_CBINDINGS
#include "salticidae/event.h"

#if defined(__linux__)
const uint64_t salticidae::NotifyFd::dummy = 1;
#endif

extern "C" {

eventcontext_t *eventcontext_new() { return new eventcontext_t(); }

void eventcontext_free(eventcontext_t *self) { delete self; }

void eventcontext_dispatch(eventcontext_t *self) { return self->dispatch(); }

void eventcontext_stop(eventcontext_t *self) { return self->stop(); }

sigev_t *sigev_new(const eventcontext_t *self, sigev_callback_t callback, void *userdata) {
    return new sigev_t(*self, [=](int signum) {
        callback(signum, userdata);
    });
}

threadcall_t *threadcall_new(const eventcontext_t *ec) { return new threadcall_t(*ec); }

void threadcall_free(threadcall_t *self) { delete self; }

void threadcall_async_call(threadcall_t *self, threadcall_callback_t callback, void *userdata) {
    self->async_call([=](salticidae::ThreadCall::Handle &h) {
        callback(&h, userdata);
    });
}

const eventcontext_t *threadcall_get_ec(const threadcall_t *self) {
    return &self->get_ec();
}

void sigev_free(sigev_t *self) { delete self; }

void sigev_add(sigev_t *self, int signum) { self->add(signum); }

void sigev_del(sigev_t *self) { self->del(); }

void sigev_clear(sigev_t *self) { self->clear(); }

const eventcontext_t *sigev_get_ec(const sigev_t *self) {
    return &self->get_ec();
}

timerev_t *timerev_new(const eventcontext_t *ec, timerev_callback_t callback, void *userdata) {
    return new timerev_t(*ec, [=](salticidae::TimerEvent &ev) {
        callback(&ev, userdata);
    });
}

void timerev_set_callback(timerev_t *self, timerev_callback_t callback, void *userdata) {
    self->set_callback([=](salticidae::TimerEvent &ev) {
        callback(&ev, userdata);
    });
}


const eventcontext_t *timerev_get_ec(const timerev_t *self) {
    return &self->get_ec();
}

void timerev_free(timerev_t *self) { delete self; }

void timerev_add(timerev_t *self, double t_sec) { self->add(t_sec); }

void timerev_del(timerev_t *self) { self->del(); }

void timerev_clear(timerev_t *self) { self->clear(); }

mpscqueue_t *mpscqueue_new() { return new mpscqueue_t(); }

void mpscqueue_free(mpscqueue_t *self) { delete self; }

void mpscqueue_reg_handler(mpscqueue_t *self, const eventcontext_t *ec, mpscqueue_callback_t callback, void *userdata) {
    self->reg_handler(*ec, [=](mpscqueue_t &q) {
        return callback(&q, userdata);
    });
}

void mpscqueue_unreg_handler(mpscqueue_t *self) { self->unreg_handler(); }

bool mpscqueue_enqueue(mpscqueue_t *self, void *elem, bool unbounded) {
    return self->enqueue(elem, unbounded);
}

bool mpscqueue_try_dequeue(mpscqueue_t *self, void **elem) {
    return self->try_dequeue(*elem);
}

void mpscqueue_set_capacity(mpscqueue_t *self, size_t cap) {
    self->set_capacity(cap);
}

}

#endif
