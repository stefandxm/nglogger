# nglogger
Non Greedy logger

Designed to log rather fast and is probably not what you are looking for.

# How to use it

We encourage you to use it per-thread to make ngloggerui work better.

## init of thread

### Create this object

Header

extern thread_local nglogger::threadlogger xlog;

Source code 

thread_local nglogger::threadlogger xlog;
// this is where you want to point ng logger ui
xlog.set_basefilename ("your base path for logging here");
xlog.set_identifier ("your thread id here");


## Logging

Use xlog.info / xlog.warning / xlog.error

See threadlogger.hpp for more information

# ngloggerui

Execute ngloggerui with parameter of folder of logging
ngloggerui <path>

# Dependencies
Nothing for library.

UI Depends on;
* FTXUI https://github.com/ArthurSonzogni/FTXUI
* MessagePack C++ https://github.com/msgpack/msgpack-c 
* BOOST https://www.boost.org/
* JSON for Modern C++ https://github.com/nlohmann/json#binary-formats-cbor-and-messagepack
