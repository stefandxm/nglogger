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
    size_t max_items = 1024*1024;
    std::string fulllog = "";
    std::mutex entriesmutex;

    std::unique_ptr<nglogger::logfilemmap> ngfile;
    std::unique_ptr<nglogger::logsplitter> ngsplitted;

    auto screen = ScreenInteractive::Fullscreen();
    //auto screen = ScreenInteractive(0, 0, 2, false);

    int selectedlogentry = 0;
    std::vector<std::string> entries;
    std::vector<loggeditem> entries_full;    
    std::vector<std::string> tab_entries;
    std::string path = "/var/xprojector/logs";
    for (const auto & entry : fs::directory_iterator(path))
    {
       if(entry.is_directory())
           continue;

      tab_entries.push_back( entry.path().filename());
    }    

    std::sort(tab_entries.begin(), tab_entries.end());

    MenuOption optionlogs;
    int selectedlog=0;
    bool logchanged =false;

    optionlogs.on_change= [&]{
        {            
            std::unique_lock<std::mutex> lockguard(entriesmutex);

            selectedlogentry=0;
            entries.clear();
            entries_full.clear();


            if(selectedlogentry<0 || selectedlogentry>=tab_entries.size())
            {
                if(ngsplitted != nullptr)
                    ngsplitted = nullptr;
                if(ngfile != nullptr)
                    ngfile = nullptr;
                return;
            }

            std::string filename= tab_entries[selectedlog];
            std::string fullpath = path + "/" + filename;
            if(ngsplitted != nullptr)
                ngsplitted = nullptr;

            ngfile = std::make_unique<nglogger::logfilemmap> ( fullpath );
            ngsplitted = std::make_unique<nglogger::logsplitter> (*ngfile);
            logchanged = true;
        }
    };


    //for( int i = 0; i < 100; i++)
//      entries.push_back("entry extra " + std::to_string(i));
    MenuOption optionlog;
    optionlog.on_change = [&]{        
//        std::string newlog = "";
//        for(int i = 0; i < 10; i++)
//        {
//            newlog += std::to_string( rand() );
//        }
//        newlog += " ";
//        newlog += newlog;
//        fulllog = newlog;

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

        //screen.PostEvent(Event::Custom);
    };


    auto menulog =  Menu(&entries, &selectedlogentry, &optionlog);
    auto menulogrender = Renderer( menulog,
        [&] {
                return menulog->Render() | frame | size(HEIGHT,LESS_THAN, 50); //|border;
            }
    );
    auto menulogs = Menu(&tab_entries, &selectedlog, &optionlogs);


    int left_size = 20;
    //int right_size = 20;
    int bottom_size = 10;
    auto container = menulogrender;
    container = ResizableSplitLeft(menulogs , container, &left_size);
    //container = ResizableSplitRight(menulog, container, &right_size);
    //container = ResizableSplitBottom(text("bottom")|center, container, &bottom_size);

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
        while (refresh_ui_continue) {
            using namespace std::chrono_literals;
            if(!foundlastime)
            {
                std::this_thread::sleep_for(1s);
            }
            else
            {
                std::this_thread::sleep_for(50ms);
            }

//            screen.PostEvent(Event::Custom);
//            continue;
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
//                if(true && ngsplitted->read_row(item.header, item.payload, item.checksumok))
//                {
//                    item.header.when = 0;
//                    item.payload ="dummy";
//                    entries.push_back( get_string_from_when(item.header.when) + ": " + item.payload);
//                    entries_full.push_back(item);
//                }
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
                //entries.push_back( std::to_string(rand()) + " " +  std::to_string(lastentryselected)+  " "  + std::to_string(selectedlogentry)  + " / " + std::to_string(entries.size()) );
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

#ifdef oldtest
int main(int argc, const char* argv[]) {

    auto screen = ScreenInteractive::Fullscreen();


    int tab_index = 0;
    std::vector<std::string> tab_entries;

    std::string path = "/var/xprojector/logs";
    for (const auto & entry : fs::directory_iterator(path))
    {
       if(entry.is_directory())
           continue;

      tab_entries.push_back( entry.path().filename());
    }

    MenuOption optionlogs;

    int selectedlog;

    std::vector<std::string> entries = {
       "entry 1",
       "entry 2",
       "entry 3",
    };
    for( int i = 0; i < 100; i++)
      entries.push_back("entry extra " + std::to_string(i));
    MenuOption optionlog;



    int selectedlogentry = 0;

    auto menulog = Menu(&entries, &selectedlogentry, &optionlog);

    auto menulogs = Menu(&tab_entries, &selectedlog, &optionlogs);

    //  auto tab_selection = Toggle(&tab_entries, &tab_index);
    //  /*auto tab_content = Container::Tab(
    //      {
    //        menu
    //      },
    //      &tab_index);*/

    auto main_container = Container::Horizontal({
    menulogs ,
    menulog
    });

    auto main_renderer = Renderer(main_container, [&] {
    return vbox({
        //hbox({
            text("NGLOGGER") | bold | hcenter,
        //}),
        separator(),
        hbox({
            /*menulogs->Render() | yframe | yflex_shrink,
            separator(),
            menulog->Render() | yframe | flex*/
            main_container->Render()  | yframe
        }) |  yflex,
        filler(),
        separator(),
        hbox({
             text("full message here") | hcenter,
        }),

    });
    });

    while(true)
    {
      screen.Loop(main_renderer);
    }

    return 0;
}

#endif
