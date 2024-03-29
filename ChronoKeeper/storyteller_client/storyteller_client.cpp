#include <iostream>
#include <ctime>
#include <chrono>

#include <thallium/serialization/stl/string.hpp>
#include <thallium.hpp>

namespace tl = thallium;

int main(int argc, char** argv) {
    if(argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <address> <provider_id>" << std::endl;
        exit(0);
    }
    tl::engine myEngine("ofi+sockets", THALLIUM_CLIENT_MODE);
    tl::remote_procedure record_event  = myEngine.define("record_event");
    tl::endpoint server = myEngine.lookup(argv[1]);
    uint16_t provider_id = atoi(argv[2]);
    tl::provider_handle ph(server, provider_id);

    try   // timed request throws timeout exception 
    {
       std::string log_record("story_line_1");
       uint64_t log_time= std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
       int ret = record_event.on(ph).timed(std::chrono::milliseconds(3),
		    static_cast<uint64_t>(123), static_cast<uint64_t>(321), log_time, log_record);
       std::cout << "(record_event) response " << ret << std::endl;
    }
    catch(thallium::timeout const&) 
    { std::cout << "(record_event timed (3mls) :timeout" << std::endl; }


    return 0;
}
