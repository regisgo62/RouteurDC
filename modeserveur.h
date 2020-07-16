#ifndef RA_MODE_SERVEUR_H
#define RA_MODE_SERVEUR_H
/***************************************/
/******** mode Serveur   ********/
/***************************************/
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <WiFi.h>
class RAServerClass
{

public:
    void setup();
    void loop();
    void coupure_reseau();


private:
    void commande_param(String mesg);
    String commande_web(String comm);                                           // recupération des commandes du site web
    int  hexa(char a);
    String convert(String a);
    String refresh(String id, String val,int typ);         // insert javascript dans la page index.html
    String affichage_web(int a);


};

extern RAServerClass RAServer;
#endif
