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

static struct fi_info *fl_info_;
static struct fid_fabric *fl_fabric_;
static struct fid_domain *fl_domain_;
static const char *fl_node_;

void fl_node(const char *n)
{
    fl_node_ = n;
}

void fl_getinfo(fi_info **fi, //
        const char *node, const char *service, //
        uint64_t flags, enum fi_ep_type ep_type, uint64_t caps)
{
    fi_info *hints = fi_allocinfo();
    int ret = 0;

    //prepare for querying fabric contexts
    hints->caps = FI_MSG | caps;
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
