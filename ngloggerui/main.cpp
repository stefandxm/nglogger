#include <array>       // for array
#include <chrono>      // for operator""s, chrono_literals
#include <cmath>       // for sin
#include <functional>  // for ref, reference_wrapper, function
#include <memory>      // for allocator, shared_ptr, __shared_ptr_access
#include <string>  // for string, basic_string, operator+, char_traits, to_string
#include <thread>   // for sleep_for, thread
#include <utility>  // for move
#include <vector>   // for vector

#include <string>
#include <iostream>
#include <filesystem>
#include <time.h>

namespace fs = std::filesystem;


#include "ftxui/component/captured_mouse.hpp"  // for ftxui
#include "ftxui/component/component.hpp"  // for Checkbox, Renderer, Horizontal, Vertical, Menu, Radiobox, Tab, Toggle
#include "ftxui/component/component_base.hpp"     // for ComponentBase
#include "ftxui/component/component_options.hpp"  // for InputOption
#include "ftxui/component/event.hpp"              // for Event, Event::Custom
#include "ftxui/component/screen_interactive.hpp"  // for Component, ScreenInteractive
#include "ftxui/dom/elements.hpp"  // for operator|, color, bgcolor, filler, Element, size, vbox, flex, hbox, graph, separator, EQUAL, WIDTH, hcenter, bold, border, window, HEIGHT, Elements, hflow, flex_grow, frame, gauge, LESS_THAN, spinner, dim, GREATER_THAN
#include "ftxui/screen/color.hpp"  // for Color, Color::BlueLight, Color::RedLight, Color::Black, Color::Blue, Color::Cyan, Color::CyanLight, Color::GrayDark, Color::GrayLight, Color::Green, Color::GreenLight, Color::Magenta, Color::MagentaLight, Color::Red, Color::White, Color::Yellow, Color::YellowLight, Color::Default

#include "../nglogger/nglogger.hpp"

using namespace ftxui;
using namespace std;

struct loggeditem
{
    nglogger::loggedrowheader header;
    std::string payload = "";
    bool checksumok = false;
};

std::string get_string_from_date(std::chrono::steady_clock::time_point time)
{
    std::time_t time_c = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()
                                                 + (time - std::chrono::steady_clock::now()));
    std::tm time_tm = *std::localtime(&time_c);
    char buff[255];
    strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", &time_tm);
    return std::string(buff);
}

std::string get_string_from_when(uint64_t when)
{
    std::chrono::milliseconds dur(when);
    std::chrono::time_point<std::chrono::steady_clock> dt(dur);
    return get_string_from_date(dt);
}


int inner_main(int /* argc*/, const char*/* argv*/[]) {
    size_t max_items = 1024;
    std::string fulllog = "";

    std::mutex entriesmutex;
    std::unique_ptr<nglogger::logfilemmap> ngfile;
    std::unique_ptr<nglogger::logsplitter> ngsplitted;


    int selectedlogentry = 0;
    std::vector<std::string> entries;
    std::vector<loggeditem> entries_full;    
    std::vector<std::string> tab_entries;
    std::string path = "/var/xprojector/logs";
    cout << "Retrieving log files" << endl;
    for (const auto & entry : fs::directory_iterator(path))
    {
       if(entry.is_directory())
           continue;

        cout << "Adding " << entry.path().filename() << endl;
        tab_entries.push_back( entry.path().filename());
    }    
    if(tab_entries.size() == 0ull)
    {
        cout << "No log files found in " << path << endl;
        return 1;
    }
    std::sort(tab_entries.begin(), tab_entries.end());

    auto screen = ScreenInteractive::Fullscreen();

    MenuOption optionlogs;
    int selectedlog=0;
    volatile bool logchanged = false;
    volatile bool logentrychanged = false;

    optionlogs.on_change= [&]{
        {            
            std::unique_lock<std::mutex> lockguard(entriesmutex);

            selectedlogentry=0;
            entries.clear();
            entries_full.clear();
            if(ngsplitted != nullptr)
                ngsplitted = nullptr;
            if(ngfile != nullptr)
                ngfile = nullptr;

            std::string filename= tab_entries[selectedlog];
            std::string fullpath = path + "/" + filename;
            if(ngsplitted != nullptr)
                ngsplitted = nullptr;

            ngfile = std::make_unique<nglogger::logfilemmap> ( fullpath );
            ngsplitted = std::make_unique<nglogger::logsplitter> (*ngfile);
            logchanged = true;
        }
    };

    MenuOption optionlog;
    optionlog.on_change = [&]{        
        {
            std::unique_lock<std::mutex> lockguard(entriesmutex);
            if(selectedlogentry < 0 || selectedlogentry >= entries_full.size())
            {
                fulllog="";
                return;
            }
            std::string newlog = entries_full[ selectedlogentry ].payload;
            fulllog = newlog;
        }
    };


    auto menulog =  Menu(&entries, &selectedlogentry, &optionlog);
    auto menulogrender = Renderer( menulog,
        [&] {
                return menulog->Render() | frame | size(HEIGHT,LESS_THAN, 50); //|border;
            }
    );
    auto menulogs = Menu(&tab_entries, &selectedlog, &optionlogs);


    int left_size = 20;
    int bottom_size = 10;
    auto container = menulogrender;
    container = ResizableSplitLeft(menulogs , container, &left_size);

    auto fullogrenderer = Renderer([&]{
        return hflow(paragraph(fulllog));
    }) ;

    container = ResizableSplitBottom(fullogrenderer, container, &bottom_size);

    auto renderer =
         Renderer(container, [&] { return container->Render() | border; });

    selectedlogentry = 0;//entries.size()-1;    

    bool refresh_ui_continue = true;
    std::thread refresh_ui([&] {
        bool foundlastime = false;
        while (refresh_ui_continue)
        {
            using namespace std::chrono_literals;
            if(!foundlastime)
            {
                std::this_thread::sleep_for(1s);
            }
            else
            {
                std::this_thread::sleep_for(50ms);
            }

            std::unique_lock<std::mutex> lockguard(entriesmutex);
            foundlastime = false;

            bool lastentryselected = false;
            if(selectedlogentry >= (int) entries.size() -1)
            {
                lastentryselected = true;
            }

            if(ngsplitted!=nullptr )
            {
                loggeditem item;
                foundlastime = true;
                for(int i = 0; i< 500 && foundlastime; i++)
                {
                    foundlastime = ngsplitted->read_row(item.header, item.payload, item.checksumok);
                    if(foundlastime)
                    {
                        if(item.checksumok)
                        {
                            entries.push_back( get_string_from_when(item.header.when) + ": " + item.payload.substr(0, 50));
                            entries_full.push_back(item);
                        }
                    }
                }

                if(logchanged )
                {
                    lastentryselected = true;
                    logchanged = false;
                }
            }

            if(entries.size() > max_items)
            {
                size_t removeitems=(entries.size()-max_items);
                entries.erase(entries.begin(), entries.begin()+ removeitems);
                entries_full.erase(entries_full.begin(), entries_full.begin()+ removeitems);

                if(selectedlogentry>(int)removeitems)
                    selectedlogentry -= removeitems;
                else
                    lastentryselected = true;
            }

            if(lastentryselected)
                selectedlogentry = entries.size()-1;

            screen.PostEvent(Event::Custom);
        }
    });

    screen.Loop(renderer);
    refresh_ui_continue = false;
    refresh_ui.join();


    return 0;
}

int main(int argc, const char* argv[]) {
    try
    {
        inner_main(argc, argv);
    }
    catch(std::exception &ex)
    {
        std::cerr << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "unknown exception" << std::endl;
    }
}

