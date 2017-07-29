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
 * @file fl_common.hpp
 * @author Maurizio Drocco
 * @brief implements fl_common class
 *
 * @ingroup internals
 *
 */

#ifndef GAM_INCLUDE_LINKS_IMPLEMENTATIONS_FL_COMMON_HPP_
#define GAM_INCLUDE_LINKS_IMPLEMENTATIONS_FL_COMMON_HPP_

#include <cstdint>
#include <cassert>
#include <cstdio>

#include <rdma/fi_domain.h>

#include "utils.hpp"

namespace gam {

static struct fid_fabric *fabric;
static struct fid_domain *domain_;

class fl_common {
public:
    ~fl_common()
    {
        int ret = 0;

        if (txctx)
            delete txctx;
        if (rxctx)
            delete rxctx;
        if (ep_)
            ret += fi_close(&ep_->fid);
        if (rxcq)
            ret += fi_close(&rxcq->fid);
        if (txcq)
            ret += fi_close(&txcq->fid);

        txctx = nullptr;
        rxctx = nullptr;
        ep_ = nullptr;
        rxcq = nullptr;
        txcq = nullptr;

        DBGASSERT(!ret);
    }

    static void fi_getinfo_(fi_info **fi, //
            char *node, char *service, //
            uint64_t flags, enum fi_ep_type ep_type)
    {
        fi_info *hints = fi_allocinfo();
        int ret = 0;

        //todo check hints

        //prepare for querying fabric contexts
        hints->caps = FI_MSG | FI_DIRECTED_RECV;
        hints->ep_attr->type = ep_type;
        hints->mode = FI_CONTEXT;

        //query fabric contexts
        ret = fi_getinfo(FI_VERSION(1, 3), node, service, flags, hints, fi);
        assert(!ret);

        for (fi_info *cur = *fi; cur; cur = cur->next)
        {
            uint32_t prot = cur->ep_attr->protocol;
            if (prot == FI_PROTO_RXM)
            {
                fi_info *tmp = fi_dupinfo(cur);
                fi_freeinfo(*fi);
                *fi = tmp;

#ifdef GAM_LOG
                fprintf(stderr, "> promoted FI_PROTO_RXM provider\n");
#endif

                break;
            }
        }

        fi_freeinfo(hints);

#ifdef GAM_LOG
        struct fi_info *cur;
        for (cur = *fi; cur; cur = cur->next)
        {
            fprintf(stderr, "provider: %s\n", cur->fabric_attr->prov_name);
            fprintf(stderr, "    fabric: %s\n", cur->fabric_attr->name);
            fprintf(stderr, "    domain: %s\n", cur->domain_attr->name);
            fprintf(stderr, "    version: %d.%d\n",
                    FI_MAJOR(cur->fabric_attr->prov_version),
                    FI_MINOR(cur->fabric_attr->prov_version));
            fprintf(stderr, "    type: %s\n",
                    fi_tostr(&cur->ep_attr->type, FI_TYPE_EP_TYPE));
            fprintf(stderr, "    protocol: %s\n",
                    fi_tostr(&cur->ep_attr->protocol, FI_TYPE_PROTOCOL));
        }
#endif
    }

    static void init(fi_info *fi)
    {
        int ret = 0;

        //init fabric context
        ret += fi_fabric(fi->fabric_attr, &fabric, NULL);

        //init domain
        ret += fi_domain(fabric, fi, &domain_, NULL);

        assert(!ret);
    }

    static void fini()
    {
        int ret = 0;

        ret += fi_close(&domain_->fid);
        ret += fi_close(&fabric->fid);

        DBGASSERT(!ret);
    }

    fid_ep *ep() const
    {
        return ep_;
    }

    static fid_domain *domain() {
        return domain_;
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

    //wait on TX CQ
    int fl_get_tx_comp()
    {
        return fl_spin_for_comp(txcq);
    }

    //wait on RX CQ
    int fl_get_rx_comp()
    {
        return fl_spin_for_comp(rxcq);
    }

    /*
     ***************************************************************************
     *
     * support for untyped send/receive (to be dropped)
     *
     ***************************************************************************
     */
    ssize_t fl_post_rx(void *rx_buf, size_t size, fi_addr_t from)
    {
        int ret;
        while (1)
        {
            ret = fi_recv(ep_, rx_buf, size, NULL, from, rxctx);
            if (!ret)
                break;
            assert(ret == -FI_EAGAIN);
        }

        return 0;
    }

    ssize_t fl_post_tx(const void *tx_buf, size_t size, fi_addr_t to)
    {
        int ret;
        while (1)
        {
            ret = fi_send(ep_, tx_buf, size, NULL, to, txctx);
            if (!ret)
                break;
            assert(ret == -FI_EAGAIN);
        }

        return 0;
    }

    ssize_t fl_tx(const void *tx_buf, size_t size, fi_addr_t to)
    {
        ssize_t ret = 0;

        //send
        ret += fl_post_tx(tx_buf, size, to);

        //wait on TX CQ
        ret += fl_get_tx_comp();

        return ret;
    }

    ssize_t fl_rx(void *rx_buf, size_t size, fi_addr_t from)
    {
        ssize_t ret = 0;

        //recv
        ret += fl_post_rx(rx_buf, size, from);

        //wait on RX CQ
        ret += fl_get_rx_comp();

        return ret;
    }

    void create_endpoint(fi_info *fi)
    {
        struct fi_cq_attr cq_attr;
        int ret = 0;

        //init TX CQ (completion queue)
        memset(&cq_attr, 0, sizeof(fi_cq_attr));
        cq_attr.format = FI_CQ_FORMAT_CONTEXT;
        cq_attr.wait_obj = FI_WAIT_NONE; //async
        cq_attr.size = fi->tx_attr->size;
        ret += fi_cq_open(domain_, &cq_attr, &txcq, &txcq);

        //init RX CQ
        cq_attr.size = fi->rx_attr->size;
        ret += fi_cq_open(domain_, &cq_attr, &rxcq, &rxcq);

        //create endpoint
        ret += fi_endpoint(domain_, fi, &ep_, NULL);

        //bind EP to TX CQ
        ret += fi_ep_bind(ep_, &txcq->fid, FI_SEND);

        //bind EP to RX CQ
        ret += fi_ep_bind(ep_, &rxcq->fid, FI_RECV);

        DBGASSERT(!ret);

        txctx = new fi_context();
        rxctx = new fi_context();
    }

    void enable_endpoint()
    {
        int ret = fi_enable(ep_);

        DBGASSERT(!ret);
    }

private:
    struct fid_ep *ep_ = nullptr; //end point
    struct fid_cq *txcq = nullptr, *rxcq = nullptr; //completion queues
    fi_context *txctx = nullptr, *rxctx = nullptr;

    /*
     ***************************************************************************
     *
     * support for blocking send/receive
     *
     ***************************************************************************
     */
    //spin-wait some completions on a completion queue
    int fl_spin_for_comp(struct fid_cq *cq)
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
};
} /* namespace gam */

#endif /* GAM_INCLUDE_LINKS_IMPLEMENTATIONS_FL_COMMON_HPP_ */
