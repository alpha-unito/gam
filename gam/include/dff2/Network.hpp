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
 * @file        Network.hpp
 * @brief       implements Network class.
 * @author      Maurizio Drocco
 * 
 */
#ifndef GAM_INCLUDE_DFF2_NETWORK_HPP_
#define GAM_INCLUDE_DFF2_NETWORK_HPP_

#include <cassert>

#include <gam.hpp>

#include "Node.hpp"
#include "Logger.hpp"
#include "Profiler.hpp"

namespace dff2 {

/*
 * Singleton class representing a whole DFF2 application.
 */
class Network {
public:
    static Network *getNetwork()
    {
        static Network network;
        return &network;
    }

    ~Network()
    {
        std::vector<Node *>::size_type i;
        for (i = 0; i < gam::rank() && i < cardinality(); ++i)
            delete nodes[i];
        if (i < cardinality())
            assert(nodes[i] == nullptr);
        for (++i; i < cardinality(); ++i)
            delete nodes[i];
        nodes.clear();
    }

    gam::executor_id cardinality()
    {
        return nodes.size();
    }

    template<typename T>
    void add(const T &n)
    {
        auto np = dynamic_cast<Node *>(new T(n));
        np->id((gam::executor_id) nodes.size());
        nodes.push_back(np);
    }

    template<typename T>
    void add(T &&n)
    {
        auto np = dynamic_cast<Node *>(new T(std::move(n)));
        np->id((gam::executor_id) nodes.size());
        nodes.push_back(np);
    }

    void run()
    {
        DFF2_PROFILER_TIMER(t0);
        DFF2_PROFILER_TIMER(t_init);
        DFF2_PROFILER_TIMER(t_run);

        DFF2_PROFILER_HRT(t0);

        /* initialize gam runtime */
        gam::init();

        /* initialize the logger */
        char *env = std::getenv("GAM_LOG_PREFIX");
        assert(env);
        DFF2_LOGGER_INIT(env, gam::rank());

        /* initialize the profiler */
        DFF2_PROFILER_INIT(env, gam::rank());

        DFF2_PROFILER_HRT(t_init);

        /* check cardinality */
        assert(gam::cardinality() >= cardinality()); //todo error reporting

        if (gam::rank() < cardinality())
        {
            /* run the node associated to the executor */
            nodes[gam::rank()]->run();

            DFF2_PROFILER_HRT(t_run);

            /* call node destructor to trigger destruction of data members */
            delete nodes[gam::rank()];
            nodes[gam::rank()] = nullptr;

        }

        /* write profiling */
        DFF2_PROFLN("NET init  = %f s", DFF2_PROFILER_SPAN(t0, t_init));
        DFF2_PROFLN("NET run   = %f s", DFF2_PROFILER_SPAN(t_init, t_run));

        /* write single-line profiling */
        DFF2_PROFLN_RAW("init\tsvc");
        DFF2_PROFLN_RAW("%f\t%f", //
                DFF2_PROFILER_SPAN(t0, t_init),//
                DFF2_PROFILER_SPAN(t_init, t_run));

        /* finalize the profiler */
        DFF2_PROFILER_FINALIZE(gam::rank());

        /* finalize the logger */
        DFF2_LOGGER_FINALIZE(gam::rank());

        /* finalize gam runtime */
        gam::finalize();
    }

private:
    std::vector<Node *> nodes;
};

template<typename T>
static void add(const T &n)
{
    Network::getNetwork()->add(n);
}

template<typename T>
static void add(T &&n)
{
    Network::getNetwork()->add(std::move(n));
}

static void run()
{
    Network::getNetwork()->run();
}

} /* namespace dff2 */

#endif /* GAM_INCLUDE_DFF2_NETWORK_HPP_ */
