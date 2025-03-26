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
#define WATER_WARNING_THRESHOLD 10   // Seuil d'avertissement (en cm)
#define WATER_CRITICAL_THRESHOLD 20  // Seuil critique (en cm)

// Configuration du capteur de niveau d'eau (pour slave)
#define WATER_LEVEL_SENSOR_PIN 34    // GPIO pour le capteur de niveau d'eau

// Configuration du capteur DHT11 (pour master)
#define DHT11_PIN 2                  // GPIO pour le capteur DHT11
#define TEMP_WARNING_THRESHOLD 35    // Seuil d'avertissement de température

// Configuration des LEDs pour les alertes visuelles
#define LED_RED_PIN 22              // GPIO pour la LED rouge
#define LED_YELLOW_PIN 4            // GPIO pour la LED jaune
#define LED_GREEN_PIN 23            // GPIO pour la LED verte

// Configuration du buzzer pour les alertes sonores
#define BUZZER_PIN 15               // GPIO pour le buzzer

// Configuration de l'encodeur rotatif (pour l'interface utilisateur du master)
#define ROTARY_ENCODER_DT_PIN 14    // GPIO pour la broche DT de l'encodeur
#define ROTARY_ENCODER_CLK_PIN 12   // GPIO pour la broche CLK de l'encodeur
#define ROTARY_ENCODER_SW_PIN 27    // GPIO pour la broche SW (bouton) de l'encodeur

// Configuration de l'alerte visuelle
#define WATER_ALERT_DELAY_MS 60000  // Délai en milliseconds (1 minute) avant activation de l'alerte

// Nombre maximum de capteurs distants
#define MAX_SENSORS 10

#endif // CONFIG_H