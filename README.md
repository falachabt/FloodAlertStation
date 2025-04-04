# 🌊 FloodAlertStation

![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Platform](https://img.shields.io/badge/platform-ESP32-orange.svg)
![Status](https://img.shields.io/badge/status-Active-brightgreen.svg)

Un système d'alerte d'inondation intelligent basé sur ESP32, conçu pour surveiller les niveaux d'eau en temps réel, envoyer des alertes automatiques et fournir une interface web conviviale pour la gestion des données.

---

## 📑 Sommaire

1. [📋 Description](#-description)
2. [🔍 Caractéristiques principales](#-caractéristiques-principales)
3. [🛠️ Matériel requis](#️-matériel-requis)
4. [💻 Prérequis logiciels](#-prérequis-logiciels)
5. [🔧 Installation et configuration](#-installation-et-configuration)
6. [📊 Utilisation](#-utilisation)
7. [📚 Structure du projet](#-structure-du-projet)
8. [📦 Dépendances](#-dépendances)
9. [🤝 Contribution](#-contribution)
10. [📝 Licence](#-licence)
11. [📧 Contact](#-contact)

---

## 📋 Description

**FloodAlertStation** est un projet IoT basé sur ESP32 qui permet de surveiller les niveaux d'eau et les conditions environnementales en temps réel. Il est conçu pour fournir des alertes visuelles, sonores et numériques en cas de risque d'inondation, tout en offrant une interface web pour la gestion et la visualisation des données.

---

## 🔍 Caractéristiques principales

- 🌐 **Surveillance en temps réel** : Mesure des niveaux d'eau et des conditions environnementales (température, humidité).
- 🚨 **Alertes intelligentes** : Notifications automatiques via LED, buzzer et interface web.
- 🔋 **Autonomie énergétique** : Fonctionnement sur batterie avec option d'alimentation solaire.
- 📊 **Interface web** : Visualisation des données en temps réel et historique des alertes.
- 📡 **Communication sans fil** : Utilisation du protocole ESP-NOW pour la communication entre les nœuds.

---

## 🛠️ Matériel requis

- **ESP32 DOIT DevKit V1**
- **Capteur DHT** (température et humidité)
- **Capteur de niveau d'eau**
- **LEDs** (rouge, jaune, vert) pour les alertes visuelles
- **Buzzer** pour les alertes sonores
- **Alimentation** (batterie ou panneau solaire)
- **Boîtier étanche** pour protéger le matériel

---

## 💻 Prérequis logiciels

- [PlatformIO](https://platformio.org/) (extension VS Code recommandée)
- [Arduino IDE](https://www.arduino.cc/en/software) (alternative)
- Bibliothèques nécessaires :
  - [ArduinoJson](https://arduinojson.org/)
  - [DHT Sensor Library](https://github.com/adafruit/DHT-sensor-library)

---

## 🔧 Installation et configuration

1. **Clonez le dépôt :**
   ```bash
   git clone https://github.com/votre-username/FloodAlertStation.git
   cd FloodAlertStation
   ```

2. **Ouvrez le projet dans PlatformIO :**
   - Dans VS Code, sélectionnez "Open Project" et naviguez vers le dossier cloné.

3. **Configurez les paramètres :**
   - Modifiez `include/Config.h` pour définir les broches des capteurs et des indicateurs.
   - Configurez les seuils d'alerte et les paramètres réseau.

4. **Compilez et téléversez le code :**
   ```bash
   pio run --target upload
   ```

5. **Téléversez les fichiers du système de fichiers (interface web) :**
   ```bash
   pio run --target uploadfs
   ```

6. **Accédez à l'interface web :**
   - Connectez-vous au point d'accès ESP32 (SSID par défaut : "FloodAlert").
   - Naviguez vers `http://192.168.4.1`.

---

## 📊 Utilisation

1. **Démarrage :**
   - Le système démarre automatiquement et commence à surveiller les niveaux d'eau.
   - Les données sont affichées sur l'interface web.

2. **Alertes :**
   - Les alertes sont déclenchées lorsque les seuils critiques sont atteints :
     - **LED rouge** : Niveau d'eau critique.
     - **LED jaune** : Niveau d'eau modéré.
     - **LED verte** : Niveau d'eau normal.
     - **Buzzer** : Alertes sonores configurables.

3. **Commandes via le moniteur série :**
   - `silence` ou `s` : Désactiver les alertes sonores.
   - `test` ou `t` : Tester les alertes sonores.

---

## 📚 Structure du projet

```
FloodAlertStation/
├── data/                  # Fichiers pour l'interface web
│   ├── css/               # Styles CSS
│   ├── js/                # Scripts JavaScript
│   └── *.html             # Pages HTML
├── include/               # Fichiers d'en-tête
│   ├── indicators/        # Interfaces pour les alertes (LED, buzzer)
│   ├── network/           # Communication réseau (ESP-NOW)
│   ├── sensors/           # Capteurs (DHT, niveau d'eau)
│   └── utils/             # Fonctions utilitaires
├── src/                   # Code source principal
│   ├── indicators/        # Implémentation des alertes
│   ├── network/           # Protocole ESP-NOW
│   ├── sensors/           # Implémentation des capteurs
│   └── utils/             # Journalisation
├── lib/                   # Bibliothèques spécifiques au projet
├── platformio.ini         # Configuration PlatformIO
└── README.md              # Documentation du projet
```

---

Data Flow
The system operates on a hub-and-spoke model where slave nodes collect sensor data and transmit to the master node using ESP-NOW protocol. The master node aggregates data and provides web interface access.

[Slave Node 1]     [Slave Node 2]     [Slave Node 3]
     │                  │                  │
     └──────────────────┼──────────────────┘
                        │
                        ▼
                  [Master Node]
                        │
                        ▼
                [Web Interface]

Key interactions:

Slave nodes read water level and temperature data every 5 seconds
Data is transmitted to master node using ESP-NOW protocol
Master node processes incoming data and updates alert status
Web interface polls master node every 30 seconds for updates
Alert indicators are triggered based on configurable thresholds
Network status is monitored and displayed on dashboard

## 📦 Dépendances

- [ArduinoJson](https://arduinojson.org/) - Gestion des données JSON.
- [DHT Sensor Library](https://github.com/adafruit/DHT-sensor-library) - Communication avec le capteur DHT.
- [ESP-NOW](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html) - Protocole de communication sans fil.

---

## 🤝 Contribution

Les contributions sont les bienvenues ! Voici comment contribuer :

1. **Forkez le dépôt.**
2. **Créez une branche :**
   ```bash
   git checkout -b feature/ma-nouvelle-fonctionnalite
   ```
3. **Commitez vos changements :**
   ```bash
   git commit -m "Ajout de ma nouvelle fonctionnalité"
   ```
4. **Poussez vers la branche :**
   ```bash
   git push origin feature/ma-nouvelle-fonctionnalite
   ```
5. **Ouvrez une Pull Request.**

---

## 📝 Licence

Ce projet est sous licence MIT. Consultez le fichier `LICENSE` pour plus de détails.

---

## 📧 Contact

Pour toute question ou suggestion, contactez-moi à : **[votre-email@example.com]**

---

## 🎨 Aperçu

![Dashboard](https://via.placeholder.com/800x400?text=Capture+d%27%C3%A9cran+de+l%27interface+web)
*Exemple de l'interface web pour la surveillance en temps réel.*

---

Avec ce README, votre projet sera bien présenté sur GitHub et attirera l'attention des contributeurs et utilisateurs potentiels. Vous pouvez ajouter des captures d'écran ou des diagrammes pour enrichir encore plus la documentation.