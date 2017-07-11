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
 * @brief       implements DFF2 Logger class and shortcut macros
 * @author      Maurizio Drocco
 *
 * @ingroup     internals
 */
#ifndef GAM_INCLUDE_DFF2_LOGGER_HPP_
#define GAM_INCLUDE_DFF2_LOGGER_HPP_

#include <cassert>
#include <fstream>
#include <iostream>
#include <cstdarg>
#include <string>
#include <unistd.h>
using namespace std;

/*
 * shortcuts
 */
#ifdef DFF2_LOG
#define DFF2_LOGLN (dff2::Logger::getLogger()->log)
#define DFF2_LOGLN_OS(x) (dff2::Logger::getLogger()->out_stream() << x << std::endl)
#define DFF2_LOGGER_INIT (dff2::Logger::getLogger()->init)
#define DFF2_LOGGER_FINALIZE (dff2::Logger::getLogger()->finalize)
#else
#define DFF2_LOGLN(...) {}
#define DFF2_LOGLN_OS(...) {}
#define DFF2_LOGGER_INIT(...) {}
#define DFF2_LOGGER_FINALIZE(...) {}
#endif

namespace dff2 {

class Logger
{
public:
    static Logger *getLogger()
    {
        static Logger logger;
        return &logger;
    }

    void init(std::string fname, int id)
    {
        fname.append("/dff2.").append(to_string(id)).append(".log");
        m_Logfile.open(fname, ios::out);
        assert(m_Logfile);
        //print header message
        log("I am DFF2 node %d", id);
    }
    void finalize(int id)
    {
        //print footer message
        log("stop logging DFF2 node %d", id);
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
