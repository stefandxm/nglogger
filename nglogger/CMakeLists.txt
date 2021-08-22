


add_library(nglogger
    nglogger.cpp
    nglogger.cpp
    logfilemmap.cpp
    logfile.cpp
    logsplitter.cpp
    threadlogger.cpp
    multithreadlogger.cpp
)

install (TARGETS nglogger DESTINATION lib)
install (DIRECTORY "${PROJECT_SOURCE_DIR}/nglogger" DESTINATION "include/nglogger" FILES_MATCHING PATTERN "*.hpp")
