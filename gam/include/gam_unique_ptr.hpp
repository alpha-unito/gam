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
 * @file gam_unique_ptr.hpp
 * @author Maurizio Drocco
 * @date Apr 14, 2017
 * @brief implements gam_unique_ptr class and some associated functions
 *
 * Here typically goes a more extensive explanation of what the file
 * contains.
 */

#ifndef GAM_INCLUDE_GAM_UNIQUE_PTR_HPP_
#define GAM_INCLUDE_GAM_UNIQUE_PTR_HPP_

#include <memory>

#include "TrackingAllocator.hpp"

namespace gam {

/**
 * @brief represents a unique (local) pointer with custom deleter.
 *
 * A gam_unique_ptr value represents either:
 * - a plain unique pointer with custom deleter
 * - the (local) child of a (global) private pointer
 * In the former case, it is constructed by means of either the new operator or
 * the make_gam_unique function, analogous to std::make_unique.
 * In the latter case, it is constructed by the local() function.
 */
template<typename T>
using gam_unique_ptr = std::unique_ptr<T, void(*)(T *)>;

template<typename _Tp, typename... _Args>
gam_unique_ptr<_Tp> make_gam_unique(_Args&&... __args) {
    return unique_ptr<_Tp, void(*)(_Tp *)>(//
            NEW<_Tp>(std::forward<_Args>(__args)...), //
            DELETE<_Tp>
            );
}

} /* namespace gam */

#endif /* GAM_INCLUDE_GAM_UNIQUE_PTR_HPP_ */
