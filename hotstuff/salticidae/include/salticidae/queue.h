#ifndef _SALTICIDAE_QUEUE_H
#define _SALTICIDAE_QUEUE_H

#include <atomic>
#include <vector>
#include <cassert>
#include <thread>

namespace salticidae {

static size_t const cacheline_size = 64;
using cacheline_pad = uint8_t[cacheline_size];

class FreeList {
    public:
    struct Node {
        std::atomic<Node *> next;
        std::atomic<size_t> refcnt;
        std::atomic<bool> freed;
        Node(): next(nullptr), refcnt(1), freed(false) {}
        virtual ~Node() {}
    };

    private:
    std::atomic<Node *> top;

    public:
    FreeList(): top(nullptr) {}
    FreeList(const FreeList &) = delete;
    FreeList(FreeList &&) = delete;

    void release_ref(Node *u) {
        if (u->refcnt.fetch_sub(1, std::memory_order_relaxed) != 1) return;
        for (;;)
        {
            auto t = top.load(std::memory_order_relaxed);
            // repair the next pointer before CAS, otherwise u->next == nullptr
            // could lead to skipping elements
            u->next.store(t, std::memory_order_relaxed);
            // the replacement is ok even if ABA happens
            if (top.compare_exchange_weak(t, u, std::memory_order_release))
            {
                // must have seen freed == true
                u->refcnt.store(1, std::memory_order_relaxed);
                break;
            }
        }
    }

    inline void push(Node *u) {
        u->freed.store(true, std::memory_order_relaxed);
        release_ref(u);
    }

    bool pop(Node *&r) {
        bool loop = true;
        while (loop)
        {
            auto u = top.load(std::memory_order_acquire);
            /* the list is now empty */
            if (u == nullptr) return false;
            auto t = u->refcnt.load(std::memory_order_relaxed);
            /* let's wait for another round if u is a ghost (already popped) */
            if (!t) continue;
            /* otherwise t > 0, so with CAS, the invariant that zero refcnt can
             * never be increased is guaranteed */
            if (u->refcnt.compare_exchange_weak(t, t + 1, std::memory_order_relaxed))
            {
                /* here, nobody is able to change v->next (because v->next is
                 * only changed when pushed) even when ABA happens */
                auto v = u;
                auto nv = u->next.load(std::memory_order_relaxed);
                if (top.compare_exchange_weak(v, nv, std::memory_order_relaxed))
                {
                    /* manage to pop the head */
                    r = u;
                    loop = false;
                    /* do not need to try cas_push here because the current
                     * thread is the only one who can push u back */
                }
                /* release the refcnt and execute the delayed push call if
                 * necessary */
                release_ref(u);
            }
        }
        return true;
    }
};

const size_t MPMCQ_SIZE = 4096;

template<typename T>
class MPMCQueue {
    protected:
    struct Block: public FreeList::Node {
        std::atomic<uint32_t> head;
        cacheline_pad _pad0;
        std::atomic<uint32_t> tail;
        T elem[MPMCQ_SIZE];
        std::atomic<bool> avail[MPMCQ_SIZE] = {};
        std::atomic<Block *> next;
    };

    FreeList blks;

    std::atomic<Block *> head;
    cacheline_pad _pad0;
    std::atomic<Block *> tail;

    template<typename U>
    bool _enqueue(U &&e, bool unbounded = true) {
        for (;;)
        {
            auto t = tail.load(std::memory_order_acquire);
            auto tcnt = t->refcnt.load(std::memory_order_relaxed);
            if (!tcnt) continue;
            if (!t->refcnt.compare_exchange_weak(tcnt, tcnt + 1, std::memory_order_relaxed))
                continue;
            if (t->freed.load(std::memory_order_relaxed))
            {
                blks.release_ref(t);
                continue;
            }
            auto tt = t->tail.load(std::memory_order_relaxed);
            if (tt >= MPMCQ_SIZE)
            {
                if (t->next.load(std::memory_order_relaxed) == nullptr)
                {
                    FreeList::Node * _nblk;
                    if (!blks.pop(_nblk))
                    {
                        if (unbounded) _nblk = new Block();
                        else {
                            blks.release_ref(t);
                            return false;
                        }
                    }
                    auto nblk = static_cast<Block *>(_nblk);
                    nblk->head.store(0, std::memory_order_relaxed);
                    nblk->tail.store(0, std::memory_order_relaxed);
                    nblk->next.store(nullptr, std::memory_order_relaxed);
                    Block *tnext = nullptr;
                    if (!t->next.compare_exchange_weak(tnext, nblk, std::memory_order_acq_rel))
                        blks.push(nblk);
                    else
                    {
                        tail.store(nblk, std::memory_order_release);
                        nblk->freed.store(false, std::memory_order_release);
                    }
                }
                blks.release_ref(t);
                continue;
            }
            auto tt2 = tt;
            if (t->tail.compare_exchange_weak(tt2, tt2 + 1, std::memory_order_relaxed))
            {
                new (&(t->elem[tt])) T(std::forward<U>(e));
                t->avail[tt].store(true, std::memory_order_release);
                blks.release_ref(t);
                break;
            }
            blks.release_ref(t);
        }
        return true;
    }

    public:
    MPMCQueue(const MPMCQueue &) = delete;
    MPMCQueue(MPMCQueue &&) = delete;

    MPMCQueue(): head(new Block()), tail(head.load()) {
        auto h = head.load();
        h->head = h->tail = 0;
        h->next = nullptr;
    }

    ~MPMCQueue() {
        for (FreeList::Node *ptr; blks.pop(ptr); ) delete ptr;
        for (Block *ptr = head.load(), *nptr; ptr; ptr = nptr)
        {
            nptr = ptr->next;
            delete ptr;
        }
    }

    void set_capacity(size_t capacity = 0) {
        capacity = std::max(capacity / MPMCQ_SIZE, (size_t)1);
        while (capacity--) blks.push(new Block());
    }

    template<typename U>
    bool enqueue(U &&e, bool unbounded = true) {
        return _enqueue(e, unbounded);
    }

    template<typename U>
    bool try_enqueue(U &&e) {
        return _enqueue(e, false);
    }

    bool try_dequeue(T &e) {
        for (;;)
        {
            auto h = this->head.load(std::memory_order_acquire);
            auto hcnt = h->refcnt.load(std::memory_order_relaxed);
            if (!hcnt) continue;
            if (!h->refcnt.compare_exchange_weak(hcnt, hcnt + 1, std::memory_order_relaxed))
                continue;

            auto hh = h->head.load(std::memory_order_relaxed);
            auto tt = h->tail.load(std::memory_order_relaxed);
            if (hh >= tt)
            {
                if (tt < MPMCQ_SIZE) { blks.release_ref(h); return false; }
                auto hnext = h->next.load(std::memory_order_acquire);
                if (hnext == nullptr) { blks.release_ref(h); return false; }
                auto h2 = h;
                if (this->head.compare_exchange_weak(h2, hnext, std::memory_order_acq_rel))
                    this->blks.push(h);
                blks.release_ref(h);
                continue;
            }
            auto hh2 = hh;
            if (h->head.compare_exchange_weak(hh2, hh2 + 1, std::memory_order_relaxed))
            {
                while (!h->avail[hh].load(std::memory_order_acquire))
                    std::this_thread::yield();
                e = std::move(h->elem[hh]);
                h->avail[hh].store(false, std::memory_order_relaxed);
                blks.release_ref(h);
                break;
            }
            blks.release_ref(h);
        }
        return true;
    }
};

template<typename T>
struct MPSCQueue: public MPMCQueue<T> {
    using MPMCQueue<T>::MPMCQueue;
    /* the same thread is calling the following functions */

    bool try_dequeue(T &e) {
        for (;;)
        {
            auto h = this->head.load(std::memory_order_relaxed);
            auto hh = h->head.load(std::memory_order_relaxed);
            auto tt = h->tail.load(std::memory_order_relaxed);
            if (hh >= tt)
            {
                if (tt < MPMCQ_SIZE) return false;
                auto hnext = h->next.load(std::memory_order_relaxed);
                if (hnext == nullptr) return false;
                this->head.store(hnext, std::memory_order_relaxed);
                this->blks.push(h);
                continue;
            }
            h->head.store(hh + 1, std::memory_order_relaxed);
            while (!h->avail[hh].load(std::memory_order_acquire))
                std::this_thread::yield();
            e = std::move(h->elem[hh]);
            h->avail[hh].store(false, std::memory_order_relaxed);
            break;
        }
        return true;
    }

    template<typename U>
    bool rewind(U &&e) {
        auto h = this->head.load(std::memory_order_relaxed);
        auto hh = h->head.load(std::memory_order_relaxed);
        if (!hh)
        {
            FreeList::Node * _nblk;
            if (!this->blks.pop(_nblk)) _nblk = new typename MPMCQueue<T>::Block();
            auto nblk = static_cast<typename MPMCQueue<T>::Block *>(_nblk);
            nblk->head.store(MPMCQ_SIZE, std::memory_order_relaxed);
            nblk->tail.store(MPMCQ_SIZE, std::memory_order_relaxed);
            nblk->next.store(h, std::memory_order_relaxed);
            this->head.store(nblk, std::memory_order_relaxed);
        }
        h = this->head.load(std::memory_order_relaxed);
        hh = h->head.load(std::memory_order_relaxed) - 1;
        new (&(h->elem[hh])) T(std::forward<U>(e));
        h->avail[hh].store(true, std::memory_order_relaxed);
        h->head.store(hh, std::memory_order_relaxed);
        return true;
    }
};

}

#endif
