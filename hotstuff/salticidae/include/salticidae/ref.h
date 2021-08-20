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

#ifndef _SALTICIDAE_REF_H
#define _SALTICIDAE_REF_H

#include <atomic>
#include <functional>

namespace salticidae {

template<typename T>
struct default_delete {
    constexpr default_delete() = default;
    void operator()(T *ptr) const {
        static_assert(!std::is_void<T>::value,
                    "can't delete pointer to incomplete type");
        static_assert(sizeof(T) > 0,
                    "can't delete pointer to incomplete type");
        delete ptr;
    }
};

template<typename T>
struct default_delete<T[]> {
    constexpr default_delete() = default;
    void operator()(T *ptr) const {
        static_assert(sizeof(T) > 0,
                    "can't delete pointer to incomplete type");
        delete [] ptr;
    }
};

template<typename T, typename R, typename D> class _RcObjBase;
template<typename T, typename D> class _BoxObj;
template<typename T, typename D> class BoxObj;

template<typename T, typename D>
class _BoxObj {
    protected:
    T *obj;

    template<typename T_, typename R, typename D_>
    friend class _RcObjBase;
    template<typename T_, typename D_>
    friend class _BoxObj;

    public:
    using type = T;
    constexpr _BoxObj(): obj(nullptr) {}
    constexpr _BoxObj(T *obj): obj(obj) {}
    _BoxObj(const _BoxObj &other) = delete;
    _BoxObj(_BoxObj &&other): obj(other.obj) {
        other.obj = nullptr;
    }

    template<typename T_, typename D_>
    _BoxObj(_BoxObj<T_, D_> &&other): obj(other.obj) {
        other.obj = nullptr;
    }

    ~_BoxObj() {
        if (obj) D()(obj);
    }

    void swap(_BoxObj &other) { std::swap(obj, other.obj); }

    _BoxObj &operator=(const _BoxObj &other) = delete;
    _BoxObj &operator=(_BoxObj &&other) {
        if (this != &other)
        {
            _BoxObj tmp(std::move(other));
            tmp.swap(*this);
        }
        return *this;
    }

    T &operator *() const { return *obj; }
    T *get() const { return obj; }
    T *unwrap() {
        auto ret = obj;
        obj = nullptr;
        return ret;
    }
    operator bool() const { return obj != nullptr; }
    bool operator==(const _BoxObj &other) const { return obj == other.obj; }
    bool operator!=(const _BoxObj &other) const { return obj != other.obj; }
    bool operator==(std::nullptr_t) const { return obj == nullptr; }
    bool operator!=(std::nullptr_t) const { return obj != nullptr; }
    bool operator<(const _BoxObj &other) const { return obj < other.obj; }
    bool operator>(const _BoxObj &other) const { return obj > other.obj; }
    bool operator<=(const _BoxObj &other) const { return obj <= other.obj; }
    bool operator>=(const _BoxObj &other) const { return obj >= other.obj; }
};

template<typename T, typename D = default_delete<T>>
class BoxObj: public _BoxObj<T, D> {
    using base_t = _BoxObj<T, D>;
    friend std::hash<BoxObj>;
    template<typename T__, typename D__, typename T_, typename D_>
    friend BoxObj<T__, D__> static_pointer_cast(BoxObj<T_, D_> &&other);

    public:
    BoxObj() = default;
    BoxObj(T *obj): base_t(obj) {}
    template<typename T_, typename D_,
            typename = typename std::enable_if<!std::is_array<T_>::value>::type>
    BoxObj(BoxObj<T_, D_> &&other): base_t(std::move(other)) {}

    T *operator->() const { return base_t::obj; }
};

template<typename T, typename D>
class BoxObj<T[], D>: public _BoxObj<T, D> {
    using base_t = _BoxObj<T, D>;
    friend std::hash<BoxObj>;
    template<typename T__, typename D__, typename T_, typename D_>
    friend BoxObj<T__, D__> static_pointer_cast(BoxObj<T_, D_> &&other);

    public:
    BoxObj() = default;
    BoxObj(T obj[]): base_t(obj) {}
    template<typename T_, typename D_>
    BoxObj(BoxObj<T_[], D_> &&other): base_t(std::move(other)) {}

    T &operator[](size_t idx) const { return base_t::obj[idx]; }
};

template<typename T, typename D = default_delete<T>, typename T_, typename D_>
BoxObj<T, D> static_pointer_cast(BoxObj<T_, D_> &&other) {
    BoxObj<T, D> box{};
    box.obj = static_cast<typename BoxObj<T, D>::type *>(other.obj);
    return box;
}

struct _RCCtl {
    size_t ref_cnt;
    size_t weak_cnt;
    void add_ref() { ref_cnt++; }
    void add_weak() { weak_cnt++; }
    bool release_ref() {
        if (--ref_cnt) return false;
        if (weak_cnt) return true;
        delete this;
        return true;
    }
    void release_weak() {
        if (--weak_cnt) return;
        if (ref_cnt) return;
        delete this;
    }
    size_t get_cnt() { return ref_cnt; }
    _RCCtl(): ref_cnt(1), weak_cnt(0) {}
    ~_RCCtl() {}
};

struct _ARCCtl {
    std::atomic_size_t ref_cnt;
    std::atomic_size_t weak_cnt;
    std::atomic<std::uint8_t> dcnt;
    void add_ref() { ref_cnt++; }
    void add_weak() { weak_cnt++; }
    bool release_ref() {
        dcnt++;
        if (--ref_cnt) { dcnt--; return false; }
        if (weak_cnt) { dcnt--; return true; }
        if (!--dcnt) delete this;
        return true;
    }
    void release_weak() {
        dcnt++;
        if (--weak_cnt) { dcnt--; return; }
        if (ref_cnt) { dcnt--; return; }
        if (!--dcnt) delete this;
    }
    size_t get_cnt() { return ref_cnt.load(); }
    _ARCCtl(): ref_cnt(1), weak_cnt(0), dcnt(0) {}
    ~_ARCCtl() {}
};

template<typename T, typename R, typename D> class RcObjBase;

template<typename T, typename R>
class _WeakObjBase {
    T *obj;
    R *ctl;

    template<typename T_, typename R_, typename D>
    friend class _RcObjBase;

    public:
    _WeakObjBase(): ctl(nullptr) {}

    _WeakObjBase(const _WeakObjBase &other):
            obj(other.obj), ctl(other.ctl) {
        if (ctl) ctl->add_weak();
    }

    _WeakObjBase(_WeakObjBase &&other):
            obj(other.obj), ctl(other.ctl) {
        other.ctl = nullptr;
    }

    void swap(_WeakObjBase &other) {
        std::swap(obj, other.obj);
        std::swap(ctl, other.ctl);
    }

    _WeakObjBase &operator=(const _WeakObjBase &other) {
        if (this != &other)
        {
            _WeakObjBase tmp(other);
            tmp.swap(*this);
        }
        return *this;
    }

    _WeakObjBase &operator=(_WeakObjBase &&other) {
        if (this != &other)
        {
            _WeakObjBase tmp(std::move(other));
            tmp.swap(*this);
        }
        return *this;
    }

    template<typename D>
    _WeakObjBase(const _RcObjBase<T, R, D> &other);

    ~_WeakObjBase() { if (ctl) ctl->release_weak(); }
};


template<typename T, typename R>
class WeakObjBase: public _WeakObjBase<T, R> {
    using base_t = _WeakObjBase<T, R>;
    friend std::hash<WeakObjBase>;

    public:
    WeakObjBase() = default;
    WeakObjBase(const WeakObjBase &other) = default;
    WeakObjBase(WeakObjBase &&other) = default;
    template<typename D>
    WeakObjBase(const RcObjBase<T, R, D> &other): base_t(other) {}
    WeakObjBase &operator=(const WeakObjBase &) = default;
    WeakObjBase &operator=(WeakObjBase &&) = default;
};

template<typename T, typename R>
class WeakObjBase<T[], R>: public _WeakObjBase<T, R> {
    using base_t = _WeakObjBase<T, R>;
    friend std::hash<WeakObjBase>;

    public:
    WeakObjBase() = default;
    WeakObjBase(const WeakObjBase &other) = default;
    WeakObjBase(WeakObjBase &&other) = default;
    template<typename D>
    WeakObjBase(const RcObjBase<T[], R, D> &other): base_t(other) {}
    WeakObjBase &operator=(const WeakObjBase &) = default;
    WeakObjBase &operator=(WeakObjBase &&) = default;
};


template<typename T, typename R, typename D>
class _RcObjBase {
    protected:
    T *obj;
    R *ctl;

    friend _WeakObjBase<T, R>;
    template<typename T_, typename R_, typename D_>
    friend class _RcObjBase;

    public:
    using type = T;
    _RcObjBase(): obj(nullptr), ctl(nullptr) {}
    _RcObjBase(T *obj): obj(obj), ctl(new R()) {}
    _RcObjBase(BoxObj<T> &&box_ref): obj(box_ref.obj), ctl(new R()) {
        box_ref.obj = nullptr;
    }

    _RcObjBase(const _RcObjBase &other):
            obj(other.obj), ctl(other.ctl) {
        if (ctl) ctl->add_ref();
    }

    _RcObjBase(_RcObjBase &&other):
            obj(other.obj), ctl(other.ctl) {
        other.ctl = nullptr;
    }

    template<typename T_, typename D_>
    _RcObjBase(const _RcObjBase<T_, R, D_> &other):
            obj(other.obj), ctl(other.ctl) {
        if (ctl) ctl->add_ref();
    }

    template<typename T_, typename D_>
    _RcObjBase(_RcObjBase<T_, R, D_> &&other):
            obj(other.obj), ctl(other.ctl) {
        other.ctl = nullptr;
    }

    void swap(_RcObjBase &other) {
        std::swap(obj, other.obj);
        std::swap(ctl, other.ctl);
    }

    _RcObjBase &operator=(const _RcObjBase &other) {
        if (this != &other)
        {
            _RcObjBase tmp(other);
            tmp.swap(*this);
        }
        return *this;
    }

    _RcObjBase &operator=(_RcObjBase &&other) {
        if (this != &other)
        {
            _RcObjBase tmp(std::move(other));
            tmp.swap(*this);
        }
        return *this;
    }

    _RcObjBase(const _WeakObjBase<T, R> &other) {
        if (other.ctl && other.ctl->ref_cnt)
        {
            obj = other.obj;
            ctl = other.ctl;
            ctl->add_ref();
        }
        else
        {
            obj = nullptr;
            ctl = nullptr;
        }
    }

    ~_RcObjBase() {
        if (ctl && ctl->release_ref())
            D()(obj);
    }

    size_t get_cnt() const { return ctl ? ctl->get_cnt() : 0; }
    T &operator *() const { return *obj; }
    T *get() const { return obj; }
    operator bool() const { return obj != nullptr; }
    bool operator==(const _RcObjBase &other) const { return obj == other.obj; }
    bool operator!=(const _RcObjBase &other) const { return obj != other.obj; }
    bool operator==(std::nullptr_t) const { return obj == nullptr; }
    bool operator!=(std::nullptr_t) const { return obj != nullptr; }
    bool operator<(const _RcObjBase &other) const { return obj < other.obj; }
    bool operator>(const _RcObjBase &other) const { return obj > other.obj; }
    bool operator<=(const _RcObjBase &other) const { return obj <= other.obj; }
    bool operator>=(const _RcObjBase &other) const { return obj >= other.obj; }
};

template<typename T, typename R, typename D = default_delete<T>>
class RcObjBase: public _RcObjBase<T, R, D> {
    using base_t = _RcObjBase<T, R, D>;
    friend std::hash<RcObjBase>;
    template<typename T__, typename D__, typename T_, typename R_, typename D_>
    friend RcObjBase<T__, R_, D__> static_pointer_cast(const RcObjBase<T_, R_, D_> &other);
    template<typename T__, typename D__, typename T_, typename R_, typename D_>
    friend RcObjBase<T__, R_, D__> static_pointer_cast(RcObjBase<T_, R_, D_> &&other);

    public:
    T *operator->() const { return base_t::obj; }
    RcObjBase() = default;
    RcObjBase(T *obj): base_t(obj) {}
    RcObjBase(BoxObj<T> &&box_ref): base_t(std::move(box_ref)) {}

    template<typename T_, typename D_,
            typename = typename std::enable_if<!std::is_array<T_>::value>::type>
    RcObjBase(const RcObjBase<T_, R, D_> &other): base_t(other) {}

    template<typename T_, typename D_,
            typename = typename std::enable_if<!std::is_array<T_>::value>::type>
    RcObjBase(RcObjBase<T_, R, D_> &&other): base_t(std::move(other)) {}

    RcObjBase(const WeakObjBase<T, R> &other): base_t(other) {}

    RcObjBase(const RcObjBase &other) = default;
    RcObjBase(RcObjBase &&other) = default;
    RcObjBase &operator=(const RcObjBase &) = default;
    RcObjBase &operator=(RcObjBase &&) = default;
};

template<typename T, typename R, typename D>
class RcObjBase<T[], R, D>: public _RcObjBase<T, R, D> {
    using base_t = _RcObjBase<T, R, D>;
    friend std::hash<RcObjBase>;
    template<typename T__, typename D__, typename T_, typename R_, typename D_>
    friend RcObjBase<T__, R_, D__> static_pointer_cast(const RcObjBase<T_, R_, D_> &other);
    template<typename T__, typename D__, typename T_, typename R_, typename D_>
    friend RcObjBase<T__, R_, D__> static_pointer_cast(RcObjBase<T_, R_, D_> &&other);

    public:
    T &operator[](size_t idx) { return base_t::obj; }
    RcObjBase() = default;
    RcObjBase(T obj[]): base_t(obj) {}
    RcObjBase(BoxObj<T[]> &&box_ref): base_t(std::move(box_ref)) {}

    template<typename T_, typename D_>
    RcObjBase(const RcObjBase<T_[], R, D_> &other): base_t(other) {}

    template<typename T_, typename D_>
    RcObjBase(RcObjBase<T_[], R, D_> &&other): base_t(std::move(other)) {}

    RcObjBase(const WeakObjBase<T[], R> &other): base_t(other) {}

    RcObjBase(const RcObjBase &other) = default;
    RcObjBase(RcObjBase &&other) = default;
    RcObjBase &operator=(const RcObjBase &) = default;
    RcObjBase &operator=(RcObjBase &&) = default;
};


template<typename T, typename D = default_delete<T>,
        typename T_, typename R_, typename D_>
RcObjBase<T, R_, D> static_pointer_cast(const RcObjBase<T_, R_, D_> &other) {
    RcObjBase<T, R_, D> rc{};
    rc.obj = static_cast<typename RcObjBase<T, R_, D>::type *>(other.obj);
    if ((rc.ctl = other.ctl))
        rc.ctl->add_ref();
    return rc;
}

template<typename T, typename D = default_delete<T>,
        typename T_, typename R_, typename D_>
RcObjBase<T, R_, D> static_pointer_cast(RcObjBase<T_, R_, D_> &&other) {
    RcObjBase<T, R_, D> rc{};
    rc.obj = static_cast<typename RcObjBase<T, R_, D>::type *>(other.obj);
    rc.ctl = other.ctl;
    other.ctl = nullptr;
    return rc;
}

template<typename T, typename R>
template<typename D>
inline _WeakObjBase<T, R>::_WeakObjBase(const _RcObjBase<T, R, D> &other):
        obj(other.obj), ctl(other.ctl) {
    if (ctl) ctl->add_weak();
}

template<typename T, typename D = default_delete<T>> using RcObj = RcObjBase<T, _RCCtl, D>;
template<typename T> using WeakObj = WeakObjBase<T, _RCCtl>;

template<typename T, typename D = default_delete<T>> using ArcObj = RcObjBase<T, _ARCCtl, D>;
template<typename T> using AweakObj = WeakObjBase<T, _ARCCtl>;

}

namespace std {
    template<typename T, typename R, typename D>
    struct hash<salticidae::RcObjBase<T, R, D>> {
        size_t operator()(const salticidae::RcObjBase<T, R, D> &k) const {
            return (size_t)k.obj;
        }
    };

    template<typename T, typename D>
    struct hash<salticidae::BoxObj<T, D>> {
        size_t operator()(const salticidae::BoxObj<T, D> &k) const {
            return (size_t)k.obj;
        }
    };
}

#endif
