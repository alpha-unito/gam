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
 * @file        backendptr.hpp
 * @brief       a short description of the source file
 * @author      Maurizio Drocco
 * 
 */
#ifndef GAM_INCLUDE_BACKEND_PTR_HPP_
#define GAM_INCLUDE_BACKEND_PTR_HPP_

#include "TrackingAllocator.hpp"

namespace gam {

/*
 *
 */
class backend_ptr {
public:
    virtual ~backend_ptr()
    {

    }

    virtual void *get() const = 0;
};

template<typename T, typename Deleter>
class backend_typed_ptr: public backend_ptr {
public:
    backend_typed_ptr(T *ptr, Deleter d)
            : ptr(ptr), d(d)
    {
    }

    ~backend_typed_ptr()
    {
        d(ptr);
    }

    void *get() const
    {
        return (void *) ptr;
    }

    T *typed_get() const
    {
        return ptr;
    }

private:
    T * ptr;
    Deleter d;
};

} /* namespace gam */

#endif /* GAM_INCLUDE_BACKEND_PTR_HPP_ */
