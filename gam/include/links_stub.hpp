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
 * @file links_stub.hpp
 * @author Maurizio Drocco
 * @brief implements links_stub class
 *
 * @ingroup internals
 *
 */

#ifndef GAM_INCLUDE_LINKS_STUB_HPP_
#define GAM_INCLUDE_LINKS_STUB_HPP_

#include <vector>
#include <algorithm>

#include <rdma/fabric.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_errno.h>

#include "defs.hpp"
#include "GlobalPointer.hpp"
#include "Logger.hpp"

namespace gam {

template<typename impl, typename T>
class links_stub {
public:
    links_stub(executor_id cardinality, executor_id self, const char *svc)
            : internals(cardinality, self, svc), self_(self)
    {
    }

    ~links_stub()
    {
    }

    static void init_links(char *src_node)
    {
        impl::init_links(src_node);
    }

    static void fini_links()
    {
        impl::fini_links();
    }

    /*
     * add send link
     */
    void peer(executor_id i, char *node, char *svc)
    {
        LOGLN("LKS @%p adding PEER rank=%llu node=%s svc=%s", this, i, node, svc);
        internals.add(i, node, svc);
    }

    /*
     * add receive link
     */
    void init(char *node, char *svc)
    {
        internals.add(node, svc);
    }

    void finalize()
    {
        internals.finalize();
    }

    /*
     ***************************************************************************
     *
     * blocking send/receive
     *
     ***************************************************************************
     */
    void raw_send(const void *p, const size_t size, const executor_id to)
    {
        internals.raw_send(p, size, to);
    }

    void raw_recv(void *p, const size_t size, const executor_id from)
    {
        internals.raw_recv(p, size, from);
    }

    void raw_recv(void *p, const size_t size)
    {
        internals.raw_recv(p, size);
    }

    void send(const T &p, const executor_id to)
    {
        raw_send(&p, sizeof(T), to);
    }

    void recv(T &p, const executor_id from)
    {
        raw_recv(&p, sizeof(T), from);
    }

    void recv(T &p)
    {
        raw_recv(&p, sizeof(T));
    }

    void broadcast(const T &p)
    {
        internals.broadcast(&p, sizeof(T));
    }

    /*
     ***************************************************************************
     *
     * non-blocking send/receive
     *
     ***************************************************************************
     */
    void nb_send(const T &p, const executor_id to)
    {
        internals.nb_send(&p, sizeof(T), to);
    }

    void nb_recv(T &p)
    {
        internals.nb_recv(&p, sizeof(T));
    }

    bool nb_poll()
    {
        return internals.nb_poll();
    }

private:
    impl internals;
    executor_id self_;
};

} /* namespace gam */

#endif /* GAM_INCLUDE_LINKS_STUB_HPP_ */
