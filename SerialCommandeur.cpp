#include "SerialCommandeur.h"



SerialCommander::SerialCommander(HardwareSerial* serial, const SC_module* listeModule, uint8_t nbModule){
  this->serial=serial;
  this->listeModule=listeModule;
  this->nbModule=nbModule;

  this->pos=0;
  this->statut_spe=0;
  this->lastModule=NULL;
}

void SerialCommander::printDebug(){
  for(uint8_t i=0;i<nbModule;i++){
    this->print(F("Module : "));
    this->printlnPGM(SC_mod_Nom(&this->listeModule[i]));

    for(uint8_t n=0;n<SC_mod_TailleListe(&this->listeModule[i]);n++){
      this->print(F(" * "));
      this->printlnPGM(SC_mod_Cmde(&this->listeModule[i], n));
    }
  }
}

void SerialCommander::printlnPGM(const char* PROGMEM data){
  this->printPGM(data);
  this->println();
}

void SerialCommander::printPGM(const char* PROGMEM data){
  char c=pgm_read_byte((uint16_t)data);
  while( c != 0 ){
    this->serial->print(c);
    data++;
    c=pgm_read_byte((uint16_t)data);
  }
}

void SerialCommander::check(){
  while( this->serial->available() ){
    char c=this->serial->read();

    if( ( c >= ' ' && c <= 126 ) || this->statut_spe & SC_BIT_GUILLEMET){
      if( c == ' ' and this->pos == 0 ) continue; // zappe les espaces initiaux
      this->serial->write(c);
      this->tampon[this->pos]=c;
      this->pos++;

      // précédence du slash ?
      if( this->statut_spe & SC_BIT_SLASH ) { // si précédent est slash !
        this->statut_spe=this->statut_spe &~SC_BIT_SLASH;
      } else { // cas ou le précédent n'est pas un slash

        // gestoin du slash
        if( c == '\\' ) this->statut_spe = this->statut_spe | SC_BIT_SLASH;

        // gestion du guillement
        if( c == '"') this->statut_spe ^= SC_BIT_GUILLEMET;
      }
      continue;
    }
    if( c == 13 || c == 10 ){ // retour chariot / nouvelle ligne
      if( this->pos == 0 ) continue;  // si pas de données avant, ne fait rien
      this->serial->write(c);

      while( this->serial->peek() == 13 || this->serial->peek() == 10 ) this->serial->write(this->serial->read()); // purge les éventuels /r ou /n suivant dans la file d'arrivée

      if( this->pos == 0 ) continue;  // si pas de données avant, ne fait rien

      #ifdef DEBUG_SC
        this->utilisationMaxTampon = max(this->pos, this->utilisationMaxTampon);
      #endif

      this->tampon[this->pos]=0;
      this->execute();
      this->pos=0;
      return; // execute max 1 commande par appel pour laisser un peu la main
    }
    if( c == 8 ){ // backspace
      this->serial->write(c);
      this->pos--;
    }
  }
}

void SerialCommander::execute(){
  dataFctCherche data=this->chercheModule();

  if( data.module == NULL ){
    this->printlnPGM(SC_TXT_CMDE_INCONNUE);
    return;
  }

  data=this->chercheCmde(data);

  if( data.cmde == NULL ) {
    this->printlnPGM(SC_TXT_CMDE_INCONNUE);
    return;
  }

  if( data.startNext == 0xFF ){ // pas d'arguments
    data.startNext=0;
    memcpy(tampon, &data, sizeof(data));
  } else {
    if( data.startNext < sizeof(data) ){ // copie pour créer de la place
      uint8_t ecart=sizeof(data)-data.startNext;
      for(uint8_t i=SERIAL_COMMANDEUR_TAILLE_TAMPON-1;i>=data.startNext; i--){
        this->tampon[i]=this->tampon[i-ecart];
      }
    } else if(data.startNext > sizeof(data) ){
      uint8_t ecart=data.startNext-sizeof(data);
      for(uint8_t i=sizeof(data); i<SERIAL_COMMANDEUR_TAILLE_TAMPON;i++){
        this->tampon[i]=this->tampon[i+ecart];
      }
    }
    data.startNext=this->calculNbArgs();
    memcpy(tampon, &data, sizeof(data));
  }

  fctSC_Cmde fct=SC_mod_Fct(data.module);
  fct(this);
}

uint8_t SerialCommander::calculNbArgs(){
  /* parcours de tampon depuis sizeof(data) jusqu'à trouvé un \0

  mettre un \0 entre chaque arg et deux \0 à la fin

  args séparés par des ' ' ou des tabulation
  */

  uint8_t i=sizeof(dataFctCherche);
  uint8_t spe=0;
  uint8_t n=0;

  while(this->tampon[i] != 0 && i < SERIAL_COMMANDEUR_TAILLE_TAMPON){
    if( spe & SC_BIT_SLASH ){ // le caractère précédent était un slash, on ne prend donc en compte
      spe &= ~SC_BIT_SLASH;
      switch( this->tampon[i] ){
        case 'n': // nouvellle ligne
          this->tampon[i] = 10;
          break;
        case 'r': // retour chariot
          this->tampon[i] = 13;
          break;
        case 't': // horizontal tab
          this->tampon[i] = 9;
          break;
      }
    } else {
      if( this->tampon[i] == '\\' ){
        spe |= SC_BIT_SLASH;
        this->supprDeTampon(i,1);
        i--;
      } else if( this->tampon[i] == '"'){
        spe ^= SC_BIT_GUILLEMET;
        this->supprDeTampon(i, 1);
        i--;
      } else if( this->tampon[i] == ' ' && ! spe & SC_BIT_GUILLEMET ){
        if( this->tampon[i-1] == 0 ) {
          this->supprDeTampon(i, 1);
          i--;
        } else {
          this->tampon[i] = 0;
          n++;
        }
      } else if( this->tampon[i] == 0 ){
        if( i+1 != SERIAL_COMMANDEUR_TAILLE_TAMPON ){
          this->tampon[i+1]=0;
        }
        n++;
        return n;
      }

    }

    i++;
  }

  if( this->tampon[i-1] != 0 ) n++;

  return n;
}

void SerialCommander::supprDeTampon(uint8_t pos, uint8_t taille){
  for(uint8_t i=pos+taille;i<SERIAL_COMMANDEUR_TAILLE_TAMPON;i++) this->tampon[i-taille]=this->tampon[i];
  for(uint8_t i=SERIAL_COMMANDEUR_TAILLE_TAMPON - taille;i<SERIAL_COMMANDEUR_TAILLE_TAMPON;i++)this->tampon[i]=0;
}

const SC_module* SerialCommander::getModule(){
  return ((dataFctCherche*)tampon)->module;
}
const char * SerialCommander::getCmde(){
  return ((dataFctCherche*)tampon)->cmde;
}

fctSC_Cmde SerialCommander::getFct(){
  return (fctSC_Cmde)pgm_read_word(((dataFctCherche*)tampon)->module->fct);
}

uint8_t SerialCommander::getNbArgs(){
  return ((dataFctCherche*)tampon)->startNext;
}

char* SerialCommander::getArg(uint8_t n){
  if( n >= this->getNbArgs() ) return NULL;

  uint8_t i=sizeof(dataFctCherche);
  while( n > 0){
    while( tampon[i] != 0 ){
      i++;
    }
    i++;
    n--;
    if( i == SERIAL_COMMANDEUR_TAILLE_TAMPON ) return NULL;
  }
  return &(tampon[i]);
}


dataFctCherche SerialCommander::chercheModule(){
  #ifdef DEBUG_SC
  this->println();
  this->print(this->tampon);
  #endif

  uint8_t i=0;
  while(1){
    if( this->tampon[i]=='.'){
      if( i == 0 ){ // '.' en 1ere position -> rappel du module utilisé précédement
        return dataFctCherche{this->lastModule, NULL, 1};
      } else {// recherche du module
        this->tampon[i]=0;
        i++;
        uint8_t j=0;
        while(j<this->nbModule){
          if( strcmp_P(this->tampon, SC_mod_Nom(&this->listeModule[j])) == 0){ // trouvé
            return dataFctCherche{&(this->listeModule[j]), NULL, i};
          }
          j++;
        }
        return dataFctCherche{NULL, NULL, 0xFF};
      }
    }
    if( this->tampon[i] == ' ' || this->tampon[i] == 0){
      return dataFctCherche{this->listeModule, NULL, 0};
    }
    if( this->tampon[i] > 'z' || ( this->tampon[i] < '0' ) || ( this->tampon[i] < 'A' && ( this->tampon[i] > '9' || i == 0 ) ) || this->tampon[i] == 96 || ( this->tampon[i] > 'Z' && this->tampon[i] < '_' ) ) {  // caractère interdit là
      return dataFctCherche{NULL, NULL, 0xFF};
    }
    i++;
  }
}

dataFctCherche SerialCommander::chercheCmde(dataFctCherche data){
  uint8_t i=data.startNext;

  while(1){
    if( this->tampon[i] == ' ' || this->tampon[i] == 0 ){
      if( i == data.startNext ){ // fin de nom au 1er caractère -> erreur pas de fonction donnée ...
        data.cmde=NULL;
        return data;
      }
      char c=this->tampon[i];
      this->tampon[i]=0;
      uint8_t n=0;
      while( n < SC_mod_TailleListe(data.module)){
        if( strcmp_P(&(this->tampon[data.startNext]), SC_mod_Cmde(data.module, n)) == 0 ){
          data.cmde=SC_mod_Cmde(data.module, n);
          if( c == ' ' ) data.startNext=i+1;
          else data.startNext=0xFF;
          return data;
        }

        n++;
      }
    }

    if( this->tampon[i] > 'z' || ( this->tampon[i] < '0' ) || ( this->tampon[i] < 'A' && this->tampon[i] > '9' ) || this->tampon[i] == 96 || ( this->tampon[i] > 'Z' && this->tampon[i] < '_' ) ) {  // caractère interdit là
      data.cmde=NULL;
      return data;
    }
    i++;
  }
}
