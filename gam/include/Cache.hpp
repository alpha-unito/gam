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
#include "wrapped_allocator.hpp"

namespace gam
{

class Cache
{
public:

    Cache(wrapped_allocator &wa_)
            : wa(wa_)
    {
    }

    void finalize()
    {
        for (auto it : cache_map)
            wa.free(const_cast<void *>(it.second));
        cache_map.clear();
    }

    template<typename T>
    void store(uint64_t a, T *p)
    {
        if (!available())
            make_room();
        DBGASSERT(cache_map.find(a) == cache_map.end());
        void *slot = wa.malloc(sizeof(T));
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
    wrapped_allocator &wa;

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
