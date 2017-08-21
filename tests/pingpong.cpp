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
 * 
 * @file        pingpong.cpp
 * @brief       simple example of 2-executor pingpong
 * @author      Maurizio Drocco
 * 
 */

#include <iostream>
#include <cassert>

#include <gam.hpp>
#include <TrackingAllocator.hpp>

/*
 *******************************************************************************
 *
 * rank-specific routines
 *
 *******************************************************************************
 */
void r0()
{
    /* create a private pointer */
    auto p = gam::make_private<int>(42);

    /* push to 1 */
    p.push(1);

    /* wait for the response */
    p = gam::pull_private<int>(1);

    assert(*p.local() == 42);
}

void r1()
{
    /* pull private pointer from 0 */
    auto p = gam::pull_private<int>(); //from-any just for testing

    /* push back to 0 */
    p.push(0);
}

/*
 *******************************************************************************
 *
 * main
 *
 *******************************************************************************
 */
int main(int argc, char * argv[])
{
    /* rank-specific code */
    switch (gam::rank())
    {
    case 0:
        r0();
        break;
    case 1:
        r1();
        break;
    }

    return 0;
}
