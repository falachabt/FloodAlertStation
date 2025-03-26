// include/utils/Logger.h
#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

// Niveaux de log
enum LogLevel {
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARNING = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_DEBUG = 4
};

class Logger {
public:
    // Initialiser le logger
    static void begin(LogLevel level = LOG_LEVEL_INFO);
    
    // Activer/désactiver tous les logs
    static void enableLogs(bool enable);
    
    // Les logs système sont-ils activés?
    static bool isLogsEnabled();
    
    // Définir le niveau de log
    static void setLogLevel(LogLevel level);
    
    // Obtenir le niveau de log actuel
    static LogLevel getLogLevel();
    
    // Logs avec niveau
    static void error(const String& message);
    static void warning(const String& message);
    static void info(const String& message);
    static void debug(const String& message);
    
    // Fonctions de log avec formatage - à utiliser avec des variables
    template<typename... Args>
    static void errorF(const char* format, Args... args) {
        if (_logsEnabled && _currentLogLevel >= LOG_LEVEL_ERROR) {
            Serial.print("[ERROR] ");
            Serial.printf(format, args...);
            Serial.println();
        }
    }
    
    template<typename... Args>
    static void warningF(const char* format, Args... args) {
        if (_logsEnabled && _currentLogLevel >= LOG_LEVEL_WARNING) {
            Serial.print("[WARNING] ");
            Serial.printf(format, args...);
            Serial.println();
        }
    }
    
    template<typename... Args>
    static void infoF(const char* format, Args... args) {
        if (_logsEnabled && _currentLogLevel >= LOG_LEVEL_INFO) {
            Serial.print("[INFO] ");
            Serial.printf(format, args...);
            Serial.println();
        }
    }
    
    template<typename... Args>
    static void debugF(const char* format, Args... args) {
        if (_logsEnabled && _currentLogLevel >= LOG_LEVEL_DEBUG) {
            Serial.print("[DEBUG] ");
            Serial.printf(format, args...);
            Serial.println();
        }
    }
    
    // Log pour l'interface utilisateur - toujours affiché, non filtré par le niveau de log
    static void ui(const String& message);
    
    template<typename... Args>
    static void uiF(const char* format, Args... args) {
        Serial.printf(format, args...);
        Serial.println();
    }
    
private:
    // Déclaration des variables statiques (définies dans Logger.cpp)
    static LogLevel _currentLogLevel;
    static bool _logsEnabled;
};

#endif // LOGGER_H