#include "logsplitter.hpp"

char const* version()
{
   return "Alfa baby, alfa";
}

#include <boost/python.hpp>
//#include <boost/shared_ptr.hpp>

using namespace nglogger;
using namespace std;

struct python_splitted_post
{
    loggedrowheader header;
    string payload;
    bool checksumok;
};

class python_splitted_reader
{
    logfilemmap iofile;
    logsplitter io;
public:

    python_splitted_reader(const string &filename)
        : iofile(filename), io(iofile)
    {
    }

    shared_ptr<python_splitted_post> read()
    {
        auto rval = make_shared<python_splitted_post>();
        if(io.read_row(rval->header,rval->payload,rval->checksumok))
        {
            return rval;
        }
        return nullptr;
    }
};

shared_ptr<python_splitted_reader> open_splitted_reader(const string &filename)
{
    return make_shared<python_splitted_reader>(filename);
}


BOOST_PYTHON_MODULE(nglogger)
{
    using namespace boost::python;
    def("version", version);

    class_<loggedrowheader>("loggedrowheader")
        .def_readwrite("checksum", &loggedrowheader::checksum)
        .def_readwrite("rowid", &loggedrowheader::rowid)
        .def_readwrite("parent_rowid", &loggedrowheader::parent_rowid)
        .def_readwrite("parts", &loggedrowheader::parts)
        .def_readwrite("when", &loggedrowheader::when)
        .def_readwrite("type", &loggedrowheader::type)
        .def_readwrite("status", &loggedrowheader::status)
    ;
    class_<python_splitted_post>("row")
        .def_readwrite("header", &python_splitted_post::header)
        .def_readwrite("payload", &python_splitted_post::payload)
        .def_readwrite("checksumok", &python_splitted_post::checksumok)
     ;
    class_<python_splitted_reader, boost::noncopyable>("reader", init<const string&>())
        .def( init<const string&>() )
        .def("read", &python_splitted_reader::read)
    ;
    def("open", open_splitted_reader);

    register_ptr_to_python< shared_ptr<python_splitted_post> >();
    register_ptr_to_python< shared_ptr<python_splitted_reader> >();

//    class_<logfilemmap, boost::noncopyable>("logfilemmap", init<const string&>())
//        .def( init<const string&>() )
//    ;

//    class_<logsplitter>("logsplitter", init<logsplitter&>())
//        .def( init<logsplitter&>() )
//        .def( "read", &logsplitter::read_row )
//    ;
}

