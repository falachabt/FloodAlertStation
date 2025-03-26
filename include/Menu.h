// include/Menu.h
#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <vector>
#include <functional>
#include "utils/Logger.h"

// Déclarations anticipées
class RotaryEncoder;
class FloodAlertSystem;

// Structure d'élément de menu
struct MenuItem {
    String title;                        // Titre de l'élément
    std::function<void()> action;        // Action à exécuter (facultatif)
    std::vector<MenuItem*> subMenuItems; // Sous-éléments
    MenuItem* parent;                    // Élément parent pour le fil d'Ariane
    
    MenuItem(const String& t, std::function<void()> a = nullptr, MenuItem* p = nullptr) 
        : title(t), action(a), parent(p) {}
    
    ~MenuItem() {
        // Nettoyer les sous-éléments lors de la destruction
        for (auto item : subMenuItems) {
            delete item;
        }
        subMenuItems.clear();
    }
    
    // Ajouter un sous-élément
    MenuItem* addSubMenuItem(const String& title, std::function<void()> action = nullptr) {
        MenuItem* item = new MenuItem(title, action, this);
        subMenuItems.push_back(item);
        return item;
    }
};

class Menu {
public:
    Menu(RotaryEncoder* encoder);
    ~Menu();
    
    // Initialiser le système de menu
    void begin();
    
    // Traiter les interactions de l'encodeur
    void update();
    
    // Créer la structure du menu (à appeler une fois dans setup)
    void createMenus();
    
    // Définir la référence au système d'alerte pour permettre les contrôles système
    void setFloodAlertSystem(FloodAlertSystem* system) { _floodSystem = system; }
    
    // Navigation dans le menu
    void selectCurrentItem();
    void goBack();

    // Afficher le menu actuel sur le port série
    void printCurrentMenu();
    
    // Activer/désactiver les logs système via un appui long sur le bouton
    void toggleSystemLogs();
    
    // Obtenir le fil d'Ariane pour la position actuelle
    String getBreadcrumb();
    
private:
    RotaryEncoder* _encoder;
    FloodAlertSystem* _floodSystem; // Référence au système principal
    MenuItem* _rootMenu;
    MenuItem* _currentMenu;
    int _currentSelection;
    std::vector<MenuItem*> _menuStack;
    
    void _navigateToSubMenu(MenuItem* submenu);
};

Menu::Menu(RotaryEncoder* encoder) 
    : _encoder(encoder), _floodSystem(nullptr), _currentSelection(0) {
    // Créer le menu racine
    _rootMenu = new MenuItem("Menu Principal");
    _currentMenu = _rootMenu;
}

Menu::~Menu() {
    delete _rootMenu; // Supprime récursivement tous les éléments du menu
}

void Menu::begin() {
    createMenus();
    printCurrentMenu();
}

void Menu::update() {
    // Vérifier les événements de l'encodeur
    if (_encoder->isRotatedClockwise()) {
        // Déplacer la sélection vers le bas
        _currentSelection++;
        if (_currentSelection >= _currentMenu->subMenuItems.size()) {
            _currentSelection = 0; // Revenir au début
        }
        printCurrentMenu();
    }
    
    if (_encoder->isRotatedCounterClockwise()) {
        // Déplacer la sélection vers le haut
        _currentSelection--;
        if (_currentSelection < 0) {
            _currentSelection = _currentMenu->subMenuItems.size() - 1; // Aller à la fin
        }
        printCurrentMenu();
    }
    
    if (_encoder->isButtonPressed()) {
        // Sélectionner l'élément actuel
        selectCurrentItem();
    }
    
    // Vérifier si un appui long est détecté pour basculer les logs système
    if (_encoder->isLongPress()) {
        toggleSystemLogs();
    }
    
    // Réinitialiser les événements de l'encodeur
    _encoder->resetEvents();
}

void Menu::toggleSystemLogs() {
    bool logsEnabled = Logger::isLogsEnabled();
    Logger::enableLogs(!logsEnabled);
    
    if (Logger::isLogsEnabled()) {
        Logger::ui("\n--- Logs système ACTIVÉS ---");
    } else {
        Logger::ui("\n--- Logs système DÉSACTIVÉS ---");
    }
}

String Menu::getBreadcrumb() {
    String breadcrumb = "";
    
    // Commencer par l'élément actuel
    MenuItem* item = _currentMenu;
    
    // Construire le chemin inverse (du niveau actuel vers la racine)
    std::vector<String> path;
    while (item != nullptr) {
        path.push_back(item->title);
        item = item->parent;
    }
    
    // Construire le fil d'Ariane dans l'ordre correct (de la racine au niveau actuel)
    for (int i = path.size() - 1; i >= 0; i--) {
        breadcrumb += path[i];
        if (i > 0) breadcrumb += " > ";
    }
    
    return breadcrumb;
}

void Menu::createMenus() {
    // Ajouter des éléments au menu racine
    MenuItem* sensorStatus = _rootMenu->addSubMenuItem("État des capteurs");
    MenuItem* alertSettings = _rootMenu->addSubMenuItem("Paramètres d'alerte");
    MenuItem* networkSettings = _rootMenu->addSubMenuItem("Paramètres réseau");
    MenuItem* systemInfo = _rootMenu->addSubMenuItem("Informations système");
    MenuItem* logSettings = _rootMenu->addSubMenuItem("Paramètres des logs");
    
    // Ajouter des éléments au menu "État des capteurs"
    sensorStatus->addSubMenuItem("Voir tous les capteurs", [this]() {
        Logger::ui("\n--- Tous les capteurs ---");
        // Afficher les données de tous les capteurs
        if (_floodSystem) {
            Logger::ui("Liste des capteurs connectés :");
            // Ici, on afficherait la liste des capteurs
            // (Simulation pour l'exemple)
            
            Logger::ui("- Capteur 1: OK");
            Logger::ui("- Capteur 2: Avertissement");
            Logger::ui("- Capteur 3: OK");
        } else {
            Logger::ui("Système non disponible");
        }
    });
    
    sensorStatus->addSubMenuItem("Niveaux d'eau", [this]() {
        Logger::ui("\n--- Niveaux d'eau ---");
        // Afficher les données de niveau d'eau
        Logger::ui("Capteur 1: 15.2 cm");
        Logger::ui("Capteur 2: 32.7 cm (Avertissement)");
        Logger::ui("Capteur 3: 8.5 cm");
    });
    
    sensorStatus->addSubMenuItem("Température", [this]() {
        Logger::ui("\n--- Données de température ---");
        // Afficher les données de température
        Logger::ui("Capteur 1: 24.5°C");
        Logger::ui("Capteur 2: 26.8°C");
        Logger::ui("Capteur 3: 23.2°C");
    });
    
    sensorStatus->addSubMenuItem("Retour", [this]() { goBack(); });
    
    // Ajouter des éléments au menu "Paramètres d'alerte"
    alertSettings->addSubMenuItem("Seuil d'avertissement", [this]() { 
        Logger::ui("Réglage du seuil d'avertissement - Ajuster avec l'encodeur");
        // Permettre d'ajuster le seuil d'avertissement
    });
    
    alertSettings->addSubMenuItem("Seuil critique", [this]() { 
        Logger::ui("Réglage du seuil critique - Ajuster avec l'encodeur"); 
        // Permettre d'ajuster le seuil critique
    });
    
    alertSettings->addSubMenuItem("Couper l'alerte", [this]() { 
        Logger::ui("Coupure de l'alerte...");
        if (_floodSystem) {
            _floodSystem->silenceAudioAlert();
            Logger::ui("Alerte coupée via le menu");
        }
    });
    
    alertSettings->addSubMenuItem("Tester l'alerte", [this]() { 
        Logger::ui("Test de l'alerte...");
        // Déclencher une alerte de test
        if (_floodSystem && _floodSystem->getBuzzerIndicator()) {
            _floodSystem->getBuzzerIndicator()->playAlertTone(2);
            Logger::ui("Test d'alerte déclenché");
        }
    });
    
    alertSettings->addSubMenuItem("Retour", [this]() { goBack(); });
    
    // Ajouter des éléments au menu "Paramètres réseau"
    networkSettings->addSubMenuItem("État WiFi", [this]() {
        Logger::ui("\n--- État WiFi ---");
        if (_floodSystem) {
            Logger::uiF("Réseau prêt: %s", _floodSystem->getNetwork().isNetworkReady() ? "Oui" : "Non");
            Logger::uiF("Pairs connectés: %d", _floodSystem->getNetwork().getPeerCount());
        }
    });
    
    networkSettings->addSubMenuItem("Infos des pairs", [this]() {
        Logger::ui("\n--- Informations des pairs ---");
        if (_floodSystem) {
            _floodSystem->getNetwork().printPeers();
        }
    });
    
    networkSettings->addSubMenuItem("Scanner les réseaux", [this]() {
        Logger::ui("\n--- Scan des réseaux ---");
        Logger::ui("Recherche des réseaux WiFi...");
        // Déclencherait un scan WiFi
    });
    
    networkSettings->addSubMenuItem("Retour", [this]() { goBack(); });
    
    // Ajouter des éléments au menu "Informations système"
    systemInfo->addSubMenuItem("Infos de l'appareil", [this]() {
        Logger::ui("\n--- Informations de l'appareil ---");
        Logger::uiF("Nom de l'appareil: %s", DEVICE_NAME);
        
        // Afficher l'adresse MAC
        uint8_t mac[6];
        if (_floodSystem) {
            _floodSystem->getNetwork().getOwnMac(mac);
            Logger::ui("Adresse MAC: ");
            Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X\n", 
                         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        }
        
        // Afficher le temps de fonctionnement
        unsigned long uptime = millis() / 1000;
        Logger::uiF("Temps de fonctionnement: %dh %dm %ds", 
                  uptime / 3600, (uptime % 3600) / 60, uptime % 60);
    });
    
    systemInfo->addSubMenuItem("État du réseau", [this]() {
        Logger::ui("\n--- État du réseau ---");
        if (_floodSystem) {
            _floodSystem->getNetwork().printNetworkStatus();
        }
    });
    
    systemInfo->addSubMenuItem("Version", [this]() {
        Logger::ui("\n--- Informations de version ---");
        Logger::ui("Flood Alert System v1.0");
        Logger::uiF("Date de compilation: %s %s", __DATE__, __TIME__);
    });
    
    systemInfo->addSubMenuItem("Retour", [this]() { goBack(); });
    
    // Ajouter des éléments au menu "Paramètres des logs"
    logSettings->addSubMenuItem("Activer les logs", [this]() {
        Logger::enableLogs(true);
        Logger::ui("Logs système activés");
    });
    
    logSettings->addSubMenuItem("Désactiver les logs", [this]() {
        Logger::enableLogs(false);
        Logger::ui("Logs système désactivés");
    });
    
    logSettings->addSubMenuItem("Niveau: Erreur uniquement", [this]() {
        Logger::setLogLevel(LOG_LEVEL_ERROR);
        Logger::ui("Niveau de log: ERREUR");
    });
    
    logSettings->addSubMenuItem("Niveau: Avertissements", [this]() {
        Logger::setLogLevel(LOG_LEVEL_WARNING);
        Logger::ui("Niveau de log: AVERTISSEMENT");
    });
    
    logSettings->addSubMenuItem("Niveau: Info", [this]() {
        Logger::setLogLevel(LOG_LEVEL_INFO);
        Logger::ui("Niveau de log: INFO");
    });
    
    logSettings->addSubMenuItem("Niveau: Debug", [this]() {
        Logger::setLogLevel(LOG_LEVEL_DEBUG);
        Logger::ui("Niveau de log: DEBUG");
    });
    
    logSettings->addSubMenuItem("Retour", [this]() { goBack(); });
}

void Menu::selectCurrentItem() {
    if (_currentMenu->subMenuItems.size() == 0) {
        return; // Pas d'éléments à sélectionner
    }
    
    MenuItem* selectedItem = _currentMenu->subMenuItems[_currentSelection];
    
    // Exécuter l'action si définie
    if (selectedItem->action) {
        selectedItem->action();
    }
    // Si a un sous-menu, y naviguer
    else if (selectedItem->subMenuItems.size() > 0) {
        _navigateToSubMenu(selectedItem);
    }
}

void Menu::goBack() {
    if (_menuStack.size() > 0) {
        // Récupérer le menu précédent dans la pile
        _currentMenu = _menuStack.back();
        _menuStack.pop_back();
        _currentSelection = 0;
        printCurrentMenu();
    }
}

void Menu::_navigateToSubMenu(MenuItem* submenu) {
    _menuStack.push_back(_currentMenu);
    _currentMenu = submenu;
    _currentSelection = 0;
    printCurrentMenu();
}

void Menu::printCurrentMenu() {
    // Afficher le fil d'Ariane
    String breadcrumb = getBreadcrumb();
    Logger::ui("\n" + breadcrumb);
    
    // Afficher une ligne de séparation
    Logger::ui("------------------------");
    
    // Afficher les éléments du menu
    for (size_t i = 0; i < _currentMenu->subMenuItems.size(); i++) {
        if (i == _currentSelection) {
            Logger::uiF("> %s", _currentMenu->subMenuItems[i]->title.c_str());
        } else {
            Logger::uiF("  %s", _currentMenu->subMenuItems[i]->title.c_str());
        }
    }
    
    // Afficher les instructions
    Logger::ui("------------------------");
    Logger::ui("Rotation: naviguer, Appui court: sélectionner");
    Logger::ui("Appui long: activer/désactiver les logs système");
}

#endif // MENU_H