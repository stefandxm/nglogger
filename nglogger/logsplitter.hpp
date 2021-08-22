#pragma once
#include "logfilemmap.hpp"
#include <chrono>
#include <thread>

namespace nglogger
{

class logsplitter
{
private:
    logfilemmap &io;

public:
    logsplitter(logfilemmap &logfile);

    void write_row(loggedrowheader &header, const string &payload);
    bool read_row(loggedrowheader &header,
                  string &payload,
                  bool &checksumok,
                  uint32_t waitms = 50,
                  uint32_t nr_retries = 100);
};

}
