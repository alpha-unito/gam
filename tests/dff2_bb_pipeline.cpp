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
 * @file        dff2_bb_pipeline.cpp
 * @brief       a simple dff2 pipeline with building blocks
 * @author      Maurizio Drocco
 * 
 * This example is a simple 4-stage pipeline:
 * - stage 1 generates a stream of random integers within a range
 * - stage 2 is a low-pass filter that discards all numbers above a threshold
 * - stage 3 computes sqrt
 * - stage 4 writes the stream to standard output
 *
 * Each stage represents a relevant instance of DFF2 node:
 * - Stage 1 is a SOURCE node:
 *   it produces a stream (no input channel)
 * - Stage 2 is a type-preserving FILTER:
 *   it reads an input stream from its input channel and emits an output stream
 *   of the same type on its output channel
 * - Stage 3 is a type-changing FILTER
 * - Stage 4 is a SINK node:
 * it consumes a stream (no output channel)
 *
 */

#include <iostream>
#include <cassert>
#include <cmath>

#include <dff2/dff2.hpp>
#include <TrackingAllocator.hpp>

#define STREAMLEN             1024
#define RNG_LIMIT             1000
#define THRESHOLD  (RNG_LIMIT / 2)

/*
 * we use unicast communicators to implement 1-to-1 pipeline channels
 */

/*
 ***************************************************************************
 *
 * pipeline stages
 *
 ***************************************************************************
 */
/*
 * Source generates a stream of random integers within [0, RNG_LIMIT)
 */
class PipeSourceLogic {
private:
    unsigned n = 0;
    std::mt19937 rng;

public:
    dff2::token_t svc(dff2::OneToOne &c)
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

typedef dff2::Source<dff2::OneToOne, //
        gam::private_ptr<int>, //
        PipeSourceLogic> PipeSource;

/*
 * Lowpass selects input integers below THRESHOLD
 */
class LowpassLogic {
public:
    dff2::token_t svc(gam::private_ptr<int> &in, dff2::OneToOne &c)
    {
        auto local_in = in.local();
        if (*local_in < THRESHOLD)
            c.emit(gam::private_ptr<int>(std::move(local_in)));
        return dff2::go_on;
    }

    void svc_init()
    {
    }

    void svc_end()
    {
    }
};

typedef dff2::Filter<dff2::OneToOne, dff2::OneToOne, //
        gam::private_ptr<int>, gam::private_ptr<int>, //
        LowpassLogic> Lowpass;

/*
 * Sqrt computes sqrt of each input integer and emits the results as floats
 */
class SqrtLogic {
private:
    float sum = 0;
    std::mt19937 rng;

public:
    dff2::token_t svc(gam::private_ptr<int> &in, dff2::OneToOne &c)
    {
        float res = std::sqrt(*(in.local()));
        sum += res;
        c.emit(gam::make_private<float>(res));
        return dff2::go_on;
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
            if (x < THRESHOLD)
                res += sqrt(x);
        }
        assert(res == sum);
    }
};

typedef dff2::Filter<dff2::OneToOne, dff2::OneToOne, //
        gam::private_ptr<int>, gam::private_ptr<float>, //
        SqrtLogic> Sqrt;

/*
 * Sink prints its input stream
 */
class PipeSinkLogic {
public:
    void svc(gam::private_ptr<float> &in)
    {
        std::cout << *(in.local()) << std::endl;
    }

    void svc_init()
    {
    }

    void svc_end()
    {
    }
};

typedef dff2::Sink<dff2::OneToOne, //
        gam::private_ptr<float>, //
        PipeSinkLogic> PipeSink;

/*
 *******************************************************************************
 *
 * main
 *
 *******************************************************************************
 */
int main(int argc, char * argv[])
{
    dff2::OneToOne comm1, comm2, comm3;

    dff2::add(PipeSource(comm1));
    dff2::add(Lowpass(comm1, comm2));
    dff2::add(Sqrt(comm2, comm3));
    dff2::add(PipeSink(comm3));

    dff2::run();

    return 0;
}
