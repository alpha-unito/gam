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
 * @file fl_connectionless.hpp
 * @author Maurizio Drocco
 * @brief implements fl_connectionless class
 *
 * @ingroup internals
 *
 * fl_connectionless implements gam communication primitives
 * on top of connection-less libfabric API.
 */

#ifndef GAM_INCLUDE_LINKS_IMPLEMENTATIONS_FL_CONNECTIONLESS_HPP_
#define GAM_INCLUDE_LINKS_IMPLEMENTATIONS_FL_CONNECTIONLESS_HPP_

#include <vector>
#include <algorithm>
#include <cassert>

#include <rdma/fabric.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_errno.h>

#include "defs.hpp"
#include "GlobalPointer.hpp"
#include "Logger.hpp"

#include "fl_common.hpp"

namespace gam {

static struct fi_av_attr av_attr;
static struct fid_av *av; //AV table

static void init_links(char *src_node)
{
    int ret = 0;

    //query fabric contexts
#ifdef GAM_LOG
    fprintf(stderr, "LKS init_links\n");
#endif
    uint64_t flags = 0;
    fi_info *fi;
    fl_common::fi_getinfo_(&fi, src_node, NULL, flags, FI_EP_RDM);

    fl_common::init(fi);

    //init AV
    ret += fi_av_open(fl_common::domain(), &av_attr, &av, NULL);

    assert(!ret);

    fi_freeinfo(fi);
}

static void fini_links()
{
    int ret = 0;

    ret += fi_close(&av->fid);
    DBGASSERT(!ret);

    fl_common::fini();
}

class fl_connectionless {
public:
    fl_connectionless(executor_id cardinality, executor_id self)
            : rank_to_addr(cardinality), self(self)
    {

    }

    /*
     * add send link
     */
    void add(executor_id i, char *node, char *svc)
    {
        LOGLN("LKS @%p adding SEND to=%llu node=%s svc=%s", this, i, node, svc);

        //insert address to AV
        fi_addr_t fi_addr;
        fi_av_insertsvc(av, node, svc, &fi_addr, 0, NULL);

        //map rank to av index
        rank_to_addr[i] = fi_addr;

        char addr[128], buf[128];
        unsigned long int len = 128;
        fi_av_lookup(av, fi_addr, (void *) addr, &len);
        assert(len < 128);
        len = 128;
        fi_av_straddr(av, addr, buf, &len);
        assert(len < 128);
        LOGLN("LKS @%p mapping rank=%llu -> addr=%llu (%s)", this, i, fi_addr,
                buf);
    }

    /*
     * add receive link
     */
    void add(char *node, char *svc)
    {
        LOGLN("LKS @%p adding RECV node=%s svc=%s", this, node, svc);
        init_endpoint(node, svc);
    }

    /*
     ***************************************************************************
     *
     * blocking send/receive
     *
     ***************************************************************************
     */
    void broadcast(const void *p, size_t size)
    {
        ssize_t ret = 0;
        rank_t to;
        for (to = 0; to < self; ++to)
            ret += fl.fl_tx(p, size, rank_to_addr[to]);
        for (to = self + 1; to < rank_to_addr.size(); ++to)
            ret += fl.fl_tx(p, size, rank_to_addr[to]);
        DBGASSERT(!ret);
    }

    void raw_send(const void *p, const size_t size, const executor_id to)
    {
        ssize_t ret = fl.fl_tx(p, size, rank_to_addr[to]);
        DBGASSERT(!ret);
    }

    void raw_recv(void *p, const size_t size, const executor_id from)
    {
        ssize_t ret = fl.fl_rx(p, size, rank_to_addr[from]);
        DBGASSERT(!ret);
    }

    void raw_recv(void *p, const size_t size)
    {
        ssize_t ret = fl.fl_rx(p, size, FI_ADDR_UNSPEC);
        DBGASSERT(!ret);
    }

    /*
     ***************************************************************************
     *
     * non-blocking send/receive
     *
     ***************************************************************************
     */
    void nb_send(const void *p, const size_t size, const executor_id to)
    {
        ssize_t ret = fl.fl_post_tx(p, size, rank_to_addr[to]);
        DBGASSERT(!ret);
    }

    void nb_recv(void *p, const size_t size)
    {
        ssize_t ret = fl.fl_post_rx(p, size, FI_ADDR_UNSPEC);
        DBGASSERT(!ret);
    }

    bool nb_poll()
    {
        return fl.nb_poll();
    }

private:
    fl_common fl;

    typedef std::vector<fi_addr_t>::size_type rank_t;
    std::vector<fi_addr_t> rank_to_addr;
    rank_t self;

    void init_endpoint(char *node, char *service)
    {
        int ret;

        //get fabric context
#ifdef GAM_LOG
        fprintf(stderr, "LKS src-endpoint node=%s svc=%s\n", node, service);
#endif
        fi_info *fi;
        fl_common::fi_getinfo_(&fi, node, service, FI_SOURCE, FI_EP_RDM);

        fl.create_endpoint(fi);

        //bind EP to AV
        ret = fi_ep_bind(fl.ep(), &av->fid, 0);
        DBGASSERT(!ret);

        fl.enable_endpoint();

        //clean-up
        fi_freeinfo(fi);
    }
};

} /* namespace gam */

#endif /* GAM_INCLUDE_LINKS_IMPLEMENTATIONS_FL_CONNECTIONLESS_HPP_ */
