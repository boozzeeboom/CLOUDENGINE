#define __gl_h_
#include <core/logger.h>
#include <core/engine.h>
#include <GLFW/glfw3.h>

int main() {
    // Logger::Init() should be called FIRST
    Logger::Init();
    CE_LOG_INFO("========================================");
    CE_LOG_INFO("CLOUDENGINE v0.2.0 - Full Engine Test");
    CE_LOG_INFO("========================================");
    
    CE_LOG_INFO("main() - Creating Core::Engine");
    Core::Engine engine;
    
    CE_LOG_INFO("main() - Calling engine.init()");
    if (!engine.init()) {
        CE_LOG_ERROR("main() - engine.init() FAILED!");
        Logger::Shutdown();
        return 1;
    }
    
    CE_LOG_INFO("main() - engine.init() SUCCESS, starting main loop");
    CE_LOG_INFO("Press ESC in window to exit...");
    engine.run();
    
    CE_LOG_INFO("main() - engine.run() completed");
    Logger::Shutdown();
    return 0;
}
