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

/**
 * @file Cache.hpp
 * @author Maurizio Drocco
 * @date Apr 10, 2017
 * @brief implements Cache class.
 *
 * @todo bounded size
 */

#ifndef GAM_INCLUDE_CACHE_HPP_
#define GAM_INCLUDE_CACHE_HPP_

#include <unordered_map>

#include "utils.hpp"
#include "TrackingAllocator.hpp"

namespace gam {

class Cache
{
public:
    void finalize() {
        for(auto it : cache_map)
            FREE(const_cast<void *>(it.second));
        cache_map.clear();
    }

    template<typename T>
    void store(uint64_t a, T *p)
    {
        if (!available())
            make_room();
        DBGASSERT(cache_map.find(a) == cache_map.end());
        void *slot = MALLOC(sizeof(T));
        memcpy(slot, p, sizeof(T));
        cache_map[a] = slot;
        LOGLN_OS("CTX cache store a=" << a << " p=" << slot);
    }

    template<typename T>
    bool load(T *dst, uint64_t a)
    {
        if (cache_map.find(a) != cache_map.end())
        {
            LOGLN_OS("CTX cache hit a=" << a);
            memcpy((void *) dst, cache_map[a], sizeof(T));
            return true;
        }
        LOGLN_OS("CTX cache miss a=" << a);
        return false;
    }

private:
    std::unordered_map<uint64_t, const void *> cache_map;

    //todo fixed capacity or whatever
    bool available()
    {
        return true;
    }

    //todo eviction policy
    void make_room()
    {
    }
};

} /* namespace gam */

#endif /* GAM_INCLUDE_CACHE_HPP_ */
