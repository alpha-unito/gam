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
 * @defgroup api GAM API
 */

/**
 * This file implements the GAM API.
 * 
 * @file        gam.hpp
 * @brief       the GAM API
 * @author      Maurizio Drocco
 * 
 * @ingroup api
 *
 * @todo non-blocking pull
 * @todo pull from any
 * @todo inlining
 *
 */
#ifndef GAM_HPP_
#define GAM_HPP_

#include "defs.hpp"
#include "utils.hpp"
#include "Context.hpp" //ctx
#include "GlobalPointer.hpp"
#include "public_ptr.hpp"
#include "private_ptr.hpp"

namespace gam {
/**
 * @brief initializes GAM executor
 */
void init()
{
    ctx.init();
}

/**
 * @brief finalizes GAM executor
 */
void finalize()
{
    ctx.finalize();
}

/**
 * @brief returns GAM rank
 *
 * The rank of a GAM executor is its index in the space of executors.
 * Namely, it ranges from 1 to the number of executors.
 *
 */
executor_id rank()
{
    return ctx.rank();
}

executor_id cardinality()
{
    return ctx.cardinality();
}

} /* namespace gam */

#endif /* GAM_HPP_ */
