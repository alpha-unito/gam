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
 * @file        non_trivially_copyable_public.cpp
 * @brief       2-executor pingpong with non-trivially-copyable types
 * @author      Maurizio Drocco / Paolo Viviani
 * 
 */

#include <iostream>
#include <cassert>

#include <gam.hpp>
#include <TrackingAllocator.hpp>

/*
 *******************************************************************************
 *
 * type definition
 *
 *******************************************************************************
 */
template<typename T>
struct gam_vector : public std::vector<T> {
	using vsize_t = typename std::vector<T>::size_type;
	vsize_t size_ = 0;

	gam_vector() = default;

	/* ingesting constructor */
	template<typename StreamInF>
	gam_vector(StreamInF &&f) {
		typename std::vector<T>::size_type in_size;
		f(&in_size, sizeof(vsize_t));
		this->resize(in_size);
		assert(this->size() == in_size);
		f(this->data(), in_size * sizeof(T));
	}

	/* marshalling function */
	gam::marshalled_t marshall() {
		gam::marshalled_t res;
		size_ = this->size();
		res.emplace_back(&size_, sizeof(vsize_t));
		res.emplace_back(this->data(), size_ * sizeof(T));
		return res;
	}
};

/*
 *******************************************************************************
 *
 * rank-specific routines
 *
 *******************************************************************************
 */
void r0()
{
    /* create a public pointer */
    auto p = gam::make_public<gam_vector<int>>();

    /* populate */
    auto lp = p.local();
    lp->push_back(42);

    /* push to 1 */
    p.push(1);

    /* wait for the response */
    p = gam::pull_public<gam_vector<int>>(1);

    /* test */
    lp = p.local();
    assert(lp->size() == 2);
    assert(lp->at(1) == 43);
}

void r1()
{
    /* pull public pointer from 0 */
    auto p = gam::pull_public<gam_vector<int>>(); //from-any just for testing

    /* test and add */
    auto lp = p.local();
    assert(lp->size() == 1);
    assert(lp->at(0) == 42);
    lp->push_back(43);

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
