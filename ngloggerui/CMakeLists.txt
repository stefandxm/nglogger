# --- Fetch FTXUI --------------------------------------------------------------
include(FetchContent)
 
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG 4d77353852414b6bec5c4d7ffcc567caf168136e

#origin/main 4d77353852414b6bec5c4d7ffcc567caf168136e
)
 
FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
  FetchContent_Populate(ftxui)
  add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
 
# ------------------------------------------------------------------------------
 
# ------------------------------------------------------------------------------
 
#project(ngloggerui
#  LANGUAGES CXX
#  VERSION 1.0.0
#)
 
add_executable(ngloggerui main.cpp)
target_include_directories(ngloggerui PRIVATE src)
 
target_link_libraries(ngloggerui
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
  nglogger
)
