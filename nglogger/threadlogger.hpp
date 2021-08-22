#pragma once

#include "logsplitter.hpp"
#include <string>

#include <memory>
#include <chrono>
namespace nglogger
{

using namespace std;
using namespace std::chrono;

class threadlogger
{
    string filename;
    string fileending;
    bool append_identifier;
    string identifier;
    uint16_t pages;
    unique_ptr<logfilemmap> iofile;
    unique_ptr<logsplitter> io;


public:
    threadlogger(const string &basefilename,
                 uint16_t pages = 100,
                 const string &fileending =".nglog",
                 bool append_identifier = true)
        : filename(basefilename),
          fileending(fileending),
          append_identifier(append_identifier),
          pages(pages)
    {
        // TODO where should we check pages so rowindexes cannot overflow...
    }

    threadlogger(const string &basefilename,
                 const string &identifier,
                 uint16_t pages = 100,
                 const string &fileending =".nglog",
                 bool append_identifier = true)
        : filename(basefilename),
          fileending(fileending),
          append_identifier(append_identifier),
          identifier(identifier),
          pages(pages)
    {
        // TODO where should we check pages so rowindexes cannot overflow...
    }

    void set_identifier(const string &identifier)
    {
        if(iofile != nullptr )
        {
            cerr << "nglogger: cannot set identifier once io file is open - call this function prio any logging!" << endl;
            throw runtime_error ("nglogger: cannot set identifier once io file is open - call this function prio any logging!");
        }
        this->identifier = identifier;
        cout << "initiating xlog identifier for " << this << " with " << this->identifier << endl;
    }

    void log(loggedrowtype type,
             loggedrowstatus status,
             const string &message)
    {
        loggedrowheader header;        
        header.status = status;
        header.type = type;
        header.when = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();

        if(io == nullptr)
        {
            string logfilename = filename;
            if(append_identifier)
            {
                logfilename += "-" + identifier;
            }
            logfilename += fileending;
            iofile = make_unique<logfilemmap>(logfilename, pages, identifier);
            io = make_unique<logsplitter>(*iofile);
        }

        io->write_row(header, message);
    }

    void info(const string &message)
    {
        loggedrowtype type = {(unsigned)loggedrowtypebase::TEXT,0};
        loggedrowstatus status = {(unsigned)loggededrowstatusbase::INFO,0};
        log( type,
             status,
             message);
    }
    void warning(const string &message)
    {
        loggedrowtype type = {(unsigned)loggedrowtypebase::TEXT,0};
        loggedrowstatus status = {(unsigned)loggededrowstatusbase::WARNING,0};
        log( type,
             status,
             message);
    }
    void error(const string &message)
    {
        loggedrowtype type = {(unsigned)loggedrowtypebase::TEXT,0};
        loggedrowstatus status = {(unsigned)loggededrowstatusbase::ERROR,0};
        log( type,
             status,
             message);
    }

};

}

