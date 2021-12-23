#include <string>
#include <iostream>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#endif

#include "rf62Xsdk.h"
#include "rf62Xtypes.h"

using namespace SDK::SCANNERS::RF62X;

void receive_profiles(std::shared_ptr<rf627smart> scanner);
int profile_count = 0;
int profile_lost = 0;
uint64_t min_time_diff = -1;
uint64_t max_time_diff = 0;

int main()
{
    std::cout << "#########################################"  << std::endl;
    std::cout << "#                                       #"  << std::endl;
    std::cout << "#      RF62X(v2.x.x) Profile Test       #"  << std::endl;
    std::cout << "#                                       #"  << std::endl;
    std::cout << "#########################################\n"<< std::endl;

    // Initialize sdk library
    sdk_init();

    // Print return rf62X sdk version
    std::cout << "SDK version: " << sdk_version()                << std::endl;
    std::cout << "========================================="     << std::endl;


    // Create value for scanners vector's type
    std::vector<std::shared_ptr<rf627smart>> list;
    // Search for rf627smart devices over network
    list = rf627smart::search();

    // Print count of discovered rf627smart in network by Service Protocol
    std::cout << "Was found\t: " << list.size()<< " RF627-Smart" << std::endl;
    std::cout << "========================================="     << std::endl;

    int index = -1;
    if (list.size() > 1)
    {
        std::cout << "Select scanner for test: " << std::endl;
        for (size_t i = 0; i < list.size(); i++)
            std::cout << i << ". Serial: "
                      << list[i]->get_info()->serial_number() << std::endl;
        std::cin >> index;
    }
    else if (list.size() == 1)
        index = 0;
    else
        return 0;

    std::shared_ptr<hello_info> info = list[index]->get_info();

    // Print information about the scanner to which the profile belongs.
    std::cout << "-----------------------------------------" << std::endl;
    std::cout << "Device information: "                      << std::endl;
    std::cout << "* Name  \t: "   << info->device_name()     << std::endl;
    std::cout << "* Serial\t: "   << info->serial_number()   << std::endl;
    std::cout << "* IP Addr\t: "  << info->ip_address()      << std::endl;

    // Establish connection to the RF627 device by Service Protocol.
    bool is_connected = list[index]->connect();

    if (is_connected)
    {
        std::thread receiver = std::thread(receive_profiles, list[index]);
#ifdef _WIN32
        SetThreadPriority(receiver.native_handle(), THREAD_PRIORITY_HIGHEST);
#endif

        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            std::cout << "FPS: " << profile_count
                      << "; Lost: " << profile_lost
                      << "; MinTimeDiff: " << min_time_diff
                      << "; MaxTimeDiff: "<< max_time_diff << std::endl;
            profile_lost = 0;
            profile_count = 0;
            min_time_diff = -1;
            max_time_diff = 0;
        }

        if (receiver.joinable())
            receiver.join();

        // Disconnect from scanner.
        list[index]->disconnect();
    }

    // Cleanup resources allocated with sdk_init()
    sdk_cleanup();
}

void receive_profiles(std::shared_ptr<rf627smart> scanner)
{
    // Get profile from scanner's data stream by Service Protocol.
    std::shared_ptr<profile2D> profile = nullptr;
    bool zero_points = true;
    bool realtime = false;

    int last_index = -1;
    uint64_t last_time = 0;
    while(true)
        if ((profile = scanner->get_profile2D(zero_points,realtime)))
        {
            if (last_index == -1)
            {
                last_time = profile->getHeader().system_time;
                last_index = profile->getHeader().measure_count;
            }
            else
            {
                profile_count++;
                if (profile->getHeader().measure_count - last_index > 1)
                    profile_lost += profile->getHeader().measure_count - last_index;
                else
                {
                    if (min_time_diff > profile->getHeader().system_time - last_time)
                        min_time_diff = profile->getHeader().system_time - last_time;
                    if (max_time_diff < profile->getHeader().system_time - last_time)
                        max_time_diff = profile->getHeader().system_time - last_time;
                }
                last_index = profile->getHeader().measure_count;
                last_time = profile->getHeader().system_time;
            }
        }else
        {
            std::cout << "Profile was not received!"                 <<std::endl;
            std::cout << "-----------------------------------------" <<std::endl;
        }
}

























