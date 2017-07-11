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

#ifndef UTILS_HPP_
#define UTILS_HPP_

/*
 * DEBUG facilities
 */
#ifdef GAM_DBG
#include <cassert>
#define DBGASSERT(x) assert(x)
#else
#define DBGASSERT(x) {}
#endif

/*
 * error reporting facilities
 */
#define USRASSERT(x) assert(x)

#endif /* UTILS_HPP_ */
