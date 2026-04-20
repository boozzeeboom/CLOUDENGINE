#define __gl_h_
#include <core/logger.h>
#include <core/engine.h>

int main(int argc, char* argv[]) {
    // Logger::Init() should be called FIRST
    Logger::Init();
    CE_LOG_INFO("========================================");
    CE_LOG_INFO("CLOUDENGINE v0.4.0 - Basic Networking");
    CE_LOG_INFO("========================================");

    // Parse command line arguments
    Core::AppMode mode = Core::Engine::parseArgs(argc, argv);

    switch (mode) {
        case Core::AppMode::Host:
            CE_LOG_INFO("Mode: HOST (server on port 12345)");
            break;
        case Core::AppMode::Client:
            CE_LOG_INFO("Mode: CLIENT (connecting to localhost:12345)");
            break;
        default:
            CE_LOG_INFO("Mode: SINGLEPLAYER");
    }

    CE_LOG_INFO("main() - Creating Core::Engine");
    Core::Engine engine(mode);

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
