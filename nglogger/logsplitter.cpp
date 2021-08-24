#include "logsplitter.hpp"


namespace nglogger
{

logsplitter::logsplitter(logfilemmap &logfile)
 : io(logfile)
{
}

void logsplitter::write_row(loggedrowheader &header, const byte *payload, size_t len)
{
    if(len <= NGLOGGER_PAYLOAD_SIZE)
    {
        loggedrow lrow;
        header.length = len;
        memcpy(&lrow.header, &header, loggedrowheader::size());
        memcpy(&lrow.payload, payload, header.length);
        //lrow.payload[payload.size()] = 0;
        io.write_row(lrow);

        return;
    }

    loggedrow itrow;
    header.length = static_cast<size_t> (NGLOGGER_PAYLOAD_SIZE);
    memcpy(&itrow.header, &header, loggedrowheader::size());
    memcpy(&itrow.payload, payload, NGLOGGER_PAYLOAD_SIZE);
    itrow.header.parts = ceil( (double)len / (double)NGLOGGER_PAYLOAD_SIZE);
    io.write_row(itrow, true, true);

    size_t bytes_written = static_cast<size_t> (header.length);
    while(bytes_written < len)
    {
        itrow.header.length = (uint32_t) min( (size_t) (NGLOGGER_PAYLOAD_SIZE),
                                                  (size_t)(len - bytes_written));
        itrow.header.parent_rowid = itrow.header.rowid;
        memcpy(&itrow.payload, payload+bytes_written, itrow.header.length);
        io.write_row(itrow);
        bytes_written += itrow.header.length;
    }
}
void logsplitter::write_row(loggedrowheader &header, const string &payload)
{
    write_row(header, reinterpret_cast<const byte*> (payload.c_str()), payload.size());
}

bool logsplitter::read_row(loggedrowheader &header,
                           vector<byte> &payload,
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

    payload.clear();
    payload.reserve(row.header.length);
    copy(row.payload, row.payload + row.header.length, back_inserter(payload));

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

        payload.reserve(payload.size() + row.header.length);
        copy(row.payload, row.payload + row.header.length, back_inserter(payload));
    }

    row.header.length = static_cast<uint32_t> (payload.size());

    return true;
}

}
