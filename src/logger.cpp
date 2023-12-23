#include "logger.h"

namespace gfx {
std::shared_ptr<spdlog::logger> g_core_logger;

void InitLogger() {
    std::vector<spdlog::sink_ptr> core_logger_sinks;
    auto stdoutSink0 = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    core_logger_sinks.emplace_back(stdoutSink0);
    core_logger_sinks[0]->set_pattern("[%T] [%n] [\"%g\":%#] \n%^[%l]: %v%$ ");
    g_core_logger =
        std::make_shared<spdlog::logger>("CSGFX", std::begin(core_logger_sinks), std::end(core_logger_sinks));
    spdlog::register_logger(g_core_logger);
    g_core_logger->set_level(spdlog::level::trace);
    g_core_logger->flush_on(spdlog::level::trace);
}
} // namespace gfx
