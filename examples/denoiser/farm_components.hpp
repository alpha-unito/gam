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
 * @file        common.hpp
 * @author      Maurizio Drocco
 * 
 */
#ifndef EXAMPLES_DENOISER_FARM_COMPONENTS_HPP_
#define EXAMPLES_DENOISER_FARM_COMPONENTS_HPP_

#include <dff2/dff2.hpp>

#include "defs.hpp"
#include "video_source.hpp"
#include "video_filter.hpp"
#include "video_writer.hpp"

/*
 ***************************************************************************
 *
 * farm components
 *
 ***************************************************************************
 */
typedef dff2::Source<dff2::RoundRobinSwitch, //
        gam::private_ptr<serial_frame_t>, //
        VideoSourceLogic<dff2::RoundRobinSwitch>> VideoSource;

typedef dff2::Sink<dff2::RoundRobinMerge, //
        gam::private_ptr<serial_frame_t>, //
        VideoWriterLogic> VideoWriter;

typedef dff2::Filter<dff2::RoundRobinSwitch, dff2::RoundRobinMerge, //
        gam::private_ptr<serial_frame_t>, gam::private_ptr<serial_frame_t>, //
        VideoFilterLogic<dff2::RoundRobinMerge>> VideoFilter;

#endif /* EXAMPLES_DENOISER_FARM_COMPONENTS_HPP_ */
