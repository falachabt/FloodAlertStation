#ifndef CONFIG_H
#define CONFIG_H

// Mode de fonctionnement - changer cette valeur pour compiler soit le master, soit le slave
#define MODE_MASTER true  // true pour master, false pour slave

// Configuration réseau
#define DEVICE_NAME "AlertStation"  // Nom pour le master
#define SLAVE_NAME "WaterSensor"    // Nom pour le slave
#define MIN_PEERS 1                 // Nombre minimum de pairs à connecter
#define WIFI_CHANNEL 1              // Canal WiFi pour ESP-NOW

// Configuration WiFi (uniquement pour le master)
#define AP_SSID "FloodAlertSystem"   // Nom du point d'accès
#define AP_PASSWORD "floodalert123"  // Mot de passe du point d'accès

// Seuils d'alerte pour le niveau d'eau
#define WATER_WARNING_THRESHOLD 50   // Seuil d'avertissement (en cm)
#define WATER_CRITICAL_THRESHOLD 75  // Seuil critique (en cm)

// Configuration du capteur de niveau d'eau (pour slave)
#define WATER_LEVEL_SENSOR_PIN 34    // GPIO pour le capteur de niveau d'eau

// Configuration du capteur DHT11 (pour master)
#define DHT11_PIN 2                // GPIO pour le capteur DHT11
#define TEMP_WARNING_THRESHOLD 35    // Seuil d'avertissement de température

// Nombre maximum de capteurs distants
#define MAX_SENSORS 10

#endif // CONFIG_H