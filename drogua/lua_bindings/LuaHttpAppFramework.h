#pragma once

#include <drogon/drogon.h>

#include <cstddef>
#include <cstdint>
#include <string>

class LuaHttpAppFramework {
    public:
        static LuaHttpAppFramework &instance();

        LuaHttpAppFramework &setThreadNum(size_t threadNum);

        LuaHttpAppFramework &addListener(const std::string &address, uint16_t port);

        LuaHttpAppFramework &setLogPath(const std::string &path);

        LuaHttpAppFramework &setLogLevel(const std::string &level);

        LuaHttpAppFramework &enableRunAsDaemon();
        
        LuaHttpAppFramework &loadJsonConfig(const std::string &file);

        void run();
};