/***************************************/
/******** mode MQTT   ********/
/***************************************/
#include "settings.h"
#ifdef WifiMqtt
#include "modemqtt.h"
#include "modeserveur.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include "WiFi.h"
#include "afficheur.h"
/************ WiFi & MQTT objects  ******************/
WiFiClient espClient;
PubSubClient client(espClient);
int affpub = 0;
int testconnect = 0; //  permet de savoir si la connection est faite

void RAMQTTClass::setup()
{

 
  if (!SAP)
  {
    WiFi.begin(routeur.ssid, routeur.password); // connection au reseau 20 tentatives autorisées
    while ((testconnect < 20) && (WiFi.status() != WL_CONNECTED))
    {
      delay(500);
      Serial.println(F("Connection au WiFi.."));
      testconnect++;
    }
    if (testconnect < 20)
    { // connection réussie
      Serial.println(F("WiFi actif"));
      SAP = false;
      MQTT = true;
      client.setServer(routeur.mqttServer, routeur.mqttPort); // connection au broker mqtt
      client.setCallback(callback);
      Serial.print(F("le broker est  "));
      Serial.println(routeur.mqttServer);
      testconnect = 0;
      while ((testconnect < 5) && (!client.connected()))
      { // connection au broker 5 tentatives autorisée
        testconnect++;
        Serial.print(F("Connection au broker MQTT..."));
        if (client.connect("ESP32Client", routeur.mqttUser, routeur.mqttPassword))
        {
          Serial.println(F("connection active"));
   #ifdef EcranOled
    RAAfficheur.cls();
    RAAfficheur.affiche(20,"Brokeur ok");
    RAAfficheur.affiche(35,String(testconnect));delay(500);
  #endif       
       }
        else
        {
          Serial.println(F("Erreur de connection "));
  #ifdef EcranOled
    RAAfficheur.cls();delay(100);
    RAAfficheur.affiche(20,"Connection");
    RAAfficheur.affiche(35,String(testconnect));delay(500);
  #endif       
        }
 //       Serial.print(client.state());
     delay(100);
      }
      if (testconnect >= 5) // connection échouée au broker
      {
        Serial.println(F("Broker Mqtt indisponible"));
  #ifdef EcranOled
    RAAfficheur.cls();delay(100);
    RAAfficheur.affiche(20,"Err Brokeur");
    RAAfficheur.affiche(35,String(testconnect));delay(1000);
  #endif       
        MQTT = false; // pas de connection mqtt
      }
    }
    else
    {
      Serial.println(F("Connection au WiFi absente .... passage en mode serveur"));
      SAP = true; // mode poit d'accès activé
    }
  }
  paramchange = 1; // communication au format mqtt pour le reseau domotique 1 pour l'envoi des parametres
}

/************************* passage de parametre par BT ou MQTT  *********************************/


void RAMQTTClass::callback(char *topic, byte *message, unsigned int length)
{
  Serial.print(F("Message arrived on topic: "));
  Serial.print(topic);
  Serial.print(F(". Message: "));

  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
  if (String(topic) == routeur.mqttopicInput)
  {
    RAServer.commande_param(messageTemp);
  }
  // Mqtt_publish(1);
}

void RAMQTTClass::mqtt_subcribe()
{
  if (client.connected())
  {
    client.subscribe(routeur.mqttopicInput);
  }
  client.loop();
}

void RAMQTTClass::mqtt_publish(int a)
{

  if (a > 0)
    affpub = 30; // forcage de l'envoie
  affpub++;
  if (affpub < 20)
    return;
  else
    affpub = 0;

  const int capacity = JSON_OBJECT_SIZE(5); // 5 données maxi dans json
  StaticJsonDocument<capacity> doc;
  char buffer[200];
  //Exportation des données en trame JSON via MQTT
  doc["Intensite"] = intensiteDC;
  doc["Tension"] = capteurTension;
  doc["Gradateur"] = puissanceGradateur;
  doc["Temperature"] = temperatureEauChaude;
  doc["puissanceMono"] = puissanceDeChauffe;
  size_t n = serializeJson(doc, buffer); // calcul de la taille du json
  buffer[n - 1] = '}';   buffer[n] = 0; // fermeture de la chaine json
  Serial.println(buffer);
  if (client.connected())
  {
    if (client.publish(routeur.mqttopic, buffer, n) == true)     {      Serial.println(F("Success sending message"));
    }    else    {      Serial.println(F("Error sending message"));    }
  }
   doc.clear();

  client.loop();
  if (a == 0)
    return;

  //   char buffer[150];
  //Exportation des données en trame JSON via MQTT
  StaticJsonDocument<capacity> doc2;
  doc2["zeropince"] = routeur.zeropince;
  doc2["coeffPince"] = routeur.coeffPince;
  doc2["coeffTension"] = routeur.coeffTension;
  doc2["seuilTension"] = routeur.seuilDemarrageBatterie;
  doc2["toleranceNeg"] = routeur.toleranceNegative;
  n = serializeJson(doc2, buffer); // calcul de la taille du json
  buffer[n - 1] = '}';
  buffer[n] = 0; // fermeture de la chaine json
  Serial.println(buffer);
  if (client.connected())   {
    if (client.publish(routeur.mqttopicParam1, buffer, n) == true)     {       Serial.println(F("Success sending message param"));
    }     else     {       Serial.println(F("Error sending message param"));     }
  }
   doc2.clear();

  StaticJsonDocument<capacity> doc3;
  doc3["sortie2"] = routeur.utilisation2Sorties;
  doc3["sortie2_tempHaut"] = routeur.temperatureBasculementSortie2;
  doc3["sortie2_tempBas"] = routeur.temperatureRetourSortie1;
  doc3["temporisation"] = routeur.marcheForceeTemporisation;
  doc3["sortieActive"] = sortieActive;
  n = serializeJson(doc3, buffer); // calcul de la taille du json
  buffer[n - 1] = '}';  buffer[n] = 0; // fermeture de la chaine json
  Serial.println(buffer);
  if (client.connected())
  {
    if (client.publish(routeur.mqttopicParam2, buffer, n) == true)    {       Serial.println(F("Success sending message param"));
    }     else     {       Serial.println(F("Error sending message param"));     }
  }
   doc3.clear();
  
  StaticJsonDocument<capacity> doc4;
  doc4["sortieRelaisTemp"] = (routeur.relaisStatique && (routeur.tensionOuTemperature[0]=='D'));
  doc4["sortieRelaisTens"] = (routeur.relaisStatique && (routeur.tensionOuTemperature[0]=='V'));
  doc4["relaisMax"] = routeur.seuilMarche;
  doc4["relaisMin"] = routeur.seuilArret;
  doc4["Forcage_1h"] = marcheForcee;

  
  n = serializeJson(doc4, buffer); // calcul de la taille du json
  buffer[n - 1] = '}';
  buffer[n] = 0; // fermeture de la chaine json
  Serial.println(buffer);
  if (client.connected())
  {
    if (client.publish(routeur.mqttopicParam3, buffer, n) == true)
    {
      Serial.println(F("Success sending message param"));
    }
    else
    {
      Serial.println(F("Error sending message param"));
    }
  }
   doc4.clear();

  client.loop();

  //  sortie du pzem
#ifdef Pzem04t

  StaticJsonDocument<capacity> doc5; // ajout d'un json pour le Pzem
                                     //  char buffer[400];
  //Exportation des données en trame JSON via MQTT
  doc5["Intensite"] = Pzem_i;
  doc5["Tension"] = Pzem_U;
  doc5["Puissance"] = Pzem_P;
  doc5["Energie"] = Pzem_W;
  doc5["Cosf"] = 0;

  n = serializeJson(doc5, buffer); // calcul de la taille du json
  buffer[n - 1] = '}';
  buffer[n] = 0; // fermeture de la chaine json
  Serial.println(buffer);
  if (client.connected())
  {
    if (client.publish(routeur.mqttopicPzem1, buffer, n) == true)
    {
      Serial.println(F("Success sending message"));
    }
    else
    {
      Serial.println(F("Error sending message"));
    }
  }
   doc5.clear();

  /********************* fin d'envoie ***********************/
  client.loop();
#endif
}

void RAMQTTClass::loop()
{
  if ((!SAP) && (WiFi.status() != WL_CONNECTED))  { resetEsp = 1; return; }
  if ((testwifi == 0) && (!SAP)  && (!serverOn))
  {
//    if ((WiFi.status() == WL_CONNECTED) && (client.connected()))
    if (WiFi.status() == WL_CONNECTED) 
    {                  // teste le wifi
      bool mqttav=MQTT;
      if (!client.connected()) MQTT=false; 
      if (MQTT!=mqttav) testwifi++;
      if(MQTT) mqtt_subcribe(); // lecture de information sur mqtt output/solar
                       //  (1) avec les parametres
      //  (0) seulement les indispensables tension courant ..
      if (paramchange == 1)
      {
        if(MQTT) mqtt_publish(1);
      }
      else
      {
        if(MQTT) mqtt_publish(0); // communication au format mqtt pour le reseau domotique
      }
      MQTT=mqttav;
      paramchange = 0;
    }
    else
      testwifi++;
  }

  if (testwifi != 0)
  {
    Serial.print(F("Perte de connexion redemarrage en cours "));
    Serial.println(500 - testwifi);
    testwifi++;
    if (testwifi > 500)
      resetEsp = 1; // si perte de signal trop longtemps reset esp32
    if (client.connected()) testwifi=0;
  }

  
}

RAMQTTClass RAMQTT;
#endif
