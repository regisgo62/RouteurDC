/***************************************/
/******** Programme simulation   ********/
/***************************************/
#include "settings.h"
#ifdef simulation
#include "simulation.h"
#include <Arduino.h>

int temps = 0;
int itemps = 0;
#define ond 300

void RASimulationClass::imageMesure(int tempo)
{
  #define ta 2
  if (temps < 50*ta)
    intensiteBatterie = (-10 + 1.0 * random(ond / 2) / 10) / 10;
  if ((temps >= 50*ta) && (temps < 150*ta))
  {
    itemps++;
    intensiteBatterie = (100 - itemps + 1.0 * random(ond) / 10) / 10;
  }
  if ((temps >= 150*ta) && (temps < 250*ta))
  {
    itemps++;
    intensiteBatterie = (itemps + 1.0 * random(ond) / 10) / 10;
  }
  if ((temps >= 250*ta) && (temps < 350*ta))
    intensiteBatterie = (100 + 1.0 * random(ond / 2) / 10) / 10;
  if ((temps >= 350*ta) && (temps < 450*ta))
    intensiteBatterie = (50 + 1.0 * random(ond / 2) / 10) / 10;
  if ((temps >= 450*ta) && (temps < 550*ta))
    intensiteBatterie = (-100 + 1.0 * random(ond / 2) / 10) / 10;
  temps++;
  if (temps > 550*ta)
  {
    temps = 0;
    itemps = 0;
  }
  delay(tempo);
  capteurTension = (57.0 * 100 + random(100)) / 100;
  temperatureEauChaude = (20.0 * 100 + random(100)) / 100;
  puissanceDeChauffe = (10 * puissanceDeChauffe + random(1000)) / 11;

  //  capteurTemp=25;
}

RASimulationClass RASimulation;
#endif
