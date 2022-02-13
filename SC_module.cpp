#include "SC_module.h"


/*
struct SC_module{
  const char* PROGMEM nom;
  const char* const* PROGMEM listeCmde;
  const uint8_t tailleListe;
  fctSC_Cmde fct;
};*/

const char* SC_mod_Nom(const SC_module* module PROGMEM){
  return (const char*)pgm_read_word( (uint16_t)module );
}
uint8_t SC_mod_TailleListe(const SC_module* module PROGMEM){
  return pgm_read_byte( ((uint16_t)module) + 2*sizeof(const char*) );
}
const char* SC_mod_Cmde(const SC_module* module PROGMEM, uint8_t n){
  uint16_t addr = pgm_read_word( ((uint16_t)module) + sizeof(const char*) );
  return pgm_read_word( addr + n * sizeof(const char*));
}
fctSC_Cmde SC_mod_Fct(const SC_module* module PROGMEM){
  return pgm_read_word( ((uint16_t)module) + 2*sizeof(const char*) + sizeof(const uint8_t));
}
