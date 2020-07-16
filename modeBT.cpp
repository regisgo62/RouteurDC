/***************************************/
/******** mode bluetooth   ********/
/***************************************/
#include "settings.h"
#ifdef Bluetooth
#include "modeBT.h"
#include "triac.h"
#include "modemqtt.h"
#include <Arduino.h>
#include <BluetoothSerial.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;
int aff_visubt = 0;
int pause_inter_bt = 0;

void RABluetoothClass::setup()
{
  SerialBT.begin("routeurWemos"); //Bluetooth device name
  Serial.println(F("Démarrage du BT appairage en attente"));
}

void RABluetoothClass::BT_commande()
{
  if (!SerialBT.available()) return;
  String comm = "";
  while (SerialBT.available())
  {
    comm += char(SerialBT.read());
  }
  if (comm.length() > 0)
    RAMQTT.commande_param(comm);
  delay(20);
}

int iparam = 0;

void RABluetoothClass::BT_visu_param()
{
  iparam++;
  String comm = "";
  if (iparam == 1)
  {
    comm += "I= ";
    comm += String(intensiteBatterie);
  }
  if (iparam == 2)
  {
    comm += "U= ";
    comm += String(capteurTension);
  }
  if (iparam == 3)
  {
    comm += "T= ";
    comm += String(temperatureEauChaude);
  }
  if (iparam == 4)
  {
    comm += "P= ";
    comm += String(puissanceDeChauffe);
  }

  if (iparam == 5)
  {
    comm += "R1= ";
    comm += String(routeur.seuilDemarrageBatterie);
  }
  if (iparam == 6)
  {
    comm += "R2= ";
    comm += String(routeur.toleranceNegative);
  }
  if (iparam == 7)
  {
    comm += "R3= ";
    comm += String(routeur.temperatureBasculementSortie2);
  }
  if (iparam == 8)
  {
    comm += "R4= ";
    comm += String(routeur.temperatureRetourSortie1);
  }
  if (iparam == 9)
  {
    comm += "R5= ";
    comm += String(routeur.seuilMarche);
  }
  if (iparam == 10)
  {
    comm += "R6= ";
    comm += String(routeur.seuilArret);
  }
  if (iparam == 11)
  {
    comm += "R7= ";
    comm += String(marcheForceePercentage);
  }
  if (iparam == 12)
  {
    comm += "R8= ";
    comm += String(temporisation);
  }
  if (iparam == 13)
  {
    comm += "R9= ";
    comm += String(sortieActive);
  }

  if (iparam == 14)
  {
    comm += "S1= ";
    comm += String(routeur.utilisation2Sorties);
  }
  if (iparam == 16)
  {
    comm += "S2= ";
    if (routeur.relaisStatique && (routeur.tensionOuTemperature[0]=='D'))  comm += '1'; else comm += '0';
  }
  if (iparam == 15)
  {
    comm += "S3= ";
    comm += String(marcheForcee);
  }
  if (iparam == 17)
  {
    comm += "S4= ";
     if (routeur.relaisStatique && (routeur.tensionOuTemperature[0]=='V'))  comm += '1'; else comm += '0';
   }

  for (int i = 0; i < comm.length(); i++)
    SerialBT.write(comm.charAt(i));
  SerialBT.write('\x0A');
  if (iparam == 17)
    iparam = 0;
}

void RABluetoothClass::read_bluetooth()
{
  if (SerialBT.available())
    aff_visubt = 1;
  else
    aff_visubt = 0;
   Serial.println("Synchro Bluetooth");
  RATriac.stop_interrupt();
  // delay(10);
  if (aff_visubt == 1) BT_commande();
  for (int i = 1; i < 17; i++)
  {
    BT_visu_param();
  }
  RATriac.restart_interrupt();
}

void RABluetoothClass::printBT(String mesg){
//  if (!SerialBT.available())  return;
  for (int i = 0; i < mesg.length(); i++)
    SerialBT.write(mesg.charAt(i));
}


RABluetoothClass RABluetooth;
#endif
