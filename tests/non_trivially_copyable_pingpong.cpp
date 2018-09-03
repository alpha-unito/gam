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

template <typename T>
struct gam_indirect_vector {
  using vsize_t = typename std::vector<T>::size_type;
  vsize_t size_ = 0;

  explicit gam_indirect_vector() : vptr(nullptr) {}

  explicit gam_indirect_vector(vsize_t size, const T &v)
      : vptr(new std::vector<T>(size, v)) {}

  gam_indirect_vector &operator=(const gam_indirect_vector &copy) {
    vptr = new std::vector<T>(*copy.vptr);
    return *this;
  }

  ~gam_indirect_vector() {
    assert(vptr);
    delete vptr;
    vptr = nullptr;
  }

  std::vector<T> *get() { return vptr; }

 private:
  std::vector<T> *vptr = nullptr;
};

/*
 *******************************************************************************
 *
 * rank-specific routines
 *
 *******************************************************************************
 */
void r0() {
                auto p = gam::make_private<gam_indirect_vector<int>>(10, 42);
                p.push(1);
}

void r1() {
                // check local consistency on public pointer
                auto p = gam::pull_private<gam_indirect_vector<int>>(0);
                auto lp = p.local();
                // here end-of-scope triggers the destructor on the original object
                std::vector<int> ref(10, 42);
                assert(*lp->get() == ref);
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
