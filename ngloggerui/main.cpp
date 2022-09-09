#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <chrono>

#include <string>
#include <iostream>
#include <filesystem>
#include <time.h>

namespace fs = std::filesystem;


#include "ftxui/component/captured_mouse.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/component/event.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/color.hpp"

#include "../nglogger/nglogger.hpp"
#include <msgpack.hpp>
#include <json.hpp>

using namespace ftxui;
using namespace std;
using namespace std::chrono;

struct loggeditem
{
    nglogger::loggedrowheader header;
    string payload;
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

int inner_main(int argc, const char* argv[]) {
    size_t max_items = 1024;
    string fulllog = "";

    std::mutex entriesmutex;
    vector<unique_ptr<nglogger::logfilemmap>> ngfiles;
    vector<unique_ptr<nglogger::logsplitter>> ngsplitteds;


    int selectedlogentry = 0;
    int selectedlog=0;
    volatile bool logchanged = false;
    volatile bool logentrychanged = false;
    volatile bool paused = false;

    vector<string> log_entries_title;
    vector<string> log_filenames;
    vector<string> logfiles;

    vector< vector<string> > log_entries_titles;
    vector< vector<unique_ptr<loggeditem>>> log_entries_full;

    string basepath;

    if(argc < 2)
    {
        cerr << "Usage: nglogger <logfiles-path>" << endl;
        return 1;
    }
    basepath = argv[1];

    for (const auto & entry : fs::directory_iterator(basepath))
    {
       if(entry.is_directory())
           continue;

        log_filenames.push_back( entry.path().filename());
    }    
    if(log_filenames.empty())
    {
        return 1;
    }
    std::sort(log_filenames.begin(), log_filenames.end());

    for(const string &fn: log_filenames)
    {
        string filename = basepath + "/" + fn;
        ngfiles.emplace_back(make_unique<nglogger::logfilemmap>(filename));
        ngsplitteds.emplace_back(make_unique<nglogger::logsplitter>(*ngfiles.back()));
        logfiles.push_back(fn);
        log_entries_titles.emplace_back();
        log_entries_full.emplace_back();
    }

    auto screen = ScreenInteractive::Fullscreen();

    MenuOption optionlogs;

    optionlogs.on_change= [&]{
        {            
            //std::unique_lock<std::mutex> lockguard(entriesmutex);
            logchanged = true;
        }
    };

    MenuOption optionlog;
    optionlog.on_change = [&]{        
        {
            //std::unique_lock<std::mutex> lockguard(entriesmutex);
            logentrychanged = true;
        }
    };


    auto menulog =  Menu(&log_entries_title, &selectedlogentry, &optionlog);
    auto menulogrender = Renderer( menulog,
        [&] {
                return menulog->Render() | frame;
            }
    );
    auto menulogs = Menu(&logfiles, &selectedlog, &optionlogs);
    auto menulogsrenderer = Renderer( menulogs,
        [&]{
            if(paused)
            {
                return vbox(text("paused") |border, menulogs->Render());
            }
            return menulogs->Render();
        }
    );


    int left_size = 70;
    int bottom_size = 10;
    auto container = menulogrender;
    container = ResizableSplitLeft(menulogsrenderer , container, &left_size);

    auto fullogrenderer = Renderer([&]{
        return hflow(paragraph(fulllog));
    }) ;


    container = ResizableSplitBottom(fullogrenderer, container, &bottom_size);


    auto renderer = Renderer(container, [&] {
        return container->Render() | border;
    });



    selectedlogentry = 0;
    logchanged=true;
    logentrychanged=true;
    bool refresh_ui_continue = true;

    string errormsg = "";

    std::thread refresh_ui([&] {
        std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> lastupdate = steady_clock::now();
        bool foundlastime = false;
        bool refreshNeeded = false;
        while (refresh_ui_continue)
        {
            if(paused)
            {
                std::this_thread::sleep_for(100ms);
                continue;
            }

            using namespace std::chrono_literals;
            if(!foundlastime)
            {
                std::this_thread::sleep_for(200ms);
            }
            else
            {
                std::this_thread::sleep_for(50ms);
            }

            std::unique_lock<std::mutex> lockguard(entriesmutex);
            foundlastime = false;

            bool lastentryselected = true;

            if(logchanged)
            {
                refreshNeeded = true;
                selectedlogentry=0;
                log_entries_title.clear();

                logchanged = false;
                if(selectedlog <0 || selectedlog>= log_entries_titles.size())
                {
                    logchanged = false;
                    continue;
                }

                log_entries_title.clear();
                for(auto &t : log_entries_titles[selectedlog] )
                {
                    log_entries_title.push_back(t);
                }
            }
            else
            {
                if(selectedlogentry < (int) log_entries_title.size() -1)
                {
                    lastentryselected = false;
                }
            }

            foundlastime = false;
            for(size_t splitindex = 0; splitindex < ngsplitteds.size(); splitindex++)
            {
                auto &splitter = ngsplitteds[splitindex];

                for(int i = 0; i< max_items; i++)
                {
                    unique_ptr<loggeditem> item = make_unique<loggeditem>();
                    vector<byte> payload;
                    bool found = splitter->read_row(item->header, payload, item->checksumok,0);
                    if(!found)
                        break;

                    foundlastime = true;
                    if(item->checksumok)
                    {
                        if(item->header.type.type == (unsigned)nglogger::loggedrowtypebase::TEXT)
                        {
                            item->payload = string( (char*) payload.data(), payload.size() );
                        }
                        else
                        {
                            item->payload = "binary data";

                            if(item->header.type.usertype == (unsigned)nglogger::loggedrowtypeuserreserved::MESSAGEPACK)
                            {
                                string tmsgpack= string( (char*) payload.data(), payload.size() );

                                msgpack::object_handle oh =
                                        msgpack::unpack(tmsgpack.data(), tmsgpack.size());

                                stringstream str;
                                str << oh.get();
                                string sermsgpack = str.str();

                                try
                                {
                                    auto j3 = nlohmann::json::parse(sermsgpack);
                                    item->payload = j3.dump(1);
                                }
                                //catch(exception &ex)
                                //{
                                 //   item->payload = sermsgpack //string("EXCEPTION PARSING: ") + ex.what();
                                //}
                                catch(...)
                                {
                                    item->payload = sermsgpack;
                                }
                            }
                            else
                            {
                                item->payload="binary data";
                            }
                        }

                        string title = get_string_from_when(item->header.when) + ": " + item->payload.substr(0, 80);
                        if(selectedlog == splitindex)
                            log_entries_title.push_back( title );

                        logfiles[splitindex] = log_filenames[splitindex] +  " - " + item->payload.substr(0, 80);
                        log_entries_titles[splitindex].push_back(title);
                        log_entries_full[splitindex].push_back(move(item));
                    }
                }
            }
            if(foundlastime)
                refreshNeeded = true;

            if(log_entries_title.size() > max_items)
            {
                size_t removeitems=(log_entries_title.size()-max_items);
                log_entries_title.erase(log_entries_title.begin(), log_entries_title.begin()+ removeitems);


                if(selectedlogentry>(int)removeitems)
                    selectedlogentry -= removeitems;
                else
                    lastentryselected = true;
            }

            for(auto &v: log_entries_full)
            {
                if(v.size() < max_items)
                    continue;
                size_t removeitems=(v.size()-max_items);
                v.erase(v.begin(), v.begin() + removeitems);
            }
            for(auto &v: log_entries_titles)
            {
                if(v.size() < max_items)
                    continue;
                size_t removeitems=(v.size()-max_items);
                v.erase(v.begin(), v.begin() + removeitems);
            }

            if(lastentryselected)
                selectedlogentry = log_entries_title.size()-1;
            if(log_entries_full[selectedlog].size() > selectedlogentry)
            {
                auto &item = log_entries_full[selectedlog][selectedlogentry];
                fulllog = get_string_from_when(item->header.when) + " \n ";
                fulllog += item->payload;
            }
            else
            {
                fulllog="??";
            }
            if(refreshNeeded)
            {
                std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> now = steady_clock::now();
                if( duration_cast<milliseconds>(now-lastupdate).count()   > 500 )
                {
                    refreshNeeded = false;
                    screen.PostEvent(Event::Custom);
                }
            }
        }
    });

    renderer |=
        CatchEvent( [&](Event event) {
            if(!event.is_character())
            {
                return false;
            }

            if(event == Event::Character(' '))
            {
                paused = !paused;
                screen.PostEvent(Event::Custom);
                return true;
            }
            if(event == Event::Character('q'))
            {
                screen.ExitLoopClosure()();
                return true;
            }
            return false;
        });
    screen.Loop(renderer);
    refresh_ui_continue = false;
    refresh_ui.join();


    cerr << errormsg << endl;
    return 0;
}

int main(int argc, const char* argv[]) {
    try
    {
        return inner_main(argc, argv);
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

