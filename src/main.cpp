#include "FloodAlertSystem.h"
#include "Config.h"

// Inclure les capteurs que vous souhaitez utiliser
#include "sensors/DHT11Sensor.h"
#include "sensors/WaterLevelSensor.h"
#include "indicators/LEDAlertIndicator.h"

#include <Ticker.h>
Ticker alertTestTicker;
bool testingAlert = false;
int testPhase = 0;


// Créer l'instance du système (ne pas utiliser "system" comme nom de variable)
FloodAlertSystem floodSystem;

LEDAlertIndicator ledIndicator(LED_RED_PIN, LED_YELLOW_PIN, LED_GREEN_PIN);



void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n=== FLOOD ALERT SYSTEM ===");
    
    // Initialiser le système dans le mode approprié défini dans Config.h
    bool isMaster = MODE_MASTER;
    
    ledIndicator.begin();
    floodSystem.begin(isMaster);
    
    if (isMaster) {
        Serial.println("System initialized in MASTER mode");
        
        // En mode MASTER, ajouter le capteur DHT11
        floodSystem.addSensor(new DHT11Sensor(DHT11_PIN));
        floodSystem.setLEDIndicator(&ledIndicator);
        Serial.println("DHT11 sensor added to master");
    } else {
        Serial.println("System initialized in SLAVE mode");
        
        // En mode SLAVE, ajouter le capteur de niveau d'eau
        floodSystem.addSensor(new WaterLevelSensor(WATER_LEVEL_SENSOR_PIN));
        Serial.println("Water level sensor added to slave");
    }
    
    // Initial LED status
    ledIndicator.setGreen(true);
    Serial.println("Setup complete!");
}

void loop() {
    // Mettre à jour le système (gère tous les composants)
    floodSystem.update();
    
    // Petite pause pour économiser de l'énergie
    delay(10);
}