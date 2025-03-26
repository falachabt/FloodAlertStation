// src/utils/Logger.cpp
#include "utils/Logger.h"

// Définition des variables statiques
LogLevel Logger::_currentLogLevel = LOG_LEVEL_INFO;
bool Logger::_logsEnabled = true;

void Logger::begin(LogLevel level) {
    _currentLogLevel = level;
    _logsEnabled = true;
}

void Logger::enableLogs(bool enable) {
    _logsEnabled = enable;
}

bool Logger::isLogsEnabled() {
    return _logsEnabled;
}

void Logger::setLogLevel(LogLevel level) {
    _currentLogLevel = level;
}

LogLevel Logger::getLogLevel() {
    return _currentLogLevel;
}

void Logger::error(const String& message) {
    if (_logsEnabled && _currentLogLevel >= LOG_LEVEL_ERROR) {
        Serial.print("[ERROR] ");
        Serial.println(message);
    }
}

void Logger::warning(const String& message) {
    if (_logsEnabled && _currentLogLevel >= LOG_LEVEL_WARNING) {
        Serial.print("[WARNING] ");
        Serial.println(message);
    }
}

void Logger::info(const String& message) {
    if (_logsEnabled && _currentLogLevel >= LOG_LEVEL_INFO) {
        Serial.print("[INFO] ");
        Serial.println(message);
    }
}

void Logger::debug(const String& message) {
    if (_logsEnabled && _currentLogLevel >= LOG_LEVEL_DEBUG) {
        Serial.print("[DEBUG] ");
        Serial.println(message);
    }
}

void Logger::ui(const String& message) {
    Serial.println(message);
}