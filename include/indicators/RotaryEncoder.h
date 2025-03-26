// include/indicators/RotaryEncoder.h
#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include <Arduino.h>
#include "utils/Logger.h"

/**
 * Classe pour gérer un encodeur rotatif
 * Détecte les rotations dans le sens horaire, antihoraire et les pressions sur le bouton
 */
class RotaryEncoder {
public:
    // Constructeur avec les pins pour DT, CLK et SW
    RotaryEncoder(int dt_pin, int clk_pin, int sw_pin);
    
    // Initialiser l'encodeur
    void begin();
    
    // Mettre à jour l'état de l'encodeur (à appeler dans la boucle principale)
    void update();
    
    // Méthodes pour détecter les événements
    bool isRotatedClockwise();       // Rotation dans le sens horaire ?
    bool isRotatedCounterClockwise(); // Rotation dans le sens antihoraire ?
    bool isButtonPressed();          // Bouton pressé ?
    bool isLongPress();              // Pression longue détectée ?
    
    // Obtenir le compteur de l'encodeur
    int getCount() { return _encoderCount; }
    
    // Réinitialiser les événements après traitement
    void resetEvents();
    
private:
    int _dtPin;   // Pin DT
    int _clkPin;  // Pin CLK
    int _swPin;   // Pin bouton
    
    int _previousOutput;  // État précédent de la pin DT
    int _encoderCount;    // Compteur de l'encodeur
    bool _buttonState;    // État actuel du bouton
    bool _lastButtonState; // État précédent du bouton
    
    // Flags d'événement
    bool _rotatedCW;     // Rotation horaire détectée
    bool _rotatedCCW;    // Rotation antihoraire détectée
    bool _buttonPressed; // Pression du bouton détectée
    bool _longPress;     // Pression longue détectée
    
    // Variables pour le debounce du bouton
    unsigned long _lastDebounceTime;
    unsigned long _debounceDelay;
    
    // Variables pour détecter les pressions longues
    unsigned long _pressStartTime;
    bool _isButtonDown;
    unsigned long _longPressThreshold;
};

// src/indicators/RotaryEncoder.cpp
#include "indicators/RotaryEncoder.h"

RotaryEncoder::RotaryEncoder(int dt_pin, int clk_pin, int sw_pin)
    : _dtPin(dt_pin), _clkPin(clk_pin), _swPin(sw_pin),
      _encoderCount(0), _buttonState(HIGH), _lastButtonState(HIGH),
      _rotatedCW(false), _rotatedCCW(false), _buttonPressed(false), _longPress(false),
      _lastDebounceTime(0), _debounceDelay(50),
      _pressStartTime(0), _isButtonDown(false), _longPressThreshold(1000) {
}

void RotaryEncoder::begin() {
    // Configurer les pins
    pinMode(_dtPin, INPUT);
    pinMode(_clkPin, INPUT);
    pinMode(_swPin, INPUT_PULLUP); // Utiliser la résistance de pull-up interne
    
    // Initialiser l'état
    _previousOutput = digitalRead(_dtPin);
    
    Logger::info("Encodeur rotatif initialisé");
}

void RotaryEncoder::update() {
    // Lire l'état actuel des pins de l'encodeur
    int currentDT = digitalRead(_dtPin);
    
    // Vérifier la rotation
    if (currentDT != _previousOutput) {
        if (digitalRead(_clkPin) != _previousOutput) {
            _encoderCount++;
            _encoderCount++;
            _rotatedCW = true;
            Logger::debugF("Encodeur tourné dans le sens horaire: %d", _encoderCount);
        } else {
            _encoderCount--;
            _encoderCount--;
            _rotatedCCW = true;
            Logger::debugF("Encodeur tourné dans le sens antihoraire: %d", _encoderCount);
        }
    }
    _previousOutput = currentDT;
    
    // Lire l'état du bouton avec debounce
    int reading = digitalRead(_swPin);
    
    // Si l'état du bouton a changé
    if (reading != _lastButtonState) {
        _lastDebounceTime = millis();
    }
    
    // Vérifier si l'état du bouton est stable plus longtemps que le délai de debounce
    if ((millis() - _lastDebounceTime) > _debounceDelay) {
        // Si l'état du bouton a changé
        if (reading != _buttonState) {
            _buttonState = reading;
            
            // Si le bouton est pressé (LOW car INPUT_PULLUP)
            if (_buttonState == LOW) {
                _isButtonDown = true;
                _pressStartTime = millis();
                Logger::debug("Bouton de l'encodeur pressé");
            } 
            // Si le bouton a été relâché
            else if (_buttonState == HIGH && _isButtonDown) {
                _isButtonDown = false;
                
                // Vérifier si c'était une pression courte ou longue
                if (millis() - _pressStartTime < _longPressThreshold) {
                    _buttonPressed = true;
                    Logger::debug("Pression courte détectée");
                } else {
                    // C'était déjà détecté comme une pression longue
                    Logger::debug("Pression longue terminée");
                }
            }
        }
    }
    
    // Vérifier si une pression longue est en cours
    if (_isButtonDown && !_longPress && (millis() - _pressStartTime > _longPressThreshold)) {
        _longPress = true;
        Logger::debug("Pression longue détectée");
    }
    
    _lastButtonState = reading;
}

bool RotaryEncoder::isRotatedClockwise() {
    return _rotatedCW;
}

bool RotaryEncoder::isRotatedCounterClockwise() {
    return _rotatedCCW;
}

bool RotaryEncoder::isButtonPressed() {
    return _buttonPressed;
}

bool RotaryEncoder::isLongPress() {
    return _longPress;
}

void RotaryEncoder::resetEvents() {
    _rotatedCW = false;
    _rotatedCCW = false;
    _buttonPressed = false;
    _longPress = false;
}

#endif // ROTARY_ENCODER_H