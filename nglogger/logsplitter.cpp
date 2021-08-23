#include "logsplitter.hpp"


namespace nglogger
{

logsplitter::logsplitter(logfilemmap &logfile)
 : io(logfile)
{
}

void logsplitter::write_row(loggedrowheader &header, const string &payload)
{
    if(payload.size() <= NGLOGGER_PAYLOAD_SIZE-1)
    {
        loggedrow lrow;                
        memcpy(&lrow.header, &header, loggedrowheader::size());
        memcpy(&lrow.payload, payload.c_str(), payload.size());        
        lrow.payload[payload.size()] = 0;        
        io.write_row(lrow);

        return;
    }

    loggedrow itrow;
    memcpy(&itrow.header, &header, loggedrowheader::size());
    memcpy(&itrow.payload, payload.c_str(), NGLOGGER_PAYLOAD_SIZE-1);
    itrow.payload[NGLOGGER_PAYLOAD_SIZE-1] = 0;
    itrow.header.parts = ceil( (double)payload.size() / (double)NGLOGGER_PAYLOAD_SIZE);
    io.write_row(itrow, true, true);

    size_t bytes_written = NGLOGGER_PAYLOAD_SIZE-1;
    while(bytes_written < payload.size())
    {
        uint32_t bytes_to_write = (uint32_t) min( (size_t) (NGLOGGER_PAYLOAD_SIZE-1),
                                                  (size_t)(payload.size() - bytes_written));
        itrow.header.parent_rowid = itrow.header.rowid;
        memcpy(&itrow.payload, payload.c_str()+bytes_written, bytes_to_write);
        itrow.payload[bytes_to_write] = 0;
        io.write_row(itrow);
        bytes_written += bytes_to_write;
    }
}

bool logsplitter::read_row(loggedrowheader &header,
                           string &payload,
                           bool &checksumok,
                           uint32_t waitms,
                           uint32_t nr_retries)
{
    loggedrow row;
    if(!io.read_nextrow(row))
        return false;



    uint64_t calcedhash = calculate_hash(row);
    checksumok = (calcedhash == row.header.checksum);

    memcpy(&header, &row.header, loggedrowheader::size());
    size_t len = strnlen(row.payload, NGLOGGER_PAYLOAD_SIZE);
    if(len == NGLOGGER_PAYLOAD_SIZE)
    {
        payload = "";
    }
    else
    {
        payload = string(row.payload, len);
    }


    if(row.header.parent_rowid == 0 && row.header.parts == 1)
    {        
        return true;
    }

    if(row.header.parent_rowid != 0 && row.header.parts != 1)
    {
        checksumok = false;
        return true;
    }

    for(int i = 1; i < header.parts; i++ )
    {
        while(!io.read_nextrow(row) && (nr_retries--) > 1)
        {
            std::this_thread::sleep_for( std::chrono::milliseconds(waitms) );
        }
        if(nr_retries == 0)
        {
            checksumok = false;
            return true;
        }

        if(row.header.checksum != calculate_hash(row))
            checksumok = false;

        payload += string(row.payload, strnlen(row.payload, NGLOGGER_PAYLOAD_SIZE));
    }

    return true;
}

}
