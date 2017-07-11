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
#ifndef GAM_INCLUDE_DFF2_DISPATCHING_POLICIES_BUILTIN_HPP_
#define GAM_INCLUDE_DFF2_DISPATCHING_POLICIES_BUILTIN_HPP_

namespace dff2 {

#include <gam.hpp>

/*
 *******************************************************************************
 *
 * Pushing Policies
 *
 *******************************************************************************
 */

/*
 *
 */
class ConstantTo {
public:
    gam::executor_id operator()(const std::vector<gam::executor_id> &dest)
    {
        return dest[0];
    }
};

/*
 *
 */
class RRTo {
public:
    gam::executor_id operator()(const std::vector<gam::executor_id> &dest)
    {
        gam::executor_id res = dest[rr_cnt];
        rr_cnt = (rr_cnt + 1) % dest.size();
        return res;
    }

private:
    gam::executor_id rr_cnt = 0;
};

/*
 *
 */
class KeyedTo {
public:
    template<typename K>
    gam::executor_id operator()(const std::vector<gam::executor_id> &dest, //
            const K &key)
    {
        return key % dest.size();
    }
};

/*
 *******************************************************************************
 *
 * Pulling Policies
 *
 *******************************************************************************
 */
/**
 * Constant collection policy
 */
class ConstantFrom {
public:
    gam::executor_id operator()(const std::vector<gam::executor_id> &src)
    {
        return src[0];
    }
};

/**
 * Round-Robin collection policy
 */
class RRFrom {
public:
    gam::executor_id operator()(const std::vector<gam::executor_id> &src)
    {
        gam::executor_id res = src[rr_cnt];
        rr_cnt = (rr_cnt + 1) % src.size();
        return res;
    }

private:
    gam::executor_id rr_cnt = 0;
};

} //namespace dff2

#endif /* GAM_INCLUDE_DFF2_DISPATCHING_POLICIES_BUILTIN_HPP_ */
