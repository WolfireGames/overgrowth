#ifndef SHARED_PTR_H
#define SHARED_PTR_H

#include <stdexcept>

/**
 * EXAMPLE IMPLEMENTATION! DO NOT EVEN THINK TO USE!
 */

template <class T>
class shared_ptr
{
    class holder
    {
    
    public:
        
        holder() 
            : p(0),
              counter(1)
        {
        }
        
        holder(T* t) 
            : p(t),
              counter(1)
        {
        }
        
        ~holder()
        {
            delete p;
        }

        T* p;
        int counter;

    private:
    
        holder& operator=(const holder&);
        holder(const holder&);
    };

    holder* h_;

public:

    ~shared_ptr()
    {
        if (--h_->counter == 0)
        {
            delete h_;
        }
    }

    shared_ptr() 
        : h_(new holder())
    {
    }

    shared_ptr(T* t) 
        : h_(new holder(t))
    {
    }

    shared_ptr(const shared_ptr& rhl) 
        : h_(rhl.h_)
    {
        ++h_->counter;
    }

    shared_ptr& operator=(const shared_ptr& rhl)
    {
        if (this == &rhl)
        {
            return *this;
        }

        if (--h_->counter == 0)
        {
            delete h_;
        }
        h_ = rhl.h_;
        ++h_->counter;
        return *this;
    }

    shared_ptr& operator=(T* t)
    {
        if (--h_->counter == 0)
        {
            delete h_;
        }
        h_ = new holder(t);
        return *this;
    }

    T* get() const
    {
        return h_->p;
    }

    T* operator ->() const
    {
        if (h_->p == 0)
        {
            throw std::runtime_error("null pointer dereferenced");
        }
        return h_->p;
    }

    void reset(T* t = 0)
    {
        if (--h_->counter == 0)
        {
            delete h_;
        }
        h_ = new holder(t);
    }

    int count() const
    {
        return h_->counter;
    }
};

#endif
