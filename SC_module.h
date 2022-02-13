#ifndef __SC_MODULE_H__
#define __SC_MODULE_H__

#include <inttypes.h>
#include <avr/io.h>
#include <avr/pgmspace.h>


class SerialCommander;

typedef void (*fctSC_Cmde)(SerialCommander*);

void SC_moduleFct(SerialCommander*);

struct SC_module{
  const char* PROGMEM nom;
  const char* const* PROGMEM listeCmde;
  const uint8_t tailleListe;
  fctSC_Cmde fct;
};

const char* SC_mod_Nom(const SC_module* module PROGMEM);
uint8_t SC_mod_TailleListe(const SC_module* module PROGMEM);
const char* SC_mod_Cmde(const SC_module* module PROGMEM, uint8_t n);
fctSC_Cmde SC_mod_Fct(const SC_module* module PROGMEM);


#endif
