#include <iostream>
#include <margo.h>
#include <thallium.hpp>
#include <thallium/serialization/stl/string.hpp>

namespace tl = thallium;


#include "KeeperIdCard.h"
#include "KeeperRecordingService.h"

// uncomment to test this class as standalone process
int main(int argc, char** argv) {

    uint16_t provider_id = 22;

    margo_instance_id margo_id=margo_init("ofi+sockets://localhost:5555",MARGO_SERVER_MODE, 1, 0);

    if(MARGO_INSTANCE_NULL == margo_id)
    {
      std::cout<<"FAiled to initialise margo_instance"<<std::endl;
      return 1;
    }
    std::cout<<"margo_instance initialized"<<std::endl;

   tl::engine keeperEngine(margo_id);
 
    std::cout << "Starting KeeperRecordingService  at address " << keeperEngine.self()
        << " with provider id " << provider_id << std::endl;

    chronolog::KeeperRecordingService keeperRecordingService(keeperEngine, provider_id);

    //tl::engine myEngine("ofi+sockets", THALLIUM_CLIENT_MODE);
    tl::remote_procedure handle_stats_msg = keeperEngine.define("handle_stats_msg").disable_response();
    tl::remote_procedure register_keeper = keeperEngine.define("register_keeper");
    tl::endpoint server = keeperEngine.lookup(argv[1]);
    uint16_t registry_provider_id = atoi(argv[2]);
    tl::provider_handle ph(server, registry_provider_id);

    std::string name("Keeper:22");
    handle_stats_msg.on(ph)(name);

    chronolog::KeeperIdCard keeperIdCard(123, 5555, 22);
    register_keeper.on(ph)(keeperIdCard); 
    return 0;
}

