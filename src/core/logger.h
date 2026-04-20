#pragma once
#include <spdlog/spdlog.h>
#include <memory>
#include <vector>

/// @brief Central logging system for CLOUDENGINE
/// @details Provides per-subsystem loggers with console and file sinks
class Logger {
public:
    /// @brief Initialize all subsystem loggers
    static void Init();

    /// @brief Shutdown logging system
    static void Shutdown();

    /// @brief Engine logger for core engine messages
    static std::shared_ptr<spdlog::logger>& Engine() { return s_Engine; }

    /// @brief ECS logger for entity/component system messages
    static std::shared_ptr<spdlog::logger>& ECS() { return s_ECS; }

    /// @brief Render logger for rendering system messages
    static std::shared_ptr<spdlog::logger>& Render() { return s_Render; }

    /// @brief Network logger for network system messages
    static std::shared_ptr<spdlog::logger>& Network() { return s_Network; }

    /// @brief Physics logger for physics system messages
    static std::shared_ptr<spdlog::logger>& Physics() { return s_Physics; }

private:
    static inline std::shared_ptr<spdlog::logger> s_Engine;
    static inline std::shared_ptr<spdlog::logger> s_ECS;
    static inline std::shared_ptr<spdlog::logger> s_Render;
    static inline std::shared_ptr<spdlog::logger> s_Network;
    static inline std::shared_ptr<spdlog::logger> s_Physics;
};

// Convenience macros for each subsystem
#define CE_LOG_TRACE(...)   Logger::Engine()->trace(__VA_ARGS__)
#define CE_LOG_DEBUG(...)   Logger::Engine()->debug(__VA_ARGS__)
#define CE_LOG_INFO(...)    Logger::Engine()->info(__VA_ARGS__)
#define CE_LOG_WARN(...)    Logger::Engine()->warn(__VA_ARGS__)
#define CE_LOG_ERROR(...)   Logger::Engine()->error(__VA_ARGS__)
#define CE_LOG_CRITICAL(...) Logger::Engine()->critical(__VA_ARGS__)

#define ECS_LOG_TRACE(...)  Logger::ECS()->trace(__VA_ARGS__)
#define ECS_LOG_DEBUG(...)   Logger::ECS()->debug(__VA_ARGS__)
#define ECS_LOG_INFO(...)   Logger::ECS()->info(__VA_ARGS__)
#define ECS_LOG_WARN(...)   Logger::ECS()->warn(__VA_ARGS__)
#define ECS_LOG_ERROR(...)  Logger::ECS()->error(__VA_ARGS__)

#define RENDER_LOG_TRACE(...)  Logger::Render()->trace(__VA_ARGS__)
#define RENDER_LOG_DEBUG(...)   Logger::Render()->debug(__VA_ARGS__)
#define RENDER_LOG_INFO(...)   Logger::Render()->info(__VA_ARGS__)
#define RENDER_LOG_WARN(...)   Logger::Render()->warn(__VA_ARGS__)
#define RENDER_LOG_ERROR(...)  Logger::Render()->error(__VA_ARGS__)

#define NETWORK_LOG_INFO(...)  Logger::Network()->info(__VA_ARGS__)
#define NETWORK_LOG_WARN(...)  Logger::Network()->warn(__VA_ARGS__)
#define NETWORK_LOG_ERROR(...) Logger::Network()->error(__VA_ARGS__)

#define PHYSICS_LOG_INFO(...)  Logger::Physics()->info(__VA_ARGS__)
#define PHYSICS_LOG_WARN(...)  Logger::Physics()->warn(__VA_ARGS__)
#define PHYSICS_LOG_ERROR(...) Logger::Physics()->error(__VA_ARGS__)
