
#include "logfilemmap.hpp"

namespace nglogger
{

void logfilemmap::map(bool truncate, uint16_t requested_pages)
{
    pagesize = sysconf(_SC_PAGE_SIZE);

    off64_t size_bytes = lseek64(fd, 0, SEEK_END);
    lseek64(fd, 0, SEEK_SET);

    // Start off with one page
    // Read header
    // Re-map with proper nr pages
    uint16_t pages = size_bytes / pagesize;

    if(isproducer)
    {
        if(pages == 0 && !truncate)
        {
            throw runtime_error("empty file");
        }
        if(requested_pages != pages && truncate)
        {
            size_bytes = requested_pages * pagesize;
            pages = requested_pages;
        }

        if( truncate && ftruncate64(fd, size_bytes) == -1)
        {
            throw std::runtime_error("ftruncate failed for " + Filename);
        }
    }

    rawdata = mmap(nullptr, size_bytes, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    mappedsize = size_bytes;

    header = static_cast<logfile*> (rawdata);

    if(isproducer)
    {
        header->pages = pages;
        header->payloadsize = NGLOGGER_PAYLOAD_SIZE;
    }

    rows = static_cast< loggedrow*> (
                static_cast<void*>(
                    static_cast<uint8_t*>(rawdata)+logfile::headersize()
                    ));
    max_items = header->get_max_items(pagesize);
}

void logfilemmap::open_file()
{
    fd = open(Filename.c_str(), O_RDWR | O_CREAT, S_IRWXU|S_IRGRP);
    if(fd == -1)
    {
        cerr << "Could not open " << Filename << endl;
        cerr << "Error: " << strerror(errno) << endl;
        throw std::runtime_error(string("Could not open file; ") + strerror(errno) );
    }
}

void logfilemmap::close_file()
{
    munmap(rawdata, mappedsize);
    close(fd);
}

logfilemmap::~logfilemmap()
{
    close_file();
}

// consumer
logfilemmap::logfilemmap(const string &filename)
    : Filename(filename), isproducer(false)
{
    //readrows = 0;
    open_file();
    map(false);
}

// producer
logfilemmap::logfilemmap(const string &filename,
            uint16_t pages,
            const string &identifier)
    : Filename(filename), isproducer(true)
{
    //readrows = 0;

    open_file();
    map(true, pages);
    int identlen = max(min((int)254, (int)identifier.length()), 0);
    memcpy(header->identifier, identifier.c_str(), identlen);
    header->identifier[identlen] = '\0';
}


void logfilemmap::write_row( loggedrow &row,
                bool calculate_checksum,
                bool autorowid)
{
    if(!isproducer)
    {
        throw runtime_error("cannot write when consuming");
    }
    uint16_t n_rowid = header->currentrowindex+1;
    uint16_t n_rowindex = n_rowid % max_items;

    if(autorowid)
    {
        row.header.rowid = n_rowid;
    }

    if(calculate_checksum)
    {
        row.header.checksum = calculate_hash(row);
    }

    //cout << "row " << dec << row.header.rowid << " checksum: " << hex << row.header.checksum << endl;

    memcpy( &rows[n_rowindex], &row, loggedrow::size());
    header->currentrowindex = n_rowindex;
}

bool logfilemmap::read_nextrow( loggedrow &rval )
{
    if(header->currentrowindex < readrows)
        return false;

    memcpy( &rval, &rows[readrows], loggedrow::size());
    readrows = (readrows+1) % max_items;
    return true;
}

}
