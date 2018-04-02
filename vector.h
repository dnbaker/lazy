#ifndef _LZY_VEC_H__
#define _LZY_VEC_H__
#include <type_traits>
#include <stdexcept>
#include <cstring>
#include <cstdlib>
#include <memory>
#include "kspp/ks.h"

#ifndef LAZY_PUSH_BACK_RESIZING_FACTOR
#define LAZY_PUSH_BACK_RESIZING_FACTOR 1.25
#endif

namespace lazy {

enum initialization: bool {
    LAZY_VEC_NOINIT = false,
    LAZY_VEC_INIT = true
};

template<typename T, typename size_type=uint32_t>
class vector {
    size_type n_, m_;
    T *data_;
    static constexpr double PUSH_BACK_RESIZING_FACTOR = LAZY_PUSH_BACK_RESIZING_FACTOR;

public:
    using value_type = T;
    template<typename... FactoryArgs>
    vector(size_type n=0, bool init=!std::is_pod_v<T>, FactoryArgs &&...args): n_{n}, m_{n}, data_{n ? static_cast<T *>(std::malloc(sizeof(T) * n)): nullptr} {
        if (init)
            for(size_type i(0); i < n_; ++i)
                new(data_ + i) T(std::forward<FactoryArgs>(args)...);
    }
    vector(const vector &other): n_(other.n_), m_(other.n_), data_{static_cast<T *>(std::malloc(sizeof(T) * m_))} {
        std::copy(std::cbegin(other), std::cend(other), begin());
    }
    vector(vector &&other): n_(other.n_), m_(other.m_), data_(other.data_) {
        other.m_ = other.n_ = 0;
        other.data_ = nullptr;
    }
    vector(std::initializer_list<T> il): n_(il.size()), m_(il.size()), data_{n_ ? static_cast<T *>(std::malloc(sizeof(T) * n_)): nullptr} {
        std::move(il.begin(), il.end(), std::begin(*this));
    }
    const T *cbegin() const {return data_;}
    const T *cend()   const {return data_ + n_;}
    const T *begin()  const {return data_;}
    const T *end()          const {return data_ + n_;}
    T *begin()        {return data_;}
    T *end()          {return data_ + n_;}
    T *data()         {return data_;}
    const T *data() const {return data_;}
    auto &back() {return data_[n_ - 1];}
    const auto &back() const {return data_[n_ - 1];}
    auto &front() {return data_[0];}
    const auto &front() const {return data_[0];}
    auto size() const {return n_;}
    auto capacity() const {return m_;}
    vector &operator=(const vector &o) {
        if(m_ < o.m_) data_ = (T *)std::realloc(data_, sizeof(T) * o.m_);
        n_ = o.n_;
        m_ = o.m_;
        std::copy(begin(), end(), (T *)o.begin());
        return *this;
    }
    vector &operator=(vector &&o) {
        n_ = o.n_;
        m_ = o.m_;
        data_ = o.data_;
        std::memset(&o, 0, sizeof(o));
        return *this;
    }
    template<typename osize_type>
    bool operator==(const ::lazy::vector<T, osize_type> &other) const {
        if(size() != other.size()) return false;
        for(typename std::common_type_t<osize_type, size_type>i(0);i < n_; ++i)
            if(data_[i] != other[i])
                return false;
        return true;
    }
    // Note: push_back and bnslace_back are the same *except* that push_back changes size multiplicatively.
    template<typename... Args>
    auto &emplace_back(Args&& ... args) {
        if(n_ + 1 > m_) {
            data_ = static_cast<T *>(std::realloc(data_, sizeof(T) * (n_ + 1)));
#if !NDEBUG
            if(m_ + 1 < m_) throw std::runtime_error(
                ks::sprintf("Type of size %zu is not big enough. (%zu, %zu)\n",
                            sizeof(m_), m_, m_ + 1).data());
#else
            ++m_;
#endif
        }
        new(data_ + n_++) T(std::forward<Args>(args)...);
        return back();
    }
    template<typename... Args>
    auto &push_back(Args&& ... args) {
        if(n_ + 1 > m_)
            reserve(std::max(static_cast<size_type>(m_ * PUSH_BACK_RESIZING_FACTOR),
                             m_ + 1));
        new(data_ + n_++) T(std::forward<Args>(args)...);
        return back();
    }
    void zero() {std::memset(data_, 0, sizeof(T) * n_);}
    void reserve(size_t newsize) {
        if(newsize > m_) {
            auto tmp(static_cast<T*>(std::realloc(data_, sizeof(T) * newsize)));
            if(tmp == nullptr) throw std::bad_alloc();
            data_ = tmp;
        }
    }
    template<typename... Args>
    void resize(size_t newsize, bool init=true, Args &&...args) {
        reserve(newsize);
        if(init) {
            for(;n_ < m_;) new(data_ + n_++) T(std::forward<Args>(args)...);
        }
    }
    void shrink_to_fit() {
        if(m_ > n_) {
            auto tmp(static_cast<T*>(std::realloc(data_, sizeof(T) * n_)));
            if(tmp == nullptr) throw std::bad_alloc();
            data_ = tmp;
            m_ = n_;
        }
    }
    T &operator[](size_type idx) {return data_[idx];}
    const T &operator[](size_type idx) const {return data_[idx];}
    ~vector(){
        if constexpr(!std::is_trivially_destructible_v<T>) {
            for(auto &el: *this) el.~T();
        }
        std::free(data_);
    }
};

}

#endif // #ifndef _LZY_VEC_H__
