#include "logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <filesystem>

namespace {
    std::shared_ptr<spdlog::logger> createLogger(const char* name, 
        const std::vector<spdlog::sink_ptr>& sinks) 
    {
        auto logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::trace);
        spdlog::register_logger(logger);
        return logger;
    }
}

void Logger::Init() {
    // Create sinks
    std::vector<spdlog::sink_ptr> sinks;

    // Console sink with color
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_pattern("[%H:%M:%S.%e] [%n] [%^%l%$] %v");
    sinks.push_back(consoleSink);

    // Ensure logs directory exists
    std::filesystem::create_directories("logs");

    // Rotating file sink (5MB max, 3 files)
    auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "logs/cloudengine.log",
        5 * 1024 * 1024,
        3
    );
    fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");
    sinks.push_back(fileSink);

    // Create per-subsystem loggers
    s_Engine   = createLogger("Engine",   sinks);
    s_ECS      = createLogger("ECS",      sinks);
    s_Render   = createLogger("Render",   sinks);
    s_Network  = createLogger("Network",  sinks);
    s_Physics  = createLogger("Physics",  sinks);

    // Flush on trace to ensure all logs appear immediately (DEBUG mode)
    spdlog::flush_on(spdlog::level::trace);

    CE_LOG_INFO("========================================");
    CE_LOG_INFO("CLOUDENGINE v0.2.0 starting...");
    CE_LOG_INFO("Log file: logs/cloudengine.log");
    CE_LOG_INFO("========================================");
    CE_LOG_INFO("Logger::Init() - complete");
}

void Logger::Shutdown() {
    CE_LOG_INFO("CLOUDENGINE shutting down");
    spdlog::shutdown();
}
