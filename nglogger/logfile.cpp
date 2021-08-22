
#include "logfile.hpp"
namespace nglogger
{

// Todo: Replace
uint64_t calculate_hash(const loggedrow &row)
{
    const uint8_t *bytes = reinterpret_cast<const uint8_t*> (&row);
    uint64_t hash = 0ull;
    for(size_t i = sizeof(uint64_t) /* ignore checksum */; i < loggedrow::size(); i++)
    {
        hash = hash*31 ^ bytes[i];
    }
    return hash;
}

}
