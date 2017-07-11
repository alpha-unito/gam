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
 * @file defs.hpp
 * @author Maurizio Drocco
 * @date Apr 14, 2017
 */

#ifndef GAM_INCLUDE_DFF2_DEFS_HPP_
#define GAM_INCLUDE_DFF2_DEFS_HPP_

#include <gam.hpp>

namespace dff2 {
/*
 ***************************************************************************
 *
 * synchronization token
 *
 ***************************************************************************
 */
typedef uint64_t token_t;
static constexpr token_t eos = gam::GlobalPointer::last_reserved;
static constexpr uint64_t go_on = eos - 1;

/*
 ***************************************************************************
 *
 * end-of-stream token
 *
 ***************************************************************************
 */
template<typename T>
bool is_eos(const gam::public_ptr<T> &token)
{
    return token.get().address() == eos;
}

template<typename T>
bool is_eos(const gam::private_ptr<T> &token)
{
    return token.get().address() == eos;
}

bool is_eos(const token_t &token)
{
    return token == eos;
}

/*
 * builds a global pointer representing an eos token.
 */
template<typename T>
T global_eos()
{
    return T(gam::GlobalPointer(eos));
}

/*
 ***************************************************************************
 *
 * go-on token
 *
 ***************************************************************************
 */
bool is_go_on(const token_t &token)
{
    return token == go_on;
}

} /* namespace dff2 */

#endif /* GAM_INCLUDE_DFF2_DEFS_HPP_ */
