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
 * @file DFF2Source.hpp
 * @author Maurizio Drocco
 * @date Apr 12, 2017
 * @brief Short file description
 *
 * Here typically goes a more extensive explanation of what the file
 * contains.
 */

#ifndef GAM_INCLUDE_DFF2_SOURCE_HPP_
#define GAM_INCLUDE_DFF2_SOURCE_HPP_

#include <gam.hpp>

#include "Node.hpp"
#include "Logger.hpp"

namespace dff2
{

/**
 *
 * @brief Output-only DFF2 node.
 *
 * \see DFF2Node for the general execution model of DFF2 nodes.
 *
 * DFF2Filter represents a DFF2 node which, during its core processing phase,
 * performs some internal action and returns a brand new token.
 *
 * @todo feedback
 */
template<typename OutComm, typename out_t, typename Logic>
class Source: public Node
{
public:
    Source(OutComm &comm)
            : out_comm(comm)
    {
    }

    virtual ~Source()
    {
    }

protected:

#if 0
    /* @brief \see Filter::share()
     * @ingroup dff2-bb-api
     *
     * It does *not* activate downstream svc().
     */
    void share(const gam::public_ptr<out_t__> &to_emit)
    {
        Node::share_(out_comm, to_emit);
    }

    /* @brief \see DFF2Filter::broadcast()
     * @ingroup dff2-bb-api
     *
     * It does *not* activate downstream svc().
     */
    template<typename T>
    void broadcast(const gam::public_ptr<T> &to_emit)
    {
        DFF2Node::broadcast_(out_comm, to_emit);
    }
#endif

private:
    void set_links()
    {
        out_comm.internals.source(Node::id__);
    }

    void run()
    {
        DFF2_PROFILER_TIMER (t0);
        DFF2_PROFILER_TIMER (t1);
        DFF2_PROFILER_DURATION (d_svc);

        DFF2_LOGLN("SRC start");

        Node::init_(logic);
        token_t out;

        while (true)
        {
            /* generate a token by invoking user function */
            DFF2_PROFILER_HRT(t0);
            out = logic.svc(out_comm);
            DFF2_PROFILER_HRT(t1);
            DFF2_PROFILER_DURATION_ADD(d_svc, t0, t1);

            /* ckeck if meaningful output token */
            if (is_eos(out))
            { //user function asked for termination
                DFF2_LOGLN_OS("SRC svc returned eos");
                break;
            }

            else
            {
                //user function returned go_on: skip to next token
                DFF2_LOGLN_OS("SRC svc returned go_on");
                continue;
            }
        }

        Node::end_(logic);

        /* broadcast eos token */
        out_comm.internals.broadcast(global_eos<out_t>());

        /* write profiling */
        DFF2_PROFLN("SRC svc  = %f s", d_svc.count());

        DFF2_LOGLN("SRC done");
    }

    OutComm &out_comm;
    Logic logic;
};

} /* namespace gam */

#endif /* GAM_INCLUDE_DFF2_SOURCE_HPP_ */
