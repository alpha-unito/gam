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
 * @file        Profiler.hpp
 * @brief       implements DFF2 Profiler class and shortcut macros
 * @author      Maurizio Drocco
 *
 * @ingroup     internals
 */
#ifndef GAM_INCLUDE_DFF2_PROFILER_HPP_
#define GAM_INCLUDE_DFF2_PROFILER_HPP_

#include <cassert>
#include <fstream>
#include <iostream>
#include <cstdarg>
#include <string>
#include <unistd.h>
using namespace std;

#include "utils.hpp"

/*
 * shortcuts
 */
#ifdef DFF2_PROFILE
#define DFF2_PROFLN (dff2::Profiler::getProfiler()->log)
#define DFF2_PROFLN_RAW (dff2::Profiler::getProfiler()->log_raw)
#define DFF2_PROFLN_OS(x) (dff2::Profiler::getProfiler()->out_stream() << x << std::endl)
#define DFF2_PROFILER_INIT (dff2::Profiler::getProfiler()->init)
#define DFF2_PROFILER_FINALIZE (dff2::Profiler::getProfiler()->finalize)
#define DFF2_PROFILER_TIMER(v) time_point_t v
#define DFF2_PROFILER_HRT(v) (v) = hires_timer_ull()
#define DFF2_PROFILER_SPAN(a,b) time_diff((a),(b)).count()
#define DFF2_PROFILER_DURATION(v) duration_t v(0)
#define DFF2_PROFILER_DURATION_ADD(v,a,b) (v) += time_diff((a),(b))
#define DFF2_PROFILER_COND(s) s
#define DFF2_PROFILER_BOOL(v,c) bool v = (c);
#define DFF2_PROFILER_BOOL_COND(v,s) if(v) {s}
#else
#define DFF2_PROFLN(...) {}
#define DFF2_PROFLN_RAW(...) {}
#define DFF2_PROFLN_OS(...) {}
#define DFF2_PROFILER_INIT(...) {}
#define DFF2_PROFILER_FINALIZE(...) {}
#define DFF2_PROFILER_TIMER(...) {}
#define DFF2_PROFILER_HRT(...) {}
#define DFF2_PROFILER_SPAN(...) {}
#define DFF2_PROFILER_DURATION(...) {}
#define DFF2_PROFILER_DURATION_ADD(...) {}
#define DFF2_PROFILER_COND(...) {}
#define DFF2_PROFILER_BOOL(...) {}
#define DFF2_PROFILER_BOOL_COND(...) {}
#endif

namespace dff2 {

class Profiler {
public:
    static Profiler *getProfiler()
    {
        static Profiler logger;
        return &logger;
    }

    void init(std::string fname, int id)
    {
        fname.append("/dff2.").append(to_string(id)).append(".prof");
        m_Logfile.open(fname, ios::out);
        assert(m_Logfile);
        //print header message
        log("I am DFF2 node %d", id);
    }
    void finalize(int id)
    {
        //print footer message
        log("stop profiling DFF2 node %d", id);
        m_Logfile.close();
    }

    /**
     *   Variable Length Logger function
     *   @param format string for the message to be logged.
     */
    void log(const char * format, ...)
    {
        //print timestamp
        m_Logfile << "[" << getTime() << "] ";

        //print message
        va_start(args, format);
        vsprintf(sMessage, format, args);
        m_Logfile << sMessage << std::endl;
        va_end(args);
    }

    /**
     *   Variable Length Logger function
     *   @param format string for the message to be logged.
     */
    void log_raw(const char * format, ...)
    {
        //print message
        va_start(args, format);
        vsprintf(sMessage, format, args);
        m_Logfile << sMessage << std::endl;
        va_end(args);
    }

    std::ostream &out_stream()
    {
        return m_Logfile << "[" << getTime() << "] ";
    }

private:
    ofstream m_Logfile;

    char sMessage[256];
    va_list args;

    std::string getTime()
    {
        time_t now = time(0);
        if (now != -1)
        {
            std::string time = ctime(&now);
            time.pop_back();
            return time;
        }
        return "-";
    }

};

} /* namespace gam */

#endif /* SKEDATO_LOGGER_HPP_ */
