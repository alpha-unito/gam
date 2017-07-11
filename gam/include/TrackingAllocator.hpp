/* ***************************************************************************
 *
 *  This file is part of gam.
 *
 *  gam is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  gam is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with gam. If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************************************
 */

/**
 * @file        TrackingAllocator.hpp
 * @brief       implements TrackingAllocator class
 * @author      Maurizio Drocco
 * 
 * @ingroup internals
 *
 */
#ifndef GAM_INCLUDE_TRACKINGALLOCATOR_HPP_
#define GAM_INCLUDE_TRACKINGALLOCATOR_HPP_

#include <unordered_map>
#include <mutex>

#include "utils.hpp"
#include "ConcurrentMapWrap.hpp"

namespace gam {

/**
 * A naive concurrent allocator with leak tracking, in form of singleton class.
 */
class TrackingAllocator
{
    enum alloc_op
    {
        MALLOC_ = 0, NEW_ = 1
    };
    typedef ConcurrentMapWrap<std::unordered_map<void *, alloc_op>> alloc_map_t;

public:
    ~TrackingAllocator()
    {
        if (!inflight.empty())
        {
            for (auto it : inflight)
                fprintf(stderr, "ALC %p %d\n", it.first, it.second);
            DBGASSERT(false);
        }
    }

    static TrackingAllocator *getAllocator()
    {
        static TrackingAllocator a;
        return &a;
    }

    void *malloc(size_t size)
    {
        void *res = ::malloc(size);
        DBGASSERT(res);

        mtx.lock();
        DBGASSERT(inflight.find(res) == inflight.end());
        mtx.unlock();

        mtx.lock();
        inflight[res] = MALLOC_;
        mtx.unlock();

        return res;
    }

    void free(void *p)
    {
        DBGASSERT(p != nullptr);

        mtx.lock();
        DBGASSERT(inflight.find(p) != inflight.end());
        mtx.unlock();

        mtx.lock();
        DBGASSERT(inflight[p] == MALLOC_);
        mtx.unlock();

        mtx.lock();
        alloc_map_t::size_type res = inflight.erase(p);
        mtx.unlock();

        DBGASSERT(res > 0);

        ::free(p);
    }

    void new_(void *p)
    {
        DBGASSERT(p != nullptr);

        mtx.lock();
        DBGASSERT(inflight.find(p) != inflight.end());
        mtx.unlock();

        mtx.lock();
        DBGASSERT(inflight[p] == MALLOC_);
        mtx.unlock();

        mtx.lock();
        inflight[p] = NEW_;
        mtx.unlock();
    }

    void delete_(void *p)
    {
        DBGASSERT(p != nullptr);

        mtx.lock();
        DBGASSERT(inflight.find(p) != inflight.end());
        mtx.unlock();

        mtx.lock();
        DBGASSERT(inflight[p] == NEW_);
        mtx.unlock();

        mtx.lock();
        inflight[p] = MALLOC_;
        mtx.unlock();
    }

private:
    std::mutex mtx;
    alloc_map_t inflight;

    TrackingAllocator()
    {
    }
};

/*
 * shortcuts
 */
inline void *MALLOC(size_t size)
{
#ifdef GAM_DBG

    return gam::TrackingAllocator::getAllocator()->malloc(size);
#else
    return ::malloc(size);
#endif
}

inline void FREE(void *ptr)
{
#ifdef GAM_DBG
    gam::TrackingAllocator::getAllocator()->free(ptr);
#else
    ::free(ptr);
#endif
}

template<typename T>
inline void TYPED_FREE(T *ptr)
{
    FREE((void *) ptr);
}

template<typename obj_t, typename ... Params>
inline obj_t *NEW(Params ... p)
{
#ifdef GAM_DBG
    obj_t *ptr = (obj_t *) MALLOC(sizeof(obj_t));
    gam::TrackingAllocator::getAllocator()->new_(ptr);
    return new (ptr) obj_t(p...);
#else
    return new obj_t(p...);
#endif
}

template<typename T>
inline void DELETE(T *ptr)
{
#ifdef GAM_DBG
    ptr->~T();
    gam::TrackingAllocator::getAllocator()->delete_(ptr);
    FREE(ptr);
#else
    delete (ptr);
#endif
}

}
/* namespace gam */

#endif /* GAM_INCLUDE_TRACKINGALLOCATOR_HPP_ */
