#pragma once
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include <memory>

namespace gfx {
extern std::shared_ptr<spdlog::logger> g_core_logger;
void InitLogger();
inline std::shared_ptr<spdlog::logger> GetLogger() { return g_core_logger; }
} // namespace gfx

#define gfx_trace(...) SPDLOG_LOGGER_TRACE(::gfx::GetLogger(), __VA_ARGS__)
#define gfx_info(...) SPDLOG_LOGGER_INFO(::gfx::GetLogger(), __VA_ARGS__)
#define gfx_error(...) SPDLOG_LOGGER_ERROR(::gfx::GetLogger(), __VA_ARGS__)
#define gfx_warn(...) SPDLOG_LOGGER_WARN(::gfx::GetLogger(), __VA_ARGS__)
#define gfx_debug(...) SPDLOG_LOGGER_DEBUG(::gfx::GetLogger(), __VA_ARGS__)
#define gfx_critical(...) SPDLOG_LOGGER_CRITICAL(::gfx::GetLogger(), __VA_ARGS__)
