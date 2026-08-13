#include "LuaHttpAppFramework.h"
#include <stdexcept>

LuaHttpAppFramework &LuaHttpAppFramework::instance() {
    static LuaHttpAppFramework instance;
    return instance;
}

LuaHttpAppFramework &LuaHttpAppFramework::setThreadNum(size_t threadNum) {
    drogon::app().setThreadNum(threadNum);
    return *this;
}

LuaHttpAppFramework &LuaHttpAppFramework::addListener(const std::string &address, uint16_t port) {
    drogon::app().addListener(address, port);
    return *this;
}

LuaHttpAppFramework &LuaHttpAppFramework::setLogPath(const std::string &path) {
    drogon::app().setLogPath(path);
    return *this;
}

LuaHttpAppFramework &LuaHttpAppFramework::setLogLevel(const std::string &level) {
    if (level == "TRACE") {
        drogon::app().setLogLevel(trantor::Logger::kTrace);
    }
    else if (level == "DEBUG") {
        drogon::app().setLogLevel(trantor::Logger::kDebug);
    }
    else if (level == "INFO") {
        drogon::app().setLogLevel(trantor::Logger::kInfo);
    }
    else if (level == "WARN") {
        drogon::app().setLogLevel(trantor::Logger::kWarn);
    }
    else {
        throw std::invalid_argument("Invalid log level: " + level + ". Expected TRACE, DEBUG, INFO, or WARN.");
    }

    return *this;
}

LuaHttpAppFramework &LuaHttpAppFramework::enableRunAsDaemon() {
    drogon::app().enableRunAsDaemon();
    return *this;
}

LuaHttpAppFramework &LuaHttpAppFramework::loadJsonConfig(const std::string &file) {
    drogon::app().loadConfigFile("./" + file + ".json");
    return *this;
}

void LuaHttpAppFramework::run() {
    drogon::app().run();
}

