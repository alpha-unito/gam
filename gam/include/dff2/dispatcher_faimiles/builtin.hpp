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
 * @file        builtin.hpp
 * @brief       a short description of the source file
 * @author      Maurizio Drocco
 * 
 */
#ifndef GAM_INCLUDE_DFF2_DISPATCHER_FAIMILES_BUILTIN_HPP_
#define GAM_INCLUDE_DFF2_DISPATCHER_FAIMILES_BUILTIN_HPP_

namespace dff2 {

#include <vector>

#include <gam.hpp>

/**
 * Nondeterminate Merge (singleton) family.
 */
class NDMerge {
public:
    template<typename T>
    void get(const std::vector<gam::executor_id> &s, //
            gam::public_ptr<T> &p)
    {
        p = gam::pull_public<T>();

    }

    template<typename T>
    void get(const std::vector<gam::executor_id> &s, //
            gam::private_ptr<T> &p)
    {
        p = gam::pull_private<T>();
    }
};

/**
 * Merge family.
 */
template<typename Policy>
class Merge {
public:
    template<typename T>
    using ptr_t = gam::private_ptr<T>;

    template<typename T>
    void get(const std::vector<gam::executor_id> &s, //
            gam::public_ptr<T> &p)
    {
        p = gam::pull_public<T>(coll(s));
    }

    template<typename T>
    void get(const std::vector<gam::executor_id> &s, //
            gam::private_ptr<T> &p)
    {
        p = gam::pull_private<T>(coll(s));
    }

private:
    Policy coll;
};

/**
 * Switch family.
 */
template<typename Policy>
class Switch {
public:
    template<typename T, typename ... PolicyArgs>
    void put(const std::vector<gam::executor_id> &d, //
            const gam::public_ptr<T> &p, //
            PolicyArgs&&... __a)
    {
        p.push(dist(d, std::forward<PolicyArgs>(__a)...));
    }

    template<typename T, typename ... PolicyArgs>
    void put(const std::vector<gam::executor_id> &d, //
            gam::private_ptr<T> &&p, //
            PolicyArgs&&... __a)
    {
        p.push(dist(d, std::forward<PolicyArgs>(__a)...));
    }

    template<typename T>
    void broadcast(const std::vector<gam::executor_id> &d, //
            const gam::public_ptr<T> &p)
    {
        for (auto to : d)
            p.push(to);
    }

    template<typename T>
    void broadcast(const std::vector<gam::executor_id> &d, //
            gam::private_ptr<T> &&p)
    {
        USRASSERT(!p.get().is_address());
        for (auto to : d)
            gam::private_ptr<T>(p.get()).push(to);
    }

    Policy dist;
};

} /* namespace dff2 */

#endif /* GAM_INCLUDE_DFF2_DISPATCHER_FAIMILES_BUILTIN_HPP_ */
