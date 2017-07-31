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

static struct fid_fabric *fl_fabric_;
static struct fid_domain *fl_domain_;

void fl_getinfo(fi_info **fi, //
        char *node, char *service, //
        uint64_t flags, enum fi_ep_type ep_type)
{
    fi_info *hints = fi_allocinfo();
    int ret = 0;

    //todo check hints

    //prepare for querying fabric contexts
    hints->caps = FI_MSG | FI_DIRECTED_RECV;
    hints->ep_attr->type = ep_type;

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

void fl_init(fi_info *fi)
{
    int ret = 0;

    //init fabric context
    ret += fi_fabric(fi->fabric_attr, &fl_fabric_, NULL);

    //init domain
    ret += fi_domain(fl_fabric_, fi, &fl_domain_, NULL);

    assert(!ret);
}

void fl_fini()
{
    int ret = 0;

    ret += fi_close(&fl_domain_->fid);
    ret += fi_close(&fl_fabric_->fid);

    DBGASSERT(!ret);
}

void fl_create_endpoint(fi_info *fi, fid_ep **ep, fid_cq **txcq, fid_cq **rxcq)
{
    struct fi_cq_attr cq_attr;
    int ret = 0;

    //init TX CQ (completion queue)
    memset(&cq_attr, 0, sizeof(fi_cq_attr));
    cq_attr.format = FI_CQ_FORMAT_CONTEXT;
    cq_attr.wait_obj = FI_WAIT_NONE; //async
    cq_attr.size = fi->tx_attr->size;
    ret += fi_cq_open(fl_domain_, &cq_attr, txcq, txcq);

    //init RX CQ
    cq_attr.size = fi->rx_attr->size;
    ret += fi_cq_open(fl_domain_, &cq_attr, rxcq, rxcq);

    //create endpoint
    ret += fi_endpoint(fl_domain_, fi, ep, NULL);

    //bind EP to TX CQ
    ret += fi_ep_bind(*ep, &(*txcq)->fid, FI_SEND);

    //bind EP to RX CQ
    ret += fi_ep_bind(*ep, &(*rxcq)->fid, FI_RECV);

    DBGASSERT(!ret);
}

void fl_release_endpoint(fid_ep **ep, fid_cq **txcq, fid_cq **rxcq)
{
    int ret = 0;

    if (ep)
        ret += fi_close(&(*ep)->fid);
    if (rxcq)
        ret += fi_close(&(*rxcq)->fid);
    if (txcq)
        ret += fi_close(&(*txcq)->fid);

    DBGASSERT(!ret);

    *ep = nullptr;
    *rxcq = nullptr;
    *txcq = nullptr;
}

/*
 ***************************************************************************
 *
 * support for untyped send/receive (to be dropped)
 *
 ***************************************************************************
 */
ssize_t fl_post_rx(fid_ep *ep, void *rxbuf, size_t size, fi_addr_t from)
{
    int ret;
    while (1)
    {
        ret = fi_recv(ep, rxbuf, size, NULL, from, NULL);
        if (!ret)
            break;
        assert(ret == -FI_EAGAIN);
    }

    return 0;
}

ssize_t fl_post_tx(fid_ep *ep, const void *txbuf, size_t size, fi_addr_t to)
{
    int ret;
    while (1)
    {
        ret = fi_send(ep, txbuf, size, NULL, to, NULL);
        if (!ret)
            break;
        assert(ret == -FI_EAGAIN);
    }

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

ssize_t fl_tx(fid_ep *ep, fid_cq *txcq, const void *tx_buf, size_t size, //
        fi_addr_t to)
{
    ssize_t ret = 0;

    //send
    ret += fl_post_tx(ep, tx_buf, size, to);

    //wait on TX CQ
    ret += fl_spin_for_comp(txcq);

    return ret;
}

} /* namespace gam */

#endif /* GAM_INCLUDE_LINKS_IMPLEMENTATIONS_FL_COMMON_HPP_ */
