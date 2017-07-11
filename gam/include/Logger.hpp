/* ***************************************************************************
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License version 3 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************
 */

/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/**
 * @file        Logger.hpp
 * @brief       implements GAM Logger class and shortcut macros
 * @author      Maurizio Drocco
 *
 * @ingroup     internals
 */
#ifndef LOGGER_HPP_
#define LOGGER_HPP_

#include <fstream>
#include <iostream>
#include <cstdarg>
#include <string>
#include <unistd.h>
#include <mutex>

using namespace std;

/*
 * shortcuts
 */
#ifdef GAM_LOG
#define LOGLN (gam::Logger::getLogger()->log)
#define LOGLN_OS(x) {\
    gam::Logger::getLogger()->lock(); \
    gam::Logger::getLogger()->out_stream() << x << std::endl; \
    gam::Logger::getLogger()->unlock();}
#define LOGGER_INIT (gam::Logger::getLogger()->init)
#define LOGGER_FINALIZE (gam::Logger::getLogger()->finalize)
#else
#define LOGLN(...) {}
#define LOGLN_OS(...) {}
#define LOGGER_INIT(...) {}
#define LOGGER_FINALIZE(...) {}
#endif

namespace gam {

class Logger {
public:
    static Logger *getLogger()
    {
        static Logger logger;
        return &logger;
    }

    void init(std::string fname, int id)
    {
        fname.append("/gam.").append(to_string(id)).append(".log");
        m_Logfile.open(fname, ios::out);
        assert(m_Logfile);
        //print header message
        log("I am gam executor %d (pid=%d)", id, getpid());
    }
    void finalize(int id = 0)
    {
        //print footer message
        log("stop logging executor %d", id);
        m_Logfile.close();
    }

    /**
     *   Variable Length Logger function
     *   @param format string for the message to be logged.
     */
    void log(const char * format, ...)
    {
        //print message
        va_start(args, format);
        vsprintf(sMessage, format, args);

        lock();
        m_Logfile << "[" << time(0) << "] " << sMessage << std::endl;
        unlock();

        va_end(args);
    }

    std::ostream &out_stream()
    {
        return m_Logfile << "[" << time(0) << "] ";
    }

    void lock()
    {
        mtx.lock();
    }

    void unlock()
    {
        mtx.unlock();
    }

private:
    ofstream m_Logfile;
    std::mutex mtx;

    char sMessage[256];
    va_list args;

};

} /* namespace gam */

#endif /* SKEDATO_LOGGER_HPP_ */
