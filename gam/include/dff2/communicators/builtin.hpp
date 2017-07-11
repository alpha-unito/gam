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
 * @file        OneToTone.hpp
 * @brief       a short description of the source file
 * @author      Maurizio Drocco
 * 
 */
#ifndef GAM_INCLUDE_DFF2_COMMUNICATORS_BUILTIN_HPP_
#define GAM_INCLUDE_DFF2_COMMUNICATORS_BUILTIN_HPP_

#include <gam.hpp>

#include "../dispatcher_faimiles/builtin.hpp"
#include "../dispatching_policies/builtin.hpp"
#include "../CommunicatorInternals.hpp"

namespace dff2
{
template<typename T, typename Internals, typename ... PolicyArgs>
static inline void emit_(const gam::public_ptr<T> &p, Internals &internals, //
        PolicyArgs&&... __a)
{
    internals.put(p, std::forward<PolicyArgs>(__a)...);
}

template<typename T, typename Internals, typename ... PolicyArgs>
static inline void emit_(gam::private_ptr<T> &&p, Internals &internals, //
        PolicyArgs&&... __a)
{
    internals.put(std::move(p), std::forward<PolicyArgs>(__a)...);
}

class OneToOne
{
public:
    template<typename T>
    void emit(const gam::public_ptr<T> &p)
    {
        emit_(p, internals);
    }

    template<typename T>
    void emit(gam::private_ptr<T> &&p)
    {
        emit_(std::move(p), internals);
    }

    CommunicatorInternals<Switch<ConstantTo>, Merge<ConstantFrom>> internals;
};

class RoundRobinSwitch
{
public:
    template<typename T>
    void emit(const gam::public_ptr<T> &p)
    {
        emit_(p, internals);
    }

    template<typename T>
    void emit(gam::private_ptr<T> &&p)
    {
        emit_(std::move(p), internals);
    }

    CommunicatorInternals<Switch<RRTo>, Merge<ConstantFrom>> internals;
};

class Shuffle
{
public:
    template<typename T>
    void emit(const gam::public_ptr<T> &p)
    {
        emit_(p, internals);
    }

    template<typename T>
    void emit(gam::private_ptr<T> &&p)
    {
        emit_(std::move(p), internals);
    }

    CommunicatorInternals<Switch<KeyedTo>, Merge<ConstantFrom>> internals;
};

class RoundRobinMerge
{
public:
    template<typename T>
    void emit(const gam::public_ptr<T> &p)
    {
        emit_(p, internals);
    }

    template<typename T>
    void emit(gam::private_ptr<T> &&p)
    {
        emit_(std::move(p), internals);
    }

    CommunicatorInternals<Switch<ConstantTo>, Merge<RRFrom>> internals;
};

class NondeterminateMerge
{
public:
    template<typename T>
    void emit(const gam::public_ptr<T> &p)
    {
        emit_(p, internals);
    }

    template<typename T>
    void emit(gam::private_ptr<T> &&p)
    {
        emit_(std::move(p), internals);
    }

    CommunicatorInternals<Switch<ConstantTo>, NDMerge> internals;
};

} /* namespace dff2 */

#endif /* GAM_INCLUDE_DFF2_COMMUNICATORS_BUILTIN_HPP_ */
