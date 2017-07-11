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

/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/**
 * A long description of the source file goes here.
 * 
 * @file        Node.hpp
 * @brief       a short description of the source file
 * @author      Maurizio Drocco
 * 
 */
#ifndef GAM_INCLUDE_DFF2_NODE_HPP_
#define GAM_INCLUDE_DFF2_NODE_HPP_

#include "defs.hpp"

namespace dff2 {

/*
 *
 */
class Node {
public:
    virtual ~Node()
    {

    }

    void id(gam::executor_id i)
    {
        id__ = i;
        set_links();
    }

    gam::executor_id id()
    {
        return id__;
    }

    virtual void set_links() = 0;

    virtual void run() = 0;

protected:
    template<typename Logic>
    void init_(Logic &l)
    {
        l.svc_init();
    }

    template<typename Logic>
    void end_(Logic &l)
    {
        l.svc_end();
    }

protected:
    gam::executor_id id__;
};

} /* namespace dff2 */

#endif /* GAM_INCLUDE_DFF2_NODE_HPP_ */
