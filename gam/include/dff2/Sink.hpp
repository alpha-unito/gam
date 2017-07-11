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
 * @file DFF2Sink.hpp
 * @author Maurizio Drocco
 * @date Apr 12, 2017
 * @brief Short file description
 *
 * Here typically goes a more extensive explanation of what the file
 * contains.
 */

#ifndef GAM_INCLUDE_DFF2_SINK_HPP_
#define GAM_INCLUDE_DFF2_SINK_HPP_

#include <gam.hpp>

#include "Node.hpp"
#include "Logger.hpp"

namespace dff2 {

/**
 *
 * @brief Input-only DFF2 node.
 *
 * \see Node for the general execution model of DFF2 nodes.
 *
 * Sink represents a DFF2 node which, during its core processing phase,
 * receives an input token, processes it and performs some internal action
 * without returning anything.
 *
 * @todo feedback
 */
template<typename InComm, typename in_t, typename Logic>
class Sink: public Node {
public:
    Sink(InComm &comm)
            : in_comm(comm)
    {
    }

    virtual ~Sink()
    {
    }

protected:
#if 0
    /* @brief \see Filter::get()
     * @ingroup dff2-bb-api
     */
    template<typename collected_t>
    gam::public_ptr<collected_t> get()
    {
        return Node::get_<InComm, collected_t>(in_comm);
    }
#endif

private:
    void set_links()
    {
        in_comm.internals.destination(Node::id__);
    }

    void run()
    {
        DFF2_PROFILER_TIMER (t0);
        DFF2_PROFILER_TIMER (t1);
        DFF2_PROFILER_DURATION (d_get);
        DFF2_PROFILER_DURATION (d_get_max);
        DFF2_PROFILER_COND (unsigned long long d_get_max_index = 0);
        DFF2_PROFILER_DURATION (d_svc);

        DFF2_LOGLN("SNK start");

        Node::init_(logic);

        in_t in;

        while (true)
        {
            /* prepare local input pointer */
            DFF2_PROFILER_HRT(t0);
            in = in_comm.internals.template get<in_t>();
            DFF2_PROFILER_HRT(t1);
            DFF2_PROFILER_DURATION_ADD(d_get, t0, t1);
            DFF2_PROFILER_BOOL(flg, time_diff(t0, t1) > d_get_max);
            DFF2_PROFILER_BOOL_COND (flg, d_get_max = time_diff(t0,t1););
            DFF2_PROFILER_BOOL_COND (flg, d_get_max_index = token_id;);

            if (!is_eos(in))
            { //meaningful input token
                DFF2_LOGLN_OS("SNK got=" << in);

                /* process token by invoking user function */
                DFF2_PROFILER_HRT(t0);
                logic.svc(in);
                DFF2_PROFILER_HRT(t1);
                DFF2_PROFILER_DURATION_ADD(d_svc, t0, t1);
            }

            else
            {
                //got eos from upstream communicator
                DFF2_LOGLN_OS("SNK got eos");

                if (++received_eos == in_comm.internals.in_cardinality())
                    break;
            }

            ++token_id;
        }

        Node::end_(logic);

        /* write profiling */
        DFF2_PROFLN("SNK svc      = %f s", d_svc.count());
        DFF2_PROFLN("SNK get      = %f s", d_get.count());
        DFF2_PROFLN("SNK get MAX  = %f s", d_get_max.count());
        DFF2_PROFLN("SNK get MAXi = %d", d_get_max_index);

        /* write single-line profiling */
        DFF2_PROFLN_RAW("svc\tget\tgetM\tgetMi");
        DFF2_PROFLN_RAW("%f\t%f\t%f\t%d", //
                d_svc.count(), d_get.count(), //
                d_get_max.count(), d_get_max_index);

        DFF2_LOGLN("SNK done");
    }

    InComm &in_comm;
    gam::executor_id received_eos = 0;
    Logic logic;
    unsigned long long token_id = 0;
};

}
/* namespace gam */

#endif /* GAM_INCLUDE_DFF2SINK_HPP_ */
