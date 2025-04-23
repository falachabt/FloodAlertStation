#define ENABLE_GxEPD2_GFX 0
#include <GxEPD2_BW.h>
#include <SPI.h>

// Configuration de l'écran 5.8" (792x272)
GxEPD2_579_GDEY0579T93 epd_raw(/*CS=*/5, /*DC=*/17, /*RST=*/16, /*BUSY=*/4); 
GxEPD2_BW<GxEPD2_579_GDEY0579T93, GxEPD2_579_GDEY0579T93::HEIGHT> display(epd_raw);

// Variables des données
float niveauEau = 3.2;      // en mètres
float temperature = 16.5;   // en °C
bool capteurImmerse = true;
int niveauAlerte = 0;       // 0=Normal, 1=Vigilance, 2=Alerte, 3=Danger
int heure = 15;
int minute = 42;

// Messages d'alerte pour chaque niveau
const char* messages[] = {
  "NORMAL",
  "VIGILANCE",
  "ALERTE",
  "DANGER"
};

// Instructions pour chaque niveau d'alerte
const char* instructions[] = {
  "Aucune action requise",
  "Suivez l'évolution",
  "Préparez-vous",
  "ÉVACUATION POSSIBLE"
};


// Fonction pour mettre à jour l'affichage
void updateDisplay();
void drawCenteredText(const char* text, int x, int y);
void drawTextAt(const char* text, int x, int y);


void setup() {
  Serial.begin(115200);
  
  // Initialiser SPI
  SPI.begin(/*SCK*/18, /*MISO*/-1, /*MOSI*/23, /*SS*/5);
  
  // Initialiser l'écran
  display.init();
  
  // Premier affichage
  updateDisplay();
}

void loop() {
  delay(30000);  // Attendre 30 secondes
  
  // Simuler des changements
  niveauEau += random(-15, 15) / 100.0;
  if (niveauEau < 0.5) niveauEau = 0.5;
  if (niveauEau > 5.5) niveauEau = 5.5;
  
  temperature += random(-5, 5) / 10.0;
  capteurImmerse = (niveauEau > 0.3);
  
  // Mise à jour de l'heure
  minute++;
  if (minute >= 60) {
    minute = 0;
    heure++;
    if (heure >= 24) {
      heure = 0;
    }
  }
  
  // Ajuster le niveau d'alerte
  if (niveauEau > 4.5) niveauAlerte = 3;
  else if (niveauEau > 3.5) niveauAlerte = 2;
  else if (niveauEau > 2.5) niveauAlerte = 1;
  else niveauAlerte = 0;
  
  // Mettre à jour l'affichage
  display.init(115200, false, 50, false);
  updateDisplay();
}

void updateDisplay() {
  display.setRotation(2);  // Mode paysage
  display.setFullWindow();
  display.firstPage();
  
  do {
    // Fond blanc
    display.fillScreen(GxEPD_WHITE);
    
    // Variables pour la disposition
    int w = display.width();  // 792
    int h = display.height(); // 272
    
    // ====== SECTION 1: ENTÊTE (60px de hauteur) ======
    // Cadre d'entête
    display.drawRect(0, 0, w, 60, GxEPD_BLACK);
    
    // Logo / Titre personnalisé à gauche
    display.setTextSize(2);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(20, 22);
    display.print("STATION MAISON DUBOIS");
    
    // Heure à droite
    char timeBuffer[10];
    sprintf(timeBuffer, "%02d:%02d", heure, minute);
    display.setTextSize(3);
    display.setCursor(w - 120, 22);
    display.print(timeBuffer);
    
    // ====== SECTION 2: TROIS PANNEAUX PRINCIPAUX (150px de hauteur) ======
    int panelY = 65;
    int panelHeight = 150;
    int panelWidth = w/3 - 6; // Légère marge entre les panneaux
    
    // ------ PANNEAU 1: NIVEAU D'ALERTE ------
    int panel1X = 5;
    
    // Cadre avec couleur selon niveau d'alerte
    if (niveauAlerte >= 2) {
      display.fillRect(panel1X, panelY, panelWidth, panelHeight, GxEPD_BLACK);
      display.setTextColor(GxEPD_WHITE);
    } else {
      display.drawRect(panel1X, panelY, panelWidth, panelHeight, GxEPD_BLACK);
      display.setTextColor(GxEPD_BLACK);
    }
    
    // Titre du panneau
    display.setTextSize(2);
    display.setCursor(panel1X + 10, panelY + 20);
    display.print("Status");
    
    // Affichage du niveau d'alerte
    display.setTextSize(4);
    int alerteTextX = panel1X + panelWidth/2 - 60; // Position ajustée manuellement
    display.setCursor(alerteTextX, panelY + 50);
    display.print(messages[niveauAlerte]);
    
    // Instructions selon niveau
    display.setTextSize(1);
    display.setCursor(panel1X + 10, panelY + 110);
    display.print(instructions[niveauAlerte]);
    
    // ------ PANNEAU 2: NIVEAU D'EAU ------
    int panel2X = panel1X + panelWidth + 6;
    
    // Cadre
    display.drawRect(panel2X, panelY, panelWidth, panelHeight, GxEPD_BLACK);
    display.setTextColor(GxEPD_BLACK);
    
    // Titre du panneau
    display.setTextSize(2);
    display.setCursor(panel2X + 10, panelY + 20);
    display.print("NIVEAU D'EAU");
    
    // Valeur du niveau d'eau en grand
    display.setTextSize(6);
    char waterBuffer[10];
    sprintf(waterBuffer, "%.1f", niveauEau);
    
    // Centrage manuel pour éviter les calculs qui peuvent causer des erreurs
    int waterTextWidth = strlen(waterBuffer) * 36; // Approximation pour taille 6
    int waterTextX = panel2X + (panelWidth - waterTextWidth) / 2;
    
    display.setCursor(waterTextX, panelY + 50);
    display.print(waterBuffer);
    
    // Unité
    display.setTextSize(2);
    display.setCursor(waterTextX + waterTextWidth + 5, panelY + 80);
    display.print("m");
    
    // État du capteur
    display.setTextSize(1);
    display.setCursor(panel2X + 10, panelY + 120);
    display.print(capteurImmerse ? "Capteur immergé" : "Capteur hors d'eau");
    
    // ------ PANNEAU 3: TEMPÉRATURE ------
    int panel3X = panel2X + panelWidth + 6;
    
    // Cadre
    display.drawRect(panel3X, panelY, panelWidth, panelHeight, GxEPD_BLACK);
    
    // Titre du panneau
    display.setTextSize(2);
    display.setCursor(panel3X + 10, panelY + 20);
    display.print("TEMPERATURE");
    
    // Valeur de la température en grand
    display.setTextSize(6);
    char tempBuffer[10];
    sprintf(tempBuffer, "%.1f", temperature);
    
    // Centrage manuel
    int tempTextWidth = strlen(tempBuffer) * 36; // Approximation pour taille 6
    int tempTextX = panel3X + (panelWidth - tempTextWidth) / 2;
    
    display.setCursor(tempTextX, panelY + 50);
    display.print(tempBuffer);
    
    // Unité
    display.setTextSize(2);
    display.setCursor(tempTextX + tempTextWidth + 5, panelY + 80);
    display.print("°C");
    
    // Description
    display.setTextSize(1);
    display.setCursor(panel3X + 10, panelY + 110);
    if (temperature < 5) {
      display.print("Risque de gel");
    } else if (temperature > 30) {
      display.print("Température élevée");
    } else {
      display.print("Température normale");
    }
    
    // ====== SECTION 3: JAUGE D'ALERTE (50px de hauteur) ======
    int gaugeY = panelY + panelHeight + 5;
    int gaugeHeight = 50;
    
    // Cadre de la jauge
    display.drawRect(5, gaugeY, w-10, gaugeHeight, GxEPD_BLACK);
    
    // Titre de la jauge
    display.setTextSize(1);
    display.setCursor(15, gaugeY + 12);
    display.print("ÉCHELLE D'ALERTE:");
    
    // Jauge elle-même
    int barY = gaugeY + 20;
    int barHeight = 20;
    int barWidth = w - 120; // Marge pour le texte à gauche
    int barX = 100;
    
    // Cadre de la barre
    display.drawRect(barX, barY, barWidth, barHeight, GxEPD_BLACK);
    
    // Seuils d'alerte marqués sur la barre
    int normalX = barX;
    int vigilanceX = barX + barWidth * 2.5/6.0;
    int alerteX = barX + barWidth * 3.5/6.0;
    int dangerX = barX + barWidth * 4.5/6.0;
    int maxX = barX + barWidth;
    
    // Lignes de séparation des seuils
    display.drawFastVLine(vigilanceX, barY, barHeight, GxEPD_BLACK);
    display.drawFastVLine(alerteX, barY, barHeight, GxEPD_BLACK);
    display.drawFastVLine(dangerX, barY, barHeight, GxEPD_BLACK);
    
    // Remplissage selon niveau actuel
    int fillWidth = min(barWidth, (int)(barWidth * niveauEau / 6.0));
    display.fillRect(barX, barY, fillWidth, barHeight, GxEPD_BLACK);
    
    // Étiquettes sous la barre
    display.setTextSize(1);
    
    // Utilisation de positions fixes calculées pour éviter tout chevauchement
    display.setCursor(normalX + 5, barY + barHeight + 12);
    display.print("0m");
    
    display.setCursor(vigilanceX - 15, barY + barHeight + 12);
    display.print("2.5m");
    
    display.setCursor(alerteX - 15, barY + barHeight + 12);
    display.print("3.5m");
    
    display.setCursor(dangerX - 15, barY + barHeight + 12);
    display.print("4.5m");
    
    display.setCursor(maxX - 15, barY + barHeight + 12);
    display.print("6m");
    
  } while (display.nextPage());
  
  // Économie d'énergie
  display.hibernate();
}