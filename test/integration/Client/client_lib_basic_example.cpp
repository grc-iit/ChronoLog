#include <chronolog_client.h>
#include <common.h>

int main() {
    // Configure the client connection
    chronolog::ClientPortalServiceConf portalConf("ofi+sockets", "127.0.0.1", 5555, 55);

    // Create a ChronoLog client
    chronolog::Client client(portalConf);

    // Connect to ChronoVisor
    client.Connect();

    // Create a chronicle
    std::string chronicle_name = "MyChronicle";
    std::map<std::string, std::string> chronicle_attrs;
    int flags = 0;
    client.CreateChronicle(chronicle_name, chronicle_attrs, flags);

    // Acquire a story
    std::string story_name = "MyStory";
    std::map<std::string, std::string> story_attrs;
    auto acquire_result = client.AcquireStory(chronicle_name, story_name, story_attrs, flags);
    auto story_handle = acquire_result.second;

    // Log a few events to the story
    story_handle->log_event("Event 1");
    story_handle->log_event("Event 2");
    story_handle->log_event("Event 3");

    // TODO: Replay the story to read logged events

    // Release the story
    client.ReleaseStory(chronicle_name, story_name);

    // Destroy the story
    client.DestroyStory(chronicle_name, story_name);

    // Destroy the chronicle
    client.DestroyChronicle(chronicle_name);

    // Disconnect from ChronoVisor
    client.Disconnect();

    return 0;
}