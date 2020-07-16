/***************************************/
/******** mode Serveur   ********/
/***************************************/
#include "settings.h"
#ifdef WifiServer
#include "modeserveur.h"
#include "triac.h"
#include "prgEEprom.h"
#include "afficheur.h"
#include <string.h>

#include <SPIFFS.h>

#ifdef WifiSAPServeur
bool wifiSAP = true;
#else
bool wifiSAP = false;
#endif

int pause_inter = 0;
WiFiServer server(80);
IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);
bool restartEsp = false;


void RAServerClass::setup()
{

#ifndef WifiMqtt
    int testconnect = 0;
    if (!SAP)
    {
        WiFi.begin(routeur.ssid, routeur.password); // connection au reseau 20 tentatives autorisées
        while ((testconnect < 20) && (WiFi.status() != WL_CONNECTED))
        {
            delay(500);
            Serial.println(F("Connection au WiFi.."));
            testconnect++;
        }
        if (testconnect >= 20)
            SAP = true;
        else
        {
            Serial.println("\n");
            Serial.println("Connexion etablie!");
            Serial.print("Adresse IP: ");
            Serial.println(WiFi.localIP());
        }
    }
#endif
    if (SAP)
    {
        const char *ssid = "routeur_esp32"; // mode point d'accès
        const char *password = "adminesp32";
        WiFi.enableAP(true);
        delay(100);
        WiFi.softAP(ssid, password); // l'esp devient serveur et se place à l'adresse 192.168.4.1
        WiFi.softAPConfig(local_ip, gateway, subnet);
        IPAddress myIP = WiFi.softAPIP();
        Serial.print("AP IP address: ");
        Serial.println(myIP);
    }

    if (!SPIFFS.begin())
    {
        Serial.println("Erreur SPIFFS...");
        return;
    }
    server.begin();
}


void RAServerClass::commande_param(String mesg)
{
  Serial.println(mesg);
  if (mesg == "rst")
    resetEsp = 1;
  if (mesg.substring(0, 3).equals("bat")) // reception ex: "bat51" 51 est converti en seuil batterie
  {
    int com = mesg.substring(3).toInt();
    routeur.seuilDemarrageBatterie = com;
  }
  if (mesg.substring(0, 3).equals("neg")) // reception ex: "neg0.6" autorise un seuil de démarrage
  {
    float com = mesg.substring(3).toFloat();
   routeur.toleranceNegative = com;
  }

  if (mesg.substring(0, 3).equals("zpi")) // reception ex: "zpi0.6" zero pince
  {
    float com = mesg.substring(3).toFloat();
   routeur.zeropince = com;
     Serial.print("pince=");
     Serial.println(routeur.zeropince);
  }
  if (mesg.substring(0, 3).equals("cop")) // reception ex: "zpi0.6" zero pince
  {
   float com = atof(mesg.substring(3).c_str());
   routeur.coeffPince = com;
  }
  if (mesg.substring(0, 3).equals("cot")) // reception ex: "zpi0.6" zero pince
  {
   float com = atof(mesg.substring(3).c_str());
    Serial.println(com*1000);
  routeur.coeffTension = com;
  }
  
  if (mesg.substring(0, 3).equals("sth")) // reception ex: "sth51" 51 seuil de temperature haute
  {
    int com = mesg.substring(3).toInt();
    routeur.temperatureBasculementSortie2 = com;
  }
  if (mesg.substring(0, 3).equals("stb")) // reception ex: "stb51" 51 seuil de temperature basse
  {
    int com = mesg.substring(3).toInt();
    routeur.temperatureRetourSortie1 = com;
  }
  if (mesg.substring(0, 3).equals("rth")) // reception ex: "rth51" 51 relais tension haut
  {
    int com = mesg.substring(3).toInt();
    routeur.seuilMarche = com;
  }
  if (mesg.substring(0, 3).equals("rtb")) // reception ex: "rtb51" 51 relais tension bas
  {
    int com = mesg.substring(3).toInt();
    routeur.seuilArret = com;
  }

#ifdef Sortie2
  if (mesg.substring(0, 3).equals("sor")) // reception ex: "sor1"  ou "sor0"  pour la commande du 2eme gradateur
  {
    int com = mesg.substring(3).toInt();
    if (com == 1)
      routeur.utilisation2Sorties = true;
    else
      routeur.utilisation2Sorties = false;
  }
#endif

  if (mesg.substring(0, 3).equals("cmf")) // reception ex: "cmf1"  ou "cmf0"  pour la commande de la marche forcee
  {
    int com = mesg.substring(3).toInt();
    if (com == 1)
      marcheForcee = true;
    else
      marcheForcee = false;
  }
  if (mesg.substring(0, 3).equals("rmf")) // reception ex: "rmf25" 25% ratio marche forcée
  {
    int com = mesg.substring(3).toInt();
    Serial.println(com);
    marcheForceePercentage = com;
  }
  if (mesg.substring(0, 3).equals("rel")) // reception ex: "rel0" le relais température et tension à zéro rel1 pour temp rel2 tension
  {
    int com = mesg.substring(3).toInt();
    if (com == 0)
    {
      strcpy(routeur.tensionOuTemperature, "");
        routeur.relaisStatique = false;
    }
    if (com == 1)
    {
      strcpy(routeur.tensionOuTemperature, "D");
        routeur.relaisStatique = true;

    }
    if (com == 2)
    {
      strcpy(routeur.tensionOuTemperature, "V");
        routeur.relaisStatique = true;
    }
  }
  if (mesg.substring(0, 3).equals("tmp")) // reception ex: "tmp60" 60 minute de marche forcée
  {
    int com = mesg.substring(3).toInt();
    marcheForceeTemporisation = com;
  }
  paramchange = 1;
}



int RAServerClass::hexa(char a){
  if ((a>='0') && (a<='9')) return(int (a-'0'));
  if ((a>='A') && (a<='F')) return(int (a-'A')+10);
  return(-1);
}

String RAServerClass::convert(String a){
     String b="";
     int pos=a.indexOf('%');
      while (pos!=-1){
        int icar=0;
        b="%";
        b+=a.charAt(pos+1);icar=16*hexa(a.charAt(pos+1));
        b+=a.charAt(pos+2);icar+=hexa(a.charAt(pos+2));
        String c=""; c+=char(icar);
        a.replace(b,c);
        pos=a.indexOf('%');
      }
  return(a);
}


String RAServerClass::commande_web(String comm){                                           // recupération des commandes du site web
//   String st=comm.substring(6);
   String st=comm;
   String com="";
 // if (comm.substring(0, 6).equals("GET / ")) st="/index.html";
 //  st.replace(" HTTP/1.1","");
   
 //  Serial.print("Serveur fichier");Serial.println(st);
  com="defaut";
  if (st.substring(0, com.length()).equals(com)) {RAPrgEEprom.defaut_param();  }
  com="reset";
  if (st.substring(0, com.length()).equals(com)) { RAPrgEEprom.sauve_param(); RAPrgEEprom.close_param();  }

  com="sortie2=";
  if (st.substring(0, com.length()).equals(com)) { st="sor"+st.substring(com.length()); commande_param(st);  }

  com="relais=";
  if (st.substring(0, com.length()).equals(com)) { st="rel"+st.substring(com.length()); commande_param(st);  }
 
  com="forcage=";
  if (st.substring(0, com.length()).equals(com)) { st="cmf"+st.substring(com.length()); commande_param(st);  }

  com="duree-forcage=";
  if (st.substring(0, com.length()).equals(com)) { st="tmp"+st.substring(com.length()); commande_param(st);  }
  
  com="valeur-forcage=";
  if (st.substring(0, com.length()).equals(com)) { st="rmf"+st.substring(com.length()); commande_param(st);  }

  com="zeroPince=";
  if (st.substring(0, com.length()).equals(com)) { st="zpi"+st.substring(com.length()); commande_param(st);  }

  com="coeffPince=";
  if (st.substring(0, com.length()).equals(com)) { st="cop"+st.substring(com.length()); commande_param(st);  }

  com="coeffTension=";
  if (st.substring(0, com.length()).equals(com)) { st="cot"+st.substring(com.length()); commande_param(st);  }
  
  com="Tolerance-neg=";
  if (st.substring(0, com.length()).equals(com)) { st="neg"+st.substring(com.length()); commande_param(st);  }
 
  com="dem-Batterie=";
  if (st.substring(0, com.length()).equals(com)) { st="bat"+st.substring(com.length()); commande_param(st);  }

  com="TempHaut=";
  if (st.substring(0, com.length()).equals(com)) { st="sth"+st.substring(com.length()); commande_param(st);  }
  
  com="TempBas=";
  if (st.substring(0, com.length()).equals(com)) { st="stb"+st.substring(com.length()); commande_param(st);  }
  
  com="relaisvalmax=";
  if (st.substring(0, com.length()).equals(com)) { st="rth"+st.substring(com.length()); commande_param(st);  }
  
  com="relaisvalmin=";
  if (st.substring(0, com.length()).equals(com)) { st="rtb"+st.substring(com.length()); commande_param(st);  }
  
   com="reset=";
  if (st.substring(0, com.length()).equals(com)) { st="rst"; }
 
   com="ssid=";
  if (st.substring(0, com.length()).equals(com)) {
          int pos=st.indexOf('&');
          String stssid=st.substring(com.length(),pos);
          Serial.println(stssid);
          st=st.substring(pos+1);
          st.replace("ssidpassword=","");
          pos=st.indexOf('&');
          String stssidpwd=st.substring(0,pos);
          Serial.println(stssidpwd);
          st="reboot pour sauvegarde";resetEsp =1;
          for (int i=0;i<stssid.length()+1;i++) routeur.ssid[i]=stssid.charAt(i);
          for (int i=0;i<stssidpwd.length()+1;i++) routeur.password[i]=stssidpwd.charAt(i);
          paramchange = 1;
          }

//GET /?broker=192.168.1.28&brokerpassword=azerty&topic=routeur&enreg=Enregistrer HTTP/1.1
//      broker=192.168.1.28&brokeruser=&brokerpassword=&topic=routeurb&enreg=Enregistrer
   com="broker=";
  if (st.substring(0, com.length()).equals(com)) {
          int pos=st.indexOf('&');
          String stbroker=st.substring(com.length(),pos);
          Serial.println(stbroker);
          st=st.substring(pos+1);
          st.replace("brokeruser=","");
          pos=st.indexOf('&');
          String stuser=st.substring(0,pos);
          Serial.println(stuser);
          st=st.substring(pos+1);
          st.replace("brokerpassword=","");
          pos=st.indexOf('&');
          String stbrokerpwd=st.substring(0,pos);
          Serial.println(stbrokerpwd);
          st=st.substring(pos+1);
          st.replace("topic=","");
          pos=st.indexOf('&');
          String sttopic=st.substring(0,pos);
          Serial.println(sttopic);
          
          st=sttopic+"/solar"; 
          for (int i=0;i<st.length()+1;i++) routeur.mqttopic[i]=st.charAt(i);
          st=sttopic+"/isolar";
          for (int i=0;i<st.length()+1;i++) routeur.mqttopicInput[i]=st.charAt(i);
          st=sttopic+"/solar1";
          for (int i=0;i<st.length()+1;i++) routeur.mqttopicParam1[i]=st.charAt(i);
          st=sttopic+"/solar2";
           for (int i=0;i<st.length()+1;i++) routeur.mqttopicParam2[i]=st.charAt(i);
          st=sttopic+"/solar3";
          for (int i=0;i<st.length()+1;i++) routeur.mqttopicParam3[i]=st.charAt(i);
          st=sttopic+"/Pzem1";
          for (int i=0;i<st.length()+1;i++) routeur.mqttopicPzem1[i]=st.charAt(i);
          st="reboot pour sauvegarde";resetEsp =1;paramchange = 1;
           }
                      
   
   return(st);
}




String RAServerClass::refresh(String id, String val,int typ){          // insert javascript dans la page index.html
  String com="\ndocument.getElementById('";                             // typ=1 pour du texte
  com+=id; com+="').value = ";
  if (typ==1) com+="\"";
  com+=val;
  if (typ==1) com+="\""; 
  com+=";";
  return(com);
}



String RAServerClass::affichage_web(int a){
  String info="";
  char *buf="                         ";
  if (a==0){
  info+="Le courant dans la batterie est de : ";
  info+=String(intensiteDC) +" A <br><br>";

  info+="La tension de la batterie est de : ";
  info+=String(capteurTension) +" V <br><br>";

  info+="La Puissance dans le chauffe-eau est de : ";
  info+=String(puissanceDeChauffe) +" W <br><br>";

  info+="La température de l'eau est de : ";
  info+=String(temperatureEauChaude) +" degrés celcius<br><br>";

  info+="Le gradateur est ouvert à ";
  info+=String(puissanceGradateur/10) +" % <br><br>";
  
  //info+="Les autres parametres sont ";
  //info+="Coeff pince : "+ String(routeur.coeffPince) +"  <br><br>";

  }
  else {
 
    info+=refresh("sortie2",String(routeur.utilisation2Sorties),0); //nombre
    info+=refresh("broker",routeur.mqttServer,1);        //string
    info+=refresh("ssid",routeur.ssid,1);        //string
    info+=refresh("relais","0",1);        //string
    info+=refresh("forcage",String(marcheForcee),0); //nombre
    info+=refresh("duree-forcage",String(marcheForceeTemporisation),0); //nombre
    info+=refresh("valeur-forcage",String(marcheForceePercentage),0); //nombre
    info+=refresh("zeroPince",String(routeur.zeropince),0); //nombre
    info+=refresh("coeffPince",String(routeur.coeffPince,5),0); //nombre
   info+=refresh("coeffTension",String(routeur.coeffTension,5),0); //nombre
    info+=refresh("Tolerance-neg",String(routeur.toleranceNegative),0); //nombre
    info+=refresh("dem-Batterie",String(routeur.seuilDemarrageBatterie),0); //nombre
 
    info+=refresh("TempHaut",String(routeur.temperatureBasculementSortie2),0); //nombre
    info+=refresh("TempBas",String(routeur.temperatureRetourSortie1),0); //nombre
   info+=refresh("relaisvalmax",String(routeur.seuilMarche),0); //nombre
    info+=refresh("relaisvalmin",String(routeur.seuilArret),0); //nombre
   info+=refresh("brokeruser",routeur.mqttUser,1);        //string
   info+=refresh("brokerpassword",routeur.mqttPassword,1);        //string
    String st="";
    for (int i=0;i<20;i++) if (routeur.mqttopic[i]=='/') i=40; else st+=routeur.mqttopic[i]; 
    info+=refresh("topic",st,1);        //string
 
  }
 return(info);
}



void RAServerClass::loop()
{
    if ((!SAP) && (WiFi.status() != WL_CONNECTED))
    {
        resetEsp = 1;
        return;
    }
    char car;
    String lect="";
    WiFiClient client = server.available(); // listen for incoming clients
    if (client)
    {
        String req = client.readStringUntil('\r'); // lecture la commande
        Serial.println(req);
        String content = "";
        if (req.length() > 0)
        {
           RATriac.stop_interrupt();
           pause_inter = 1;
     
             File serverFile;
            if (req.startsWith("POST"))
            {
                boolean currentLineIsBlank = false;
                String requestBody = "{\"";
                while (client.available())
                {
                    char c = client.read();
                    if (c == '\n' && currentLineIsBlank)
                    {
                        while (client.available())
                        {
                            c = client.read();
                        lect+=c;
                        if (c == '=')
                            {
                                requestBody += "\":\"";
                            }
                            else if (c == '&')
                            {
                                requestBody += "\",\"";
                            }
                            else
                            {
                                requestBody += c;
                            }
                        }
                    }
                    else if (c == '\n')
                    {
                        currentLineIsBlank = true;
                    }
                    else if (c != '\r')
                    {
                        currentLineIsBlank = false;
                    }
                }
                requestBody += "\"}";
                requestBody.replace("%2F", "/");
                requestBody=convert(requestBody);
                Serial.println(requestBody);
                Serial.println(lect);
                commande_web(lect);
                lect="";

                client.println("HTTP/1.1 204 OK");
            }
            else
            {
              if (req.indexOf(".css") > 0 || req.indexOf(".js") > 0)
                {
                    String tmp = req.substring(4);
                    String filePath = tmp.substring(0, tmp.length() - 9);
                    serverFile = SPIFFS.open(filePath, "r"); //Ouverture fichier pour le lire
                }
                else
                {
                    serverFile = SPIFFS.open("/index.html", "r"); //Ouverture fichier pour le lire
                }
            }

            if (serverFile)
            {
                //client.println("HTTP/1.1 200 OK");
                for (int i = 0; i < serverFile.size(); i++)
                {
                    //content += (char)serverFile.read();
                    car=(char)serverFile.read();
                    if (car=='~')  lect+=affichage_web(0); else  
                      if (car=='|')  lect+=affichage_web(1); 
                                else {
                                       lect+=car; 
                                       if (lect.length()>100) // limite la taille de la string
                                         {
                                            client.print(lect);
                                            lect="";
                                          }
                                                                   }
                  //Serial.print(car);    //Read file
 
                }
                client.println(lect);                                          // envoie l'index.html dans le navigateur               
                serverFile.close();

              if (req.indexOf(".js") > 0)
                {
                    client.println("Content-Type: text/javascript");
                }
                else if (req.indexOf(".css") > 0)
                {
                    client.println("Content-Type: text/css");
                }
                client.println();
                //client.println(content);
             }
        }
        // close the connection:
        client.stop();
    }
    if (restartEsp)
    {
        delay(200);
        resetEsp = 1;
    }
    if (pause_inter > 0)
    {
        pause_inter++;
        Serial.print("Serveur en cours ");
        Serial.println(pause_inter);
    }
    if ((pause_inter > 3) && (!SAP))
    {
        pause_inter = 0;
        RATriac.restart_interrupt();
      }
    if ((pause_inter > 20) && (SAP))
    {
        pause_inter = 0;
        RATriac.restart_interrupt();
    }
}

int resloop = 0;
void RAServerClass::coupure_reseau()
{
    if ((!wifiSAP) && (SAP))
    {
        resloop++;
        if (resloop > 40)
        {
            Serial.println(F("Attente de reseau "));
            WiFi.disconnect();
            int n = WiFi.scanNetworks();
            Serial.println("scan done");
            if (n == 0)
            {
                Serial.println("no networks found");
            }
            else
            {
                Serial.print(n);
                Serial.println(" networks found");
                for (int i = 0; i < n; ++i)
                {
                    if (WiFi.SSID(i) == routeur.ssid)
                        resetEsp = 1;
                    delay(10);
                }
            }
            resloop = 0;
#ifdef EcranOled
            RAAfficheur.cls();
            RAAfficheur.affiche(20, "WIFI");
            RAAfficheur.affiche(35, "Deconnecte");
#endif
        }
    }
}

RAServerClass RAServer;
#endif
