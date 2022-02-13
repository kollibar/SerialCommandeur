#ifndef __SERIALCOMMANDEUR_H__
#define __SERIALCOMMANDEUR_H__

#include "SerialCommandeur_TXT.h"
#include "SC_module.h"

#include <inttypes.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include <Stream.h>
#include <Arduino.h>

#define SERIAL_COMMANDEUR_TAILLE_TAMPON 40

#define DEBUG_SC


#define SC_BIT_GUILLEMET    0x01
#define SC_BIT_SLASH        0x02



#define SC_STATUT_GUILLEMET   0b10000000
#define SC_STATUT_SLASH       0b01000000
#define SC_STATUT_ERREUR      0b00100000
#define SC_STAUT_HORS_ARG     0b00010000

#define SC_STATUT_BITS_SPE  ( SC_STATUT_SLASH | SC_STATUT_GUILLEMET )

#define SC_STATUT_ERREUR_OVERFLOW               ( SC_STATUT_ERREUR | 0x01)
#define SC_STATUT_ERREUR_CARACTERE_NON_ACCEPTE  ( SC_STATUT_ERREUR | 0x02)
#define SC_STATUT_ERREUR_MODULE_INCONNU         ( SC_STATUT_ERREUR | 0x03)
#define SC_STATUT_ERREUR_CMDE_INCONNUE          ( SC_STATUT_ERREUR | 0x04)

#define SC_STATUT_INIT        ( SC_STAUT_HORS_ARG | 0x00 )
#define SC_STATUT_MODULE      ( SC_STAUT_HORS_ARG | 0x01 )
#define SC_STATUT_CMDE        ( SC_STAUT_HORS_ARG | 0x02 )

#define SC_CMDE_INCONNUE  0xFF
#define SC_MODULE_INCONNU  0xFF



struct dataFctCherche{
  const SC_module* PROGMEM module;
  const char * PROGMEM cmde;
  uint8_t startNext;
};


class SerialCommander : public Stream{
public:
  SerialCommander(HardwareSerial* serial, const SC_module* listeModule, uint8_t nbModule);
  void check();

  const SC_module* getModule();
  const char* getCmde();
  fctSC_Cmde getFct();

  uint8_t getNbArgs();

  char* getArg(uint8_t n);

  void printPGM(const char* PROGMEM);
  void printlnPGM(const char* PROGMEM);

  #ifdef DEBUG_SC
  uint8_t getUtilisationMaxTampon(){ return this->utilisationMaxTampon;};
  void printDebug();
  #endif

  inline int available(){return this->serial->available();};
  inline int read(){return this->serial->read();};
  inline int peek(){return this->serial->peek();};
  inline void flush(){return this->serial->flush();};
  inline size_t write(uint8_t c){return this->serial->write(c);};

protected:
  void execute();

  dataFctCherche chercheModule();
  dataFctCherche chercheCmde(dataFctCherche data);
  uint8_t calculNbArgs();

  void supprDeTampon(uint8_t pos, uint8_t taille=1);

private:
  char tampon[SERIAL_COMMANDEUR_TAILLE_TAMPON]; // tampon pour recevoir la ligne de commande
  uint8_t pos;  // position courante dans le tampon
  uint8_t statut_spe; // statut des caractères spéciaux entre 2 appel de la fonction check ( slash, guillemet, ...)
  SC_module* lastModule; // dernier module appellé

  const SC_module* listeModule;
  uint8_t nbModule;
  HardwareSerial* serial;

  #ifdef DEBUG_SC
  uint8_t utilisationMaxTampon;  // utilisation maximum du tampon
  #endif
};


#endif
