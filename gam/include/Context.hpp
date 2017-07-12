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
 * @defgroup internals Internals
 *
 * all stuff working under the hood
 */

/**
 * @file        Context.hpp
 * @brief       implements Context class
 * @author      Maurizio Drocco
 * 
 * @ingroup     internals
 *
 * @todo remote-load by RMA operations + remote memory allocation
 * @todo move local memory management to a dedicated module
 * @todo friendly error reporting
 * @todo define thread-safeness for each function
 */
#ifndef CONTEXT_HPP_
#define CONTEXT_HPP_

#include <vector>
#include <cstdlib>
#include <cassert>
#include <cstring> //memcpy
#include <random>
#include <thread>
#include <atomic>
#include <memory>

#include "Logger.hpp"
#include "utils.hpp"
#include "defs.hpp"
#include "Links.hpp"
#include "GlobalPointer.hpp"
#include "View.hpp"
#include "MemoryController.hpp"
#include "Cache.hpp"
#include "backend_ptr.hpp"
#include "TrackingAllocator.hpp"

namespace gam
{

/**
 * Context represents the executor state.
 */
class Context
{
public:
    void init()
    {
        char *env, *tmp;

        /*
         * read rank from env
         */
        env = std::getenv("GAM_RANK");
        assert(env);
        rank_ = strtoull(env, &tmp, 10);
        USRASSERT(rank_ <= GlobalPointer::max_home);

        /*
         * initialize logger
         */
        env = std::getenv("GAM_LOG_PREFIX");
        assert(env);
        LOGGER_INIT(env, rank_);
        LOGLN("CTX rank = %llu", rank_);

        /*
         * read cardiality from env
         */
        env = std::getenv("GAM_CARDINALITY");
        assert(env);
        cardinality_ = strtoull(std::getenv("GAM_CARDINALITY"), &tmp, 10);
        LOGLN("CTX cardinality = %llu", cardinality_);
        USRASSERT(cardinality_ <= GlobalPointer::max_home + 1);

        /*
         * read node and service names from env
         */
        struct node_t
        {
            char *host, *svc_pap, *svc_local, *svc_remote;
        } node;
        std::vector<node_t> nodes;
        std::string env_prefix = "GAM_", env_name;
        for (unsigned long long i = 0; i < cardinality_; ++i)
        {
            env_name = env_prefix + "NODE_" + std::to_string(i);
            assert((node.host = std::getenv(env_name.c_str())));
            env_name = env_prefix + "SVC_PAP_" + std::to_string(i);
            assert((node.svc_pap = std::getenv(env_name.c_str())));
            env_name = env_prefix + "SVC_MEM_" + std::to_string(i);
            assert((node.svc_local = std::getenv(env_name.c_str())));
            env_name = env_prefix + "SVC_DMN_" + std::to_string(i);
            assert((node.svc_remote = std::getenv(env_name.c_str())));
            nodes.push_back(node);
            LOGLN(
                    "CTX rank %llu: node=%s svc_pap=%s svc_mem=%s svc_dmn=%s", //
                    i, node.host, node.svc_pap, node.svc_local,
                    node.svc_remote);
        }

        /*
         * initialize links
         */
        init_links();

        /*
         * create links
         */
        pap_links = new Links<pap_pointer>(cardinality_, rank_);
        local_links = new Links<daemon_pointer>(cardinality_, rank_);
        remote_links = new Links<daemon_pointer>(cardinality_, rank_);

        /*
         * add send links
         */
        for (unsigned long long i = 0; i < cardinality_; ++i)
        {
            if (i != rank_)
            {
                pap_links->add(i, nodes[i].host, nodes[i].svc_pap); //send push
                local_links->add(i, nodes[i].host, nodes[i].svc_remote); //send rc + rload req
                remote_links->add(i, nodes[i].host, nodes[i].svc_local); //send rload rep
            }
        }

        /*
         * add receive links
         */
        pap_links->add(nodes[rank_].host, nodes[rank_].svc_pap); //recv push (i.e. pull)
        local_links->add(nodes[rank_].host, nodes[rank_].svc_local); //recv rload rep
        remote_links->add(nodes[rank_].host, nodes[rank_].svc_remote); //recv rc + rload req

        /*
         * spawn daemon thread
         */
        daemon = new std::thread(Daemon(*this));
    }

    void finalize()
    {
        /*
         * finalize and join daemon thread
         */
        daemon_termination = true;
        daemon->join();
        delete daemon;

        /*
         * finalize cache
         */
        cache.finalize();

        //clean-up
        delete pap_links;
        delete local_links;
        delete remote_links;

        /*
         * finalize logger
         */
        LOGGER_FINALIZE(rank_);
    }

    executor_id rank() const
    {
        return rank_;
    }

    executor_id cardinality() const
    {
        return cardinality_;
    }

    /*
     ***************************************************************************
     *
     * global memory mapping
     *
     ***************************************************************************
     */
    /*
     * map a fresh global address (for public memory) to local pointer
     */
    template<class T, typename Deleter>
    GlobalPointer mmap_public(T *lp, Deleter d)
    {
        GlobalPointer res = mmap_global<AL_PUBLIC>(lp, d);
        uint64_t a = res.address();

        /* initialize owner and child fields with void values */
        view.bind_owner(a, (executor_id) GlobalPointer::max_home + 1);
        view.bind_child(a, nullptr);

        return res;
    }

    /*
     * map a fresh global address (for public memory) to local pointer
     */
    template<class T, typename Deleter>
    GlobalPointer mmap_private(T *lp, Deleter d)
    {
        GlobalPointer res = mmap_global<AL_PRIVATE>(lp, d);
        uint64_t a = res.address();

        /* update owner */
        view.bind_owner(a, rank_);

        /* update parenthood information */
        view.bind_parent(lp, a);
        view.bind_child(a, lp);

        return res;
    }

    void unmap(const GlobalPointer &p)
    {
        DBGASSERT(p.is_address());
        LOGLN_OS("CTX unmapping p=" << p);
        uint64_t a = p.address();

        /* clean up parenthood */
        if (view.access_level(a) == AL_PRIVATE)
        {
            DBGASSERT(view.has_child(a));
            DBGASSERT(view.has_parent(view.child(a)));
            view.unbind_parent(view.child(a));
        }
        else
            DBGASSERT(!view.has_child(a));

        munmap(p);
    }

    /*
     ***************************************************************************
     *
     * capability passing: push and pull
     *
     ***************************************************************************
     */
    inline void push_public(const GlobalPointer &p, const executor_id e)
    {
        DBGASSERT(p.is_address());
        uint64_t a = p.address();
        DBGASSERT(view.access_level(a) == AL_PUBLIC);
        LOGLN_OS("CTX push public=" << p << " to=" << e);

        pap_pointer buf;
        buf.p = p;
        buf.al = AL_PUBLIC;
        buf.author = view.author(a);
        pap_links->send(buf, e);
    }

    inline void push_private(const GlobalPointer &p, const executor_id e)
    {
        DBGASSERT(p.is_address());
        LOGLN_OS("CTX push private=" << p << " to=" << e);
        uint64_t a = p.address();
        DBGASSERT(view.access_level(a) == AL_PRIVATE);
        USRASSERT(view.owner(a) == rank_);

        /* switch ownership */
        view.bind_owner(a, e);

        pap_pointer buf;
        buf.p = p;
        buf.al = AL_PRIVATE;
        buf.author = view.author(a);
        pap_links->send(buf, e);
    }

    inline void push_reserved(const GlobalPointer &p, const executor_id e)
    {
        DBGASSERT(!p.is_address());
        LOGLN_OS("CTX push reserved =" << p << " to=" << e);

        pap_pointer buf;
        buf.p = p;
        pap_links->send(buf, e);
    }

    /*
     * blocking pull a public address from specific executor
     */
    inline GlobalPointer pull_public(const executor_id e)
    {
        LOGLN_OS("CTX pull public from=" << e);

        pap_pointer buf;
        pap_links->recv(buf, e);

        return pulled_public(buf);
    }

    /*
     * blocking pull public address from any executor
     */
    inline GlobalPointer pull_public()
    {
        LOGLN_OS("CTX pull public from any");

        pap_pointer buf;
        pap_links->recv(buf);

        return pulled_public(buf);
    }

    /*
     * blocking pull private address from specific executor
     */
    inline GlobalPointer pull_private(const executor_id e)
    {
        LOGLN_OS("CTX pull private from=" << e);
        pap_pointer buf;
        pap_links->recv(buf, e);

        return pulled_private(buf);
    }

    /*
     * blocking pull private address from any executor
     */
    inline GlobalPointer pull_private()
    {
        LOGLN_OS("CTX pull private from any");
        pap_pointer buf;
        pap_links->recv(buf);

        return pulled_private(buf);
    }

    /*
     ***************************************************************************
     *
     * conversion to local memory pointers
     *
     ***************************************************************************
     */
    /*
     * local_public generates a local copy of public memory
     */
    template<typename T>
    inline std::shared_ptr<T> local_public(const GlobalPointer &p)
    {
        DBGASSERT(p.is_address());

        LOGLN_OS("CTX local public " << p);
        uint64_t a = p.address();
        DBGASSERT(view.access_level(a) == AL_PUBLIC);

        /* allocate local memory */
        T *lp = (T *) MALLOC(sizeof(T));

        /* load either locally or remotely */
        if (view.author(a) == rank_)
            local_load(lp, a);
        else if (!cache.load(lp, a))
        {
            forward_load(lp, p);
            cache.store(a, lp);
        }

        /* generate a smart pointer with custom deleter to match allocation */
        return std::shared_ptr<T>((T*) lp, [](T *p_)
        {   FREE((void *)p_);});
    }

    /*
     * local_private returns the pointer associated to (private) global address
     */
    template<typename T>
    T *local_private(const GlobalPointer &p)
    {
        DBGASSERT(p.is_address());
        uint64_t a = p.address();
        LOGLN_OS("CTX local private " << p);
        DBGASSERT(view.access_level(a) == AL_PRIVATE);

        executor_id auth = view.author(a);

        if (auth != rank_)
        {
            /* steal memory and swap authorship */
            withdraw<T>(p);
            forward_reset(p, auth);
        }

        return reinterpret_cast<T *>(view.committed(a)->get());
    }

    /*
     ***************************************************************************
     *
     * publishing private addresses
     *
     ***************************************************************************
     */
    /*
     * remap private memory to a fresh public address
     */
    template<typename T>
    GlobalPointer publish(const GlobalPointer &p)
    {
        GlobalPointer p_;

        LOGLN_OS("CTX publishing p=" << p);
        DBGASSERT(p.is_address());
        DBGASSERT(is_private(p));
        DBGASSERT(am_owner(p));
        uint64_t a = p.address();
        executor_id auth = view.author(a);

        /* map to fresh global address */
        p_ = name_allocator();
        uint64_t a_ = p_.address();
        view.bind_access_level(a_, AL_PUBLIC);

        /* steal memory */
        backend_ptr *bp_;
        if (auth == rank_)
        {
            DBGASSERT(view.has_child(a));
            DBGASSERT(view.has_parent(view.child(a)));

            view.unbind_parent(view.child(a));
            bp_ = view.committed(a);
        }
        else
        {
            DBGASSERT(!view.has_child(a));

            /* allocate backend memory */
            T* tmp = (T*) MALLOC(sizeof(T));
            using bp_t = backend_typed_ptr<T, void(*)(T*)>;
            auto tbp_ = NEW<bp_t>(tmp, TYPED_FREE<T>);

            /* remote load */
            forward_load(tbp_->typed_get(), p);
            bp_ = tbp_;

            /* notify remote author  */
            forward_reset(p, auth);
        }

        /* clear old entry */
        view.unmap(a);

        /* update fresh entry */
        view.bind_committed(a_, bp_);
        view.bind_author(a_, rank_);
        view.bind_owner(a_, (executor_id) GlobalPointer::max_home + 1); //dummy
        view.bind_child(a_, nullptr); //dummy

        return p_;
    }

    /*
     ***************************************************************************
     *
     * utility query functions
     *
     ***************************************************************************
     */
    bool is_public(const GlobalPointer &p)
    {
        DBGASSERT(p.is_address());
        return view.access_level(p.address()) == AL_PUBLIC;
    }

    bool is_private(const GlobalPointer &p)
    {
        DBGASSERT(p.is_address());
        return view.access_level(p.address()) == AL_PRIVATE;
    }

    bool am_owner(const GlobalPointer &p)
    {
        DBGASSERT(p.is_address());
        DBGASSERT(is_private(p));
        return view.owner(p.address()) == rank_;
    }

    bool am_author(const GlobalPointer &p)
    {
        DBGASSERT(p.is_address());
        return view.author(p.address()) == rank_;
    }

    executor_id author(const GlobalPointer &p)
    {
        DBGASSERT(p.is_address());
        return view.author(p.address());
    }

    template<typename T>
    bool has_parent(T *lp)
    {
        return view.has_parent((void *) lp);
    }

    template<typename T>
    GlobalPointer parent(T *lp)
    {
        return GlobalPointer(view.parent((void *) lp));
    }

    /*
     ***************************************************************************
     *
     * public pointers support
     *
     ***************************************************************************
     */
    inline void rc_init(const GlobalPointer &p)
    {
        DBGASSERT(p.is_address());
        mc.rc_init(p.address());
    }

    inline void rc_inc(const GlobalPointer &p)
    {
        DBGASSERT(p.is_address());
        uint64_t a = p.address();
        DBGASSERT(view.access_level(a) == AL_PUBLIC);

        if (view.author(a) == rank_)
            mc.rc_inc(a);
        else
            forward_inc(p);
    }

    inline void rc_dec(const GlobalPointer &p)
    {
        DBGASSERT(p.is_address());
        uint64_t a = p.address();
        DBGASSERT(view.access_level(a) == AL_PUBLIC);

        if (view.author(a) == rank_)
        {
            if (mc.rc_dec(a) == 0)
                /*
                 * destroy and un-bind committed memory
                 */
                unmap(a);
        }
        else
            forward_dec(p);
    }

    /*
     ***************************************************************************
     *
     * private pointers support
     *
     ***************************************************************************
     */
    inline void forward_reset(const GlobalPointer &p, const executor_id to)
    {
        DBGASSERT(p.is_address());
        LOGLN("CTX fwd -1 %llu dest=%lu", p.address(), to);
        daemon_pointer dp;
        dp.op = daemon_pointer::PVT_RESET;
        dp.from = rank_;
        dp.p = p;
        local_links->send(dp, to);
    }

private:
    executor_id rank_, cardinality_;
    std::vector<std::string> hostnames;

    View view; //concurrent memory table
    MemoryController mc; //concurrent reference counting table
    Cache cache;

    std::thread *daemon;
    std::atomic<char> daemon_termination;

    std::mt19937 name_allocator;

    /*
     ***************************************************************************
     *
     * links are organized in different TCP services (i.e., ports)
     *
     ***************************************************************************
     */
    struct pap_pointer
    {
        GlobalPointer p;
        executor_id author = 0;
        AccessLevel al;
    };

    struct daemon_pointer
    {
        enum
        {
            RLOAD, RC_INC, RC_DEC, PVT_RESET, DMN_END
        } op;
        size_t size; //remote-load size
        executor_id from;
        GlobalPointer p;
    };

    /*
     * links for pushing and pulling pointers (svc A)
     */
    Links<pap_pointer> *pap_links;

    /*
     * links for:
     * - sending   reference-counting and remote-load requests (svc B)
     * - receiving remote-load responses (svc C)
     */
    Links<daemon_pointer> *local_links;

    /*
     * links for:
     * - receiving reference-counting and remote-load requests (svc B)
     * - sending   remote-load responses (svc C)
     */
    Links<daemon_pointer> *remote_links;

    /*
     ***************************************************************************
     *
     * Daemon represents the utility thread associated to the execution context
     * that handles memory requests from other executors
     *
     ***************************************************************************
     */
    class Daemon
    {
    public:
        Daemon(Context &ctx)
                : ctx(ctx), cnt(ctx.cardinality_ - 1)
        {
        }

        void operator()()
        {
            if (cnt)
            {
                ctx.remote_links->nb_recv(p);

                LOGLN_OS(
                        "DMN start serving remote requests [tid=" << std::this_thread::get_id() << "]");
                while (!ctx.daemon_termination)
                    poll_iteration();
            }

            /* broadcast termination to rc-consumer links */
            LOGLN("DMN broadcast termination", p.p.address());
            daemon_pointer p_end;
            p_end.op = daemon_pointer::DMN_END;
            p_end.from = ctx.rank_;
            ctx.local_links->broadcast(p_end);

            LOGLN("DMN keep serving remote requests", p.p.address());
            while (cnt)
                poll_iteration();
        }

    private:
        Context &ctx;
        executor_id cnt; //terminated partitions
        daemon_pointer p;

        void poll_iteration()
        {
            if (ctx.remote_links->nb_poll())
            {
                /* handle the incoming request */
                uint64_t a = p.p.address();
                switch (p.op)
                {
                case daemon_pointer::RC_INC:
                    LOGLN("DMN recv +1 %llu from %lu", a, p.from);
                    DBGASSERT(ctx.view.author(a) == ctx.rank_)
                    ;
                    ctx.mc.rc_inc(a);
                    break;
                case daemon_pointer::RC_DEC:
                    LOGLN("DMN recv -1 %llu from %lu", a, p.from);
                    DBGASSERT(ctx.view.author(a) == ctx.rank())
                    ;
                    if (ctx.mc.rc_dec(a) == 0)
                        ctx.unmap(a);
                    break;
                case daemon_pointer::PVT_RESET:
                    LOGLN("DMN recv PVT -1 %llu from %lu", a, p.from);
                    DBGASSERT(ctx.view.author(a) == ctx.rank_)
                    ;
                    DBGASSERT(ctx.view.committed(a) != nullptr)
                    ;
                    ctx.unmap(a);
                    break;
                case daemon_pointer::RLOAD:
                    LOGLN("DMN recv RLOAD %llu from %lu", a, p.from);
                    DBGASSERT(ctx.view.author(a) == ctx.rank_)
                    ;
                    DBGASSERT(ctx.view.committed(a) != nullptr)
                    ;
                    ctx.remote_links->raw_send(ctx.view.committed(a)->get(), //
                            p.size, p.from);
                    break;
                case daemon_pointer::DMN_END:
                    LOGLN("DMN recv RC_END from %lu", p.from);
                    --cnt;
                    break;
                default:
                    DBGASSERT(false)
                    ;
                    break;
                }

                /* accept another token */
                ctx.remote_links->nb_recv(p);
            }
        }
    };

    template<AccessLevel al, class T, typename Deleter>
    GlobalPointer mmap_global(T *lp, Deleter d)
    {
        GlobalPointer res = GlobalPointer(name_allocator(), rank_);
        uint64_t a = res.address();

        LOGLN("CTX mmap global=%llu -> local=%p", a, lp);

        /* implicit commit */
        DBGASSERT(view.committed(a) == nullptr);
        using bp_t = backend_typed_ptr<T, Deleter>;
        bp_t *bp = NEW<bp_t>(lp, d);
        view.bind_committed(a, bp);

        /* update view information */
        view.bind_access_level(a, al);
        view.bind_author(a, rank_);

        return res;
    }

    void munmap(const GlobalPointer &p)
    {
        uint64_t a = p.address();

        backend_ptr *cm = view.committed(a);
        DBGASSERT(cm != nullptr);

        /* clean up view */
        view.unmap(a);

        /* finally release committed memory */
        DELETE(cm);
    }

    GlobalPointer pulled_public(const pap_pointer &buf)
    {
        GlobalPointer res;

        if (buf.p.is_address())
        {
            /* ensure a public pointer was pulled */
            USRASSERT(buf.al == AL_PUBLIC);
            LOGLN_OS("CTX pulled public=" << buf.p);

            uint64_t a = buf.p.address();
            view.bind_access_level(a, buf.al);
            view.bind_owner(a, (executor_id) GlobalPointer::max_home + 1);
            view.bind_author(a, buf.author);
            view.bind_committed(a, nullptr);
        }
        else
            LOGLN_OS("CTX pulled reserved=" << buf.p);

        /* prepare output */
        res = buf.p;

        return res;
    }

    GlobalPointer pulled_private(const pap_pointer &buf)
    {
        GlobalPointer res;

        if (buf.p.is_address())
        {
            /* ensure a private pointer was pulled */
            USRASSERT(buf.al == AL_PRIVATE);
            LOGLN_OS("CTX pulled private=" << buf.p);

            uint64_t a = buf.p.address();
            if (!view.mapped(a) || buf.author != rank_)
            {
                view.bind_access_level(a, AL_PRIVATE);
                view.bind_author(a, buf.author);
                view.bind_committed(a, nullptr);
            }

            /* take ownership */
            view.bind_owner(a, rank_);
        }
        else
            LOGLN_OS("CTX pulled reserved=" << buf.p);

        /* prepare output */
        res = buf.p;

        return res;
    }

    template<typename T>
    inline void local_load(T *lp, uint64_t a)
    {
        LOGLN("CTX load %p size=%zu %llu", lp, sizeof(T), a);
        DBGASSERT(view.committed(a) != nullptr);
        memcpy(lp, view.committed(a)->get(), sizeof(T));
    }

    /*
     * actually transfer the private pointer to local memory
     *
     * @param p global pointer to be withdrawn
     *
     * @retval a raw pointer to the local memory for p
     */
    template<typename T>
    inline T *withdraw(const GlobalPointer &p)
    {
        DBGASSERT(p.is_address());
        LOGLN_OS("CTX withdraw=" << p);
        uint64_t a = p.address();
        DBGASSERT(view.access_level(a) == AL_PRIVATE);
        DBGASSERT(!am_author(p));
        DBGASSERT(am_owner(p));

        /* allocate backend memory */
        DBGASSERT(view.committed(a) == nullptr);
        T* tmp = (T*) MALLOC(sizeof(T));
        using bp_t = backend_typed_ptr<T, void(*)(T*)>;
        bp_t *bp = NEW<bp_t>(tmp, TYPED_FREE<T>);

        /* bind parenthood */
        T *child = bp->typed_get();
        view.bind_parent(child, a);
        view.bind_child(a, child);

        /* issue remote load */
        forward_load(child, p);

        /* take ownership */
        view.bind_committed(a, bp);
        view.bind_author(a, rank_);

        return bp->typed_get();
    }

    template<typename T>
    void forward_load(T *lp, const GlobalPointer &p)
    {
        DBGASSERT(p.is_address());
        uint64_t a = p.address();
        executor_id to = view.author(a);
        LOGLN("CTX fwd LOAD size=%zu %llu dest=%lu", sizeof(T), a, to);

        /* send remote-load request */
        daemon_pointer dp;
        dp.op = daemon_pointer::RLOAD;
        dp.p = p;
        dp.size = sizeof(T);
        dp.from = rank_;
        local_links->send(dp, to);

        /* receive remote-load reply */
        local_links->raw_recv(lp, sizeof(T), to);
    }

    inline void forward_inc(const GlobalPointer &p)
    {
        DBGASSERT(p.is_address());
        uint64_t a = p.address();
        executor_id dest = view.author(a);
        LOGLN("CTX fwd +1 %llu dest=%lu", a, dest);
        daemon_pointer dp;
        dp.op = daemon_pointer::RC_INC;
        dp.from = rank_;
        dp.p = p;
        local_links->send(dp, dest);
    }

    inline void forward_dec(const GlobalPointer &p)
    {
        DBGASSERT(p.is_address());
        uint64_t a = p.address();
        executor_id dest = view.author(a);
        LOGLN("CTX fwd -1 %llu dest=%lu", a, dest);
        daemon_pointer dp;
        dp.op = daemon_pointer::RC_DEC;
        dp.from = rank_;
        dp.p = p;
        local_links->send(dp, dest);
    }
};

static Context ctx;

} /* namespace gam */

#endif /* CONTEXT_HPP_ */
