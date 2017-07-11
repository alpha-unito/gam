/* ***************************************************************************
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License version 3 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************
 */

/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef GAM_INCLUDE_CONCURRENTMAPWRAP_HPP_
#define GAM_INCLUDE_CONCURRENTMAPWRAP_HPP_

#include <mutex>

namespace gam {

template<typename internal_map_t>
class ConcurrentMapWrap {
public:
    typedef typename internal_map_t::key_type key_type;
    typedef typename internal_map_t::mapped_type mapped_type;
    typedef typename internal_map_t::const_iterator const_iterator;
    typedef typename internal_map_t::size_type size_type;

    bool empty() {
        mtx.lock();
        bool res = map.empty();
        mtx.unlock();

        return res;
    }

    const_iterator begin()
    {
        mtx.lock();
        const_iterator res = map.begin();
        mtx.unlock();

        return res;
    }

    const_iterator end()
    {
        return map.end();
    }

    const_iterator find(const key_type &k)
    {
        mtx.lock();
        const_iterator res = map.find(k);
        mtx.unlock();

        return res;
    }

    mapped_type &operator[](const key_type &k)
    {
        mtx.lock();
        mapped_type &res(map[k]);
        mtx.unlock();

        return res;
    }

    size_type erase(const key_type &k)
    {
        mtx.lock();
        size_type res = map.erase(k);
        mtx.unlock();

        return res;
    }

private:
    internal_map_t map;
    std::mutex mtx;
};

} /* namespace gam */

#endif /* GAM_INCLUDE_CONCURRENTMAPWRAP_HPP_ */
