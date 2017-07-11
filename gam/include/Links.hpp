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
 * @file Links.hpp
 * @author Maurizio Drocco
 * @date Mar 19, 2017
 * @brief implements Links class
 *
 * @ingroup internals
 *
 * Links is libfabric implementation of a set of input/output ports from/to executors.
 */

#ifndef GAM_INCLUDE_LINKS_HPP_
#define GAM_INCLUDE_LINKS_HPP_

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

static struct fid_fabric *fabric;
static struct fid_domain *domain;
static struct fi_av_attr av_attr;
static struct fid_av *av; //AV table

static void fi_getinfo_(fi_info **fi, char *node, char *service, uint64_t flags)
{
    fi_info *hints = fi_allocinfo();
    int ret = 0;

    //prepare for querying fabric contexts
    hints->ep_attr->type = FI_EP_RDM;
    //todo more hints

    //query fabric contexts
    ret = fi_getinfo(FI_VERSION(1, 3), node, service, flags, hints, fi);
    assert(!ret);

    fi_freeinfo(hints);

#if 0
    fi_info *fip = *fi;
    for (int fii = 0; fip; fip = fip->next, fii++)
    fprintf(stderr, "***\nprovider #%d:\n%s", fii,
            fi_tostr(fip, FI_TYPE_INFO));
#endif
}

static void init_links()
{
    int ret = 0;

    //query fabric contexts
    char *node = NULL, *service = NULL;
    uint64_t flags = 0;
    fi_info *fi = fi_allocinfo();
    fi_getinfo_(&fi, node, service, flags);

    //init fabric context
    ret += fi_fabric(fi->fabric_attr, &fabric, NULL);

    //init domain
    ret += fi_domain(fabric, fi, &domain, NULL);

    //init AV
    ret += fi_av_open(domain, &av_attr, &av, NULL);

    assert(!ret);

    fi_freeinfo(fi);
}

template<typename T>
class Links
{
public:
    Links(executor_id cardinality, executor_id self) :
            rank_to_addr(cardinality), self(self)
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
        create_endpoint(node, svc);
    }

    /*
     ***************************************************************************
     *
     * blocking send/receive
     *
     ***************************************************************************
     */
    void send(const T &p, const executor_id to)
    {
        ssize_t ret = fm_tx(&p, rank_to_addr[to]);
        DBGASSERT(!ret);
    }

    void recv(T &p, const executor_id from)
    {
        ssize_t ret = fm_rx(&p, rank_to_addr[from]);
        DBGASSERT(!ret);
    }

    void recv(T &p)
    {
        ssize_t ret = fm_rx(&p);
        DBGASSERT(!ret);
    }

    void broadcast(const T &p)
    {
        ssize_t ret = 0;
        rank_t to;
        for (to = 0; to < self; ++to)
            ret += fm_tx(&p, rank_to_addr[to]);
        for (to = self + 1; to < rank_to_addr.size(); ++to)
            ret += fm_tx(&p, rank_to_addr[to]);
        DBGASSERT(!ret);
    }

    void raw_send(const void *p, const size_t size, const executor_id to)
    {
        ssize_t ret = fm_tx(p, size, rank_to_addr[to]);
        DBGASSERT(!ret);
    }

    void raw_recv(void *p, const size_t size, const executor_id from)
    {
        ssize_t ret = fm_rx(p, size, rank_to_addr[from]);
        DBGASSERT(!ret);
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
        ssize_t ret = fm_post_tx(&p, rank_to_addr[to]);
        DBGASSERT(!ret);
    }

    void nb_recv(T &p)
    {
        ssize_t ret = fm_post_rx(&p);
        DBGASSERT(!ret);
    }

    bool nb_poll()
    {
        struct fi_cq_entry cq_entry;
        ssize_t ret = fi_cq_read(rxcq, &cq_entry, 1); //non-blocking pop
        if (ret > 0)
            return true;
        else
        {
            DBGASSERT(ret == -FI_EAGAIN);
            return false;
        }
    }

private:
    typedef std::vector<fi_addr_t>::size_type rank_t;
    std::vector<fi_addr_t> rank_to_addr;
    rank_t self;
    struct fid_ep *ep = nullptr; //end point
    struct fid_cq *txcq = nullptr, *rxcq = nullptr; //completion queues
    uint64_t tx_cq_cntr = 0, rx_cq_cntr = 0; //n. of completed send/recv requests

    void create_endpoint(char *node, char *service)
    {
        struct fi_cq_attr cq_attr;
        int ret = 0;

        //get fabric context
        uint64_t flags = FI_SOURCE;
        fi_info *fi = fi_allocinfo();
        fi_getinfo_(&fi, node, service, flags);

        //init TX CQ (completion queue)
        memset(&cq_attr, 0, sizeof(fi_cq_attr));
        cq_attr.format = FI_CQ_FORMAT_CONTEXT;
        cq_attr.wait_obj = FI_WAIT_NONE; //async
        cq_attr.size = fi->tx_attr->size;
        ret += fi_cq_open(domain, &cq_attr, &txcq, &txcq);

        //init RX CQ
        cq_attr.size = fi->rx_attr->size;
        ret += fi_cq_open(domain, &cq_attr, &rxcq, &rxcq);

        //create endpoint
        ret += fi_endpoint(domain, fi, &ep, NULL);

        //bind EP to AV
        ret += fi_ep_bind(ep, &av->fid, 0);

        //bind EP to TX CQ
        ret += fi_ep_bind(ep, &txcq->fid, FI_SEND);

        //bind EP to RX CQ
        ret += fi_ep_bind(ep, &rxcq->fid, FI_RECV);

        //enable EP
        ret += fi_enable(ep);

        DBGASSERT(!ret);

        //clean-up
        fi_freeinfo(fi);
    }

    /*
     ***************************************************************************
     *
     * post non-blocking send/receive
     *
     ***************************************************************************
     */
    //post a directed recv
    ssize_t fm_post_rx(T *rx_buf, fi_addr_t from)
    {
        int ret;
        ret = fi_recv(ep, rx_buf, sizeof(T), NULL, from, NULL);
        DBGASSERT(!ret);
        return 0;
    }

    //post a from-any recv
    ssize_t fm_post_rx(T *buf)
    {
        int ret;
        ret = fi_recv(ep, buf, sizeof(T), NULL, FI_ADDR_UNSPEC, NULL);
        DBGASSERT(!ret);
        return 0;
    }

    //post a send
    ssize_t fm_post_tx(const T *tx_buf, fi_addr_t to)
    {
        int ret;
        ret = fi_send(ep, tx_buf, sizeof(T), NULL, to, NULL);
        assert(!ret);
        return 0;
    }

    /*
     ***************************************************************************
     *
     * support for blocking send/receive
     *
     ***************************************************************************
     */
    //spin-wait some completions on a completion queue
    int fm_spin_for_comp(struct fid_cq *cq)
    {
        struct fi_cq_err_entry comp;
        int ret;

        while (true)
        {
            ret = fi_cq_read(cq, &comp, 1); //non-blocking pop
            if (ret > 0)
                return 0;
            else if (ret < 0 && ret != -FI_EAGAIN)
                return ret;
        }

        return 0;
    }

    //wait on TX CQ
    int fm_get_tx_comp()
    {
        return fm_spin_for_comp(txcq);
    }

    //wait on RX CQ
    int fm_get_rx_comp()
    {
        return fm_spin_for_comp(rxcq);
    }

    //blocking send
    ssize_t fm_tx(const T *tx_buf, fi_addr_t to)
    {
        ssize_t ret;

        //send
        ret = fm_post_tx(tx_buf, to);
        DBGASSERT(!ret);

        //wait on TX CQ
        ret = fm_get_tx_comp();
        DBGASSERT(!ret);

        return ret;
    }

    //blocking recv
    ssize_t fm_rx(T *rx_buf, fi_addr_t from)
    {
        ssize_t ret;

        //recv
        ret = fm_post_rx(rx_buf, from);
        DBGASSERT(!ret);

        //wait on RX CQ
        ret = fm_get_rx_comp();
        DBGASSERT(!ret);

        return ret;
    }

    //blocking recv
    ssize_t fm_rx(T *rx_buf)
    {
        ssize_t ret;

        //recv
        ret = fm_post_rx(rx_buf);
        DBGASSERT(!ret);

        //wait on RX CQ
        ret = fm_get_rx_comp();

        return ret;
    }

    /*
     ***************************************************************************
     *
     * support for untyped send/receive (to be dropped)
     *
     ***************************************************************************
     */
    ssize_t fm_post_rx(void *rx_buf, size_t size, fi_addr_t from)
    {
        ssize_t ret = fi_recv(ep, rx_buf, size, NULL, from, NULL);
        DBGASSERT(!ret);
        return 0;
    }

    ssize_t fm_post_tx(const void *tx_buf, size_t size, fi_addr_t to)
    {
        ssize_t ret = fi_send(ep, tx_buf, size, NULL, to, NULL);
        assert(!ret);
        return 0;
    }

    ssize_t fm_tx(const void *tx_buf, size_t size, fi_addr_t to)
    {
        ssize_t ret = 0;

        //send
        ret += fm_post_tx(tx_buf, size, to);

        //wait on TX CQ
        ret += fm_get_tx_comp();

        return ret;
    }

    ssize_t fm_rx(void *rx_buf, size_t size, fi_addr_t from)
    {
        ssize_t ret = 0;

        //recv
        ret += fm_post_rx(rx_buf, size, from);

        //wait on RX CQ
        ret += fm_get_rx_comp();

        return ret;
    }
};

} /* namespace gam */

#endif /* GAM_INCLUDE_LINKS_HPP_ */
