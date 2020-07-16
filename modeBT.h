/***************************************/
/******** mode bluetooth   ********/
/***************************************/
#ifndef RA_BLUETOOTH_H
#define RA_BLUETOOTH_H
#include <Arduino.h>


class RABluetoothClass
{
public:
  void setup();
  void BT_commande();
  void BT_visu_param();
  void read_bluetooth();
  void printBT(String mesg);
};

extern RABluetoothClass RABluetooth;
#endif
