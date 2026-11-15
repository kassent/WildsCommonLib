#include "LogUtils.h"
#include "FileUtils.h"
#include <cstdarg>
#include <memory>
#include <filesystem>

#define USE_SPDLOG
#ifdef USE_SPDLOG
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#else
#include <reframework/API.hpp>
#endif

std::string pluginName;
#ifdef USE_SPDLOG
std::shared_ptr<spdlog::logger> globalLogger;
#endif

void initLogger(const char* a_pluginName) {
    pluginName = a_pluginName;
#ifdef USE_SPDLOG
    const auto logDirectory = FileUtils::GetExecutableDirectory() / "logs";
    std::filesystem::create_directories(logDirectory);
    const auto logFilePath = (logDirectory / (std::string(a_pluginName) + ".log")).generic_string();
    globalLogger = spdlog::basic_logger_mt("mhwi_logger", logFilePath, true);
    globalLogger->flush_on(spdlog::level::debug);
    globalLogger->set_level(spdlog::level::info);
    globalLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e][%l][%t] %v");
#endif
}

void _MESSAGE(const char* fmt, ...) {
    if (fmt == nullptr) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    auto len = vsnprintf(nullptr, 0, fmt, args);
    if (len <= 0) {
        return;
    }
    std::string buffer(len, '\0');
    vsnprintf(buffer.data(), static_cast<size_t>(len) + 1, fmt, args);
    va_end(args);

#ifdef USE_SPDLOG
    if (globalLogger) {
        globalLogger->info(buffer);
    }
#else
    try {
        auto &api = reframework::API::get();
        api->log_info("[%s] %s", pluginName.c_str(), buffer.c_str());
    } catch (const std::exception &e) {
        // ignore
    }
#endif
}
