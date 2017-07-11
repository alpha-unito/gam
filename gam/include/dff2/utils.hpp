/*
 * utils.hpp
 *
 *  Created on: Jun 13, 2017
 *      Author: drocco
 */

#ifndef GAM_INCLUDE_DFF2_UTILS_HPP_
#define GAM_INCLUDE_DFF2_UTILS_HPP_

/*
 * High resolution timers on top of C++ chrono
 */

#include <chrono>

namespace dff2
{
typedef std::chrono::high_resolution_clock::time_point time_point_t;
typedef std::chrono::duration<double> duration_t;

time_point_t hires_timer_ull()
{
    return std::chrono::high_resolution_clock::now();
}

duration_t time_diff(time_point_t a, time_point_t b)
{
    return std::chrono::duration_cast<duration_t>(b - a);
}

} /* namespace dff2*/

#endif /* GAM_INCLUDE_DFF2_UTILS_HPP_ */
