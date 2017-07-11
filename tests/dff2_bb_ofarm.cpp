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
 * 
 * @file        dff2_bb_ofarm.cpp
 * @brief       a simple dff2 ordering farm with building blocks
 * @author      Maurizio Drocco
 * 
 * This example is a simple ordering farm.
 * (\see dff2_bb_farm.cpp)
 *
 */

#include <iostream>
#include <cassert>
#include <cmath>

#include <dff2/dff2.hpp>
#include <TrackingAllocator.hpp>

#define NWORKERS                 4
#define STREAMLEN             1024
#define RNG_LIMIT             1000

/*
 ***************************************************************************
 *
 * farm components
 *
 ***************************************************************************
 */
/*
 * Source generates a stream of random integers within [0, RNG_LIMIT)
 */
class EmitterLogic
{
private:
    unsigned n = 0;
    std::mt19937 rng;

public:
    dff2::token_t svc(dff2::RoundRobinSwitch &c)
    {
        if (n++ < STREAMLEN)
        {
            c.emit(gam::make_private<int>((int) (rng() % RNG_LIMIT)));
            return dff2::go_on;
        }
        return dff2::eos;
    }

    void svc_init()
    {
    }

    void svc_end()
    {
    }
};

typedef dff2::Source<dff2::RoundRobinSwitch, gam::private_ptr<int>, //
        EmitterLogic> Emitter;

/*
 * Lowpass selects input integers below THRESHOLD
 */
class WorkerLogic
{
public:
    dff2::token_t svc(gam::private_ptr<int> &in, dff2::RoundRobinMerge &c)
    {
        auto local_in = in.local();
        c.emit(gam::make_private<float>(std::sqrt(*local_in)));
        return dff2::go_on;
    }

    void svc_init()
    {
    }

    void svc_end()
    {
    }
};

typedef dff2::Filter<dff2::RoundRobinSwitch, dff2::RoundRobinMerge, //
        gam::private_ptr<int>, gam::private_ptr<float>, //
        WorkerLogic> Worker;

/*
 * Sink sums up and check
 */
class CollectorLogic
{
private:
    float sum = 0;
    std::mt19937 rng;

public:
    void svc(gam::private_ptr<float> &in)
    {
        auto local_in = in.local();
        std::cout << *local_in << std::endl;
        sum += *local_in;
    }

    void svc_init()
    {
    }

    void svc_end()
    {
        float res = 0;
        for (unsigned i = 0; i < STREAMLEN; ++i)
        {
            int x = (int) (rng() % RNG_LIMIT);
            res += std::sqrt(x);
        }
        if (res != sum)
        {
            fprintf(stderr, "sum=%f exp=%f\n", sum, res);
            exit(1);
        }
    }
};

typedef dff2::Sink<dff2::RoundRobinMerge, gam::private_ptr<float>, //
        CollectorLogic> Collector;

/*
 *******************************************************************************
 *
 * main
 *
 *******************************************************************************
 */
int main(int argc, char * argv[])
{
    dff2::RoundRobinSwitch e2w;
    dff2::RoundRobinMerge w2c;

    dff2::add(Emitter(e2w));
    for (unsigned i = 0; i < NWORKERS; ++i)
        dff2::add(Worker(e2w, w2c));
    dff2::add(Collector(w2c));

    dff2::run();

    return 0;
}
