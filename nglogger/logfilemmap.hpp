/*
 * Copyright (C) ROSSAKER AB, Sweden
 * License: TBD
 */

#pragma once

//#include <boost/thread.hpp>
//#include <boost/date_time/posix_time/posix_time.hpp>

#include <fcntl.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <unistd.h>
#include <iostream>
#include <errno.h>
#include <string.h>


#include <iostream>
#include <string>
#include <vector>
#include <cstdint>

#include "logfile.hpp"


namespace nglogger
{
using namespace std;


class logfilemmap
{
    string Filename;
    int fd = 0;
    size_t pagesize;
    uint16_t max_items;
    size_t mappedsize;
    uint16_t readrows = 1;

    void *rawdata;
    logfile *header;
    loggedrow *rows;
    bool isproducer;

    logfilemmap &operator=(logfilemmap &&other);


    void map(bool truncate, uint16_t requested_pages = 1);
    void open_file();
    void close_file();

public:

    ~logfilemmap();
    // consumer
    logfilemmap(const string &filename);
    // producer
    logfilemmap(const string &filename,
                uint16_t pages,
                const string &identifier);

    void write_row( loggedrow &row,
                    bool calculate_checksum = true,
                    bool autorowid = true);

    bool read_nextrow( loggedrow &rval );
};

}
