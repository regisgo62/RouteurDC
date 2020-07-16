//**************************************************************/
// L'équipe de bidouilleurs discord reseautonome vous présente
// la réalisation d'un gradateur pour chauffe-eau en off grid
// gradateur synchronisé sur la production photovoltaique
// sans tirage sur les batteries pour utiliser le surplus dans
// un ballon d'eau chaude
//**************************************************************/

const char *version_soft = "Version 1.4 avec 1 diode ZC";

//**************************************************************/
// Initialisation
//**************************************************************/
#include "settings.h"
struct param routeur;
const int zeroc = 33;            // GPIO33  passage par zéro de la sinusoide
const int pinTriac = 27;         // GPIO27 triac
const int pinSortie2 = 13;       // pin13 pour  2eme "gradateur 
const int pinRelais = 19;        // Pin19 pour sortie relais
// analogique
const int pinPince = 32;         // GPIO32   pince effet hall
const int pinPinceAC = 32;       // GPIO35   pince alternative
const int pinPinceACref = 39;    // GPIO39   ref tension pour pince AC 30A/1V
const int pinPinceRef = 34;      // GPIO34   pince effet hall ref 2.5V
const int pinPotentiometre = 35; // GPIO35   potentiomètre
const int pinTension = 36;       // GPIO36   capteur tension
const int pinTemp = 23;          // GPIO23  capteurTempérature


short int sortieActive = 1;
bool marcheForcee = false;
short int marcheForceePercentage = 25;
unsigned long marcheForceeTemporisation = 60;
float intensiteDC = 0;
float capteurTension = 0;
int puissanceGradateur = 0;
float temperatureEauChaude = 0;
float puissanceDeChauffe = 0;
float puissanceAC = 0;
float intensiteAC = 0;
bool etatRelaisStatique = false;
bool modeparametrage = false;

int resetEsp = 0;
int testwifi = 0;
int choixSortie = 0;
int paramchange = 0;
bool SAP = false;
bool MQTT = false;
bool serverOn = false;

/**********************************************/
/********** déclaration des librairiess *******/
/**********************************************/
#include "triac.h"

#ifdef WifiMqtt
#include "modemqtt.h"
#endif

#ifdef WifiServer
#include "modeserveur.h"
#endif

#ifdef EEprom
#include "prgEEprom.h"
#endif

#ifdef EcranOled
#include "afficheur.h"
#endif

#ifdef Bluetooth
#include "modeBT.h"
#endif

//#define simulation // utiliser pour faire les essais sans les accessoires
#ifdef simulation
#include "simulation.h"
#endif

#include "mesure.h"
#include "regulation.h"

/***************************************/
/******** Programme principal   ********/
/***************************************/
void setup()
{

  Serial.begin(115200);

  pinMode(pinTriac, OUTPUT);  digitalWrite(pinTriac, LOW); // mise à zéro du triac au démarrage
  pinMode(pinSortie2, OUTPUT);  digitalWrite(pinSortie2, LOW); // mise à zéro du triac de la sortie 2
  pinMode(pinRelais, OUTPUT);  digitalWrite(pinRelais, HIGH); // mise à zéro du relais statique
  pinMode(zeroc,INPUT);
  pinMode(pinTemp, INPUT);
  pinMode(pinTension, INPUT);
  pinMode(pinPotentiometre, INPUT);
  pinMode(pinPince, INPUT);
  pinMode(pinPinceRef, INPUT);
  pinMode(pinPinceACref, INPUT);
  Serial.println();
  Serial.println(F("definition ok "));
  Serial.println(version_soft);
  Serial.println();

#ifdef EcranOled
  RAAfficheur.setup();
#endif


#ifdef EEprom
  RAPrgEEprom.setup();
#endif

  delay(500);
  marcheForcee = false; // mode forcé retirer au démarrage
  marcheForceePercentage = false;
  marcheForceeTemporisation = 0;
  
#ifdef WifiMqtt
  RAMQTT.setup();
#endif

#ifdef WifiServer
  RAServer.setup(); // activation de la page Web de l'esp
#endif

#ifdef Bluetooth // bluetooth non autorise avec serveur web ou MQTT
  RABluetooth.setup();
#endif

#ifdef EcranOled
  RAAfficheur.setup();
#endif

#ifdef MesureTemperature
  RAMesure.setup();
#endif

#ifdef Pzem04t
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2); // redefinition des broches du serial2
#endif

#ifdef parametrage
  modeparametrage = true;
#endif

#ifdef relaisPWM
  ledcAttachPin(pinRelais, 0); // broche 18, canal 0.
  ledcSetup(0, 5000, 10); // canal = 0, frequence = 5000 Hz, resolution = 10 bits 1024points
#endif


  delay(500);
  RATriac.watchdog(0);       // initialise le watchdog
  RATriac.start_interrupt(); // demarre l'interruption de zerocrossing et du timer pour la gestion du triac

}

int iloop = 0; // pour le parametrage par niveau


void loop()
{
  RATriac.watchdog(1);                  //chien de garde à 4secondes dans timer0
#ifndef simulation
  RAMesure.mesurePinceTension(700, 20); // mesure le courant et la tension avec 2 boucles de filtrage (700,20)
 #ifdef CourantAClimite
  RAMesure.mesurePinceAC(pinPinceAC,0.321,false); // mesure le courant à l'entrée de l'onduleur pour couper le routeur
#endif 
#endif 

#ifdef simulation
  if (!modeparametrage)RASimulation.imageMesure(100); // permet de faire des essais sans matériel
  Serial.print("Mode simulation");
#endif

    RAMesure.mesureTemperature(); // mesure la temperature sur le ds18b20
    RAMesure.mesure_puissance();  // mesure la puissance sur le pzem004t

  if (!modeparametrage)
  {
    int dev = RARegulation.mesureDerive(intensiteDC, 0.3); // donne 1 si ça dépasse l'encadrement haut et -1 si c'est en dessous de l'encadrement (Pince,0.2)
    RARegulation.pilotage();
     if (!marcheForcee) 
        {
 // pour onduleur avec recharge batterie
 #ifdef CourantAClimite
        if (intensiteAC< CourantAClimite)  puissanceGradateur = RARegulation.regulGrad(dev); // calcule l'augmentation ou diminution du courant dans le ballon en fonction de la deviation
            else puissanceGradateur = 0;
 #endif
 #ifndef CourantAClimite
          puissanceGradateur = RARegulation.regulGrad(dev);
 #endif
      
         }
  
  }


  
   if (modeparametrage)
  {
    int potar = map(analogRead(pinPotentiometre), 0, 4095, 0, 1000); // controle provisoire avec pot
        if (potar>10) puissanceGradateur=potar;
       else  if (potar<2) puissanceGradateur=0;
                    else puissanceGradateur=1; // priorité au potensiometre                    
   
  /*  iloop++;
    if (iloop < 10*3) puissanceGradateur = 0;
      else if (iloop < 20*3)  puissanceGradateur = 100;
            else if (iloop < 30*3)    puissanceGradateur = 200;
                  else if (iloop < 40*3) puissanceGradateur = 400;
                        else if (iloop < 50*3) puissanceGradateur = 500;
                            else if (iloop < 60*3) puissanceGradateur = 700;
                               else if (iloop < 70*3) puissanceGradateur = 800;
                                  else if (iloop < 80*3) puissanceGradateur = 900;
                                          else  iloop = 0;*/
 
   Serial.println();
    Serial.print("Courant ");
    Serial.println(intensiteDC);
    Serial.print("tension ");
    Serial.println(capteurTension);
    Serial.print("Gradateur ");
    Serial.println(puissanceGradateur);
    Serial.print("Courant AC  ");
    Serial.println(intensiteAC);
    Serial.print("Puissance de chauffe  ");
    Serial.println(puissanceDeChauffe);
    Serial.println();
     delay(200);
  }

  // affichage traceur serie
  float c = puissanceGradateur;
  Serial.print(" ");
  Serial.print(c / 100);
  Serial.print(',');
  Serial.print(devlente * 2);
  Serial.print(',');
  Serial.print(intensiteDC);
  Serial.print(',');
  Serial.print(routeur.seuilDemarrageBatterie / 5);
  Serial.print(',');
  Serial.print(capteurTension / 5);
  Serial.print(',');
  Serial.println(-routeur.toleranceNegative);
//  Serial.print(',');
//  Serial.print(intensiteAC) );
//  Serial.print(',');
//  Serial.println(puissanceAC/100);

//RAMesure.mesurePinceAC(pinPince,0.321,false);
//puissanceGradateur=300;

#ifdef relaisPWM
  ledcWrite(0, long(puissanceGradateur*1024/1000));  //  canal = 0, rapport cyclique = 
#endif

#ifdef EcranOled
 //  intensiteBatterie=RAMesure.mesurePinceAC(pinPinceAC,1.0467,false); // mesure le courant à l'entrée de l'onduleur pour couper le routeur

  RAAfficheur.affichage_oled(); // affichage de lcd
#endif

#ifdef WifiMqtt
  RAMQTT.loop();
#endif

#ifdef WifiServer
  RAServer.loop();
  RAServer.coupure_reseau();
#endif

#ifdef Bluetooth
      // RABluetooth.Read_bluetooth(); //interface de commande
  // affichage mode traceur serie
  RABluetooth.printBT(String(c / 100));
  RABluetooth.printBT(",");
  RABluetooth.printBT(String(devlente * 2));
  RABluetooth.printBT(",");
  RABluetooth.printBT(String(intensiteDC));
  RABluetooth.printBT(",");
  RABluetooth.printBT(String(routeur.seuilDemarrageBatterie / 5));
  RABluetooth.printBT(",");
  RABluetooth.printBT(String(capteurTension / 5));
  RABluetooth.printBT(",");
  RABluetooth.printBT(String(-routeur.toleranceNegative));
  RABluetooth.printBT("\n");

#endif

#ifdef EEprom
  
  if (resetEsp == 1)
  {
    RATriac.stop_interrupt();
    RAPrgEEprom.close_param(); 
    delay(5000);
  }
#endif

  if (resetEsp == 1)
  {
    Serial.println("Restart !");
    ESP.restart(); // redemarrage
  }
}
