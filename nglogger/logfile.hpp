/*
 * Copyright (C) ROSSAKER AB, Sweden
 * License: TBD
 */


#pragma once

#include <iostream>
#include <string>
#include <vector>

namespace nglogger
{
using namespace std;

// Todo:    Move to dynamic (constructor) property
//          but this will require some extra code
#define NGLOGGER_PAYLOAD_SIZE (128)

enum class loggedrowtypebase
{
    TEXT = 0,
    BINARY = 1
};

enum class loggedrowtypeuserreserved
{
    XML             = 0,
    JSON            = 1,
    MESSAGEPACK     = 2,
    BSON            = 3
};

struct loggedrowtype
{
    unsigned /*loggedrowtypebase*/ type: 1;
    unsigned usertype:15;
};

enum class loggededrowstatusbase
{
    INFO        = 0,
    WARNING     = 1,
    ERROR       = 2,
};

struct loggedrowstatus
{
    unsigned /*loggededrowstatusbase*/ status: 2;
    unsigned userstatus: 30;
};


struct loggedrowheader
{
    uint64_t checksum = 0;
    uint32_t rowid = 0;
    uint32_t parent_rowid = 0;
    uint32_t length = 0;
    uint16_t parts = 1;
    uint64_t when = 0;
    loggedrowtype type = {(unsigned)loggedrowtypebase::TEXT,0};
    loggedrowstatus status = {(unsigned)loggededrowstatusbase::INFO,0};

    loggedrowheader()
    {
//        type= {loggedrowtypebase::TEXT,0};
    }

    static constexpr size_t size()
    {
        return sizeof(loggedrowheader);
    }
};

struct loggedrow
{
    loggedrowheader header;
    byte payload[NGLOGGER_PAYLOAD_SIZE];

    static constexpr size_t size()
    {
        return sizeof(loggedrow);
    }
};

uint64_t calculate_hash(const loggedrow &row);

struct logfile
{
    char magic[12];
    char identifier[255];
    uint32_t payloadsize;
    uint16_t pages = 1024;
    uint16_t currentrowindex = 0;

    uint32_t get_max_items( size_t pagesize )
    {
        return (pagesize*pages - headersize()) / loggedrow::size();
    }

    static size_t headersize()
    {
        return  sizeof(magic) +
                sizeof(identifier) +
                sizeof(payloadsize) +
                sizeof(pages) +
                sizeof(currentrowindex);
    }

    size_t size(size_t pagesize)
    {
        return  pages*pagesize;
    }
};

}
