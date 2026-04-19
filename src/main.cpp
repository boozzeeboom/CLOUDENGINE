#include <core/logging.h>
#include <core/engine.h>
#include <iostream>

int main(int argc, char* argv[]) {
    using namespace Core;
    
    // Init logging first
    Logger::init();
    LOG_INFO("=== CLOUDENGINE v0.1 ===");
    LOG_INFO("Project C: The Clouds - Minimal Engine");
    
    // Create and run engine
    Engine engine;
    if (!engine.init()) {
        LOG_ERROR("Engine failed to initialize!");
        return 1;
    }
    
    engine.run();
    
    LOG_INFO("Exiting normally");
    return 0;
}
