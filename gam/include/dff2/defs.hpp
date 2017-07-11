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
 * @file defs.hpp
 * @author Maurizio Drocco
 * @date Apr 14, 2017
 * @brief Short file description
 *
 * Here typically goes a more extensive explanation of what the file
 * contains.
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
