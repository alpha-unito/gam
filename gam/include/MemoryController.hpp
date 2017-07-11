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

/**
 * A long description of the source file goes here.
 * 
 * @file        SmartMemoryController.hpp
 * @brief       a short description of the source file
 * @author      Maurizio Drocco
 * 
 */
#ifndef GAM_INCLUDE_MEMORYCONTROLLER_HPP_
#define GAM_INCLUDE_MEMORYCONTROLLER_HPP_

#include <mutex>
#include <cstdint>
#include <queue>
#include <thread>
#include <atomic>

#include "Logger.hpp"

namespace gam {

/*
 *
 */
class MemoryController
{
public:
    inline void rc_init(uint64_t a)
    {
        LOGLN("SMC init %llu", a);

        //mtx.lock();
        DBGASSERT(ref_cnt.find(a) == ref_cnt.end());
        //mtx.unlock();

        //mtx.lock();
        ref_cnt[a].store(1);
        //mtx.unlock();
    }

    inline unsigned long long rc_inc(uint64_t a)
    {
        //mtx.lock();
        unsigned long long res = ++ref_cnt[a];
        //mtx.unlock();

        LOGLN("SMC +1 %llu = %llu", a, res);
        return res;
    }

    inline unsigned long long rc_dec(uint64_t a)
    {
        //mtx.lock();
        unsigned long long res = --ref_cnt[a];
        //mtx.unlock();

        LOGLN("SMC -1 %llu = %llu", a, res);
        return res;
    }

private:
    /*
     * Reference count, for each address the process is author of.
     */
    std::unordered_map<uint64_t, std::atomic<unsigned long long>> ref_cnt;
    //std::mutex mtx;
};

} /* namespace gam */

#endif /* GAM_INCLUDE_MEMORYCONTROLLER_HPP_ */
