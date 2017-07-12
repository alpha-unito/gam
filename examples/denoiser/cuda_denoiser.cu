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
 * @file        cuda_denoiser.cpp
 * @brief       gam-based salt-and-pepper restoration filter, CUDA kernel
 * @author      Maurizio Drocco
 *
 */

#include <dff2/dff2.hpp>

#include "defs.hpp"
#define CUDA_RESTORE
#include "farm_components.hpp"

/*
 *******************************************************************************
 *
 * main
 *
 *******************************************************************************
 */
int main(int argc, char * argv[])
{
    dff2::add(VideoSource(e2w));
    for (unsigned i = 0; i < NWORKERS; ++i)
        dff2::add(VideoFilter(e2w, w2c));
    dff2::add(VideoWriter(w2c));
    
    frame_pool_init();

    dff2::run();

    return 0;
}
