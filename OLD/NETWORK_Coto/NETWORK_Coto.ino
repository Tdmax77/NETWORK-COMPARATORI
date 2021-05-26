/*  Questo programma va su un Nano con display 1602 e modulo radio nrf24l01.
    Funzionamento:
    Flusso trasmissione dati:
    Encoder > payload.offsetRequet          > Display
            > payload.num_sent  >
            < ToEnc.OffsetSetted       <
            < ToEnc.ValOffset              <
    Encoder appena acceso ha la variabile payload.OffsetReq a 1 ovvero manda una richiesta di inserimento offset al display;
    Display visualizza la richiesta, registra un dato var che inserisco io e lo spedisce come ToEnc.Valoffset all'Encoder, poi spedisce anche un ToEnc.OffsetSetted a 1 (era a zero) ad encoder
    Quando encoder riceve il ToEnc.offsetimpostato a 1 allora manda a 0 payload.OffsetReq (e quindi non chiederà più un offset).
    Il programma procede visualizzando il dato payload.num_sent che è la somma di ToEnc.Valoffset + il valore letto dall'encoder.
    il calcolo lo fa l'encoder, in tal modo se dovessi spegnere il display o perdere il segnale, il dato non cambierà.
    Se l'encoder viene spento, display avvisa che non lo riceve (anche se va fuori campo).
    Quando si riaccende l'encoder, la variabile payload.OffsetReq parte a 1 quindi chiede di reinserire un valore di offset.
  Connessioni Display:
  modulo nrf24
  CE   9
  SCN  10
  mosi 11
  miso 12
  sck  13
  pulsante 1 non utilizzato = 7;
  const int buttonOk = 6;
  const int buttonUpPin = 4;
  const int buttonDownPin = 5;
*/
#include <RF24.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#include "printf.h"
#include <RF24Network.h>
//#include <nRF24L01.h>
#define risoluzioneEncoder  0.05 // INSERIRE QUI VALORE ENCODER uguale anche ne programma kren 360/7200=0.05 ma adesso va 360/2850


/* variabile display*/
LiquidCrystal_I2C lcd(0x27, 16, 2);
/* variabile display*/

/*variabile network */
RF24 radio(9,10);              // nRF24L01 (CE,CSN)
RF24Network network(radio);     // Network uses that radio
const uint16_t this_node = 00;  // Address of our node in Octal format ( 04,031, etc)   this is the master (raspy display)
const uint16_t node_5 = 05;     // encoder
char msg[20];
static unsigned long previousSuccessfulTransmission;
boolean transmissionState = true;       // per laggiornamento del display dopo che ha perso il segnale
boolean PretransmissionState = false;   // per conttrollare la prerdita/ripresa del segnale

/* conversione Angolo */
float Angolo;
/* conversione Angolo */

//questa struttura manda i dati dall'encoder al display
struct payload_t {
  long num_sent; // valore che verrà visualizzato ovvero la somma del valore dell'encoder + l'offset che ho inserito
  int control;  // this is a control variable that continuously incrases on every data transmission, i need it for to understand if COMP is alive
  int OffsetReq ; //se prima accensione richiederà l'offset
};
payload_t payload;

//questa struttura definisce l'ToEnc payload ovvero i dati dal display all'encoder
struct ToNode05 {
  bool OffsetSetted; // se display rileva chisura encoder ridomanda offset
  int ValOffset; //valore letto sul volano e impostato dal display
};
ToNode05 ToEnc;

int val =9999 ;                 // variabile utilizzata nella procedura inserimento offset
int insert = 0;           // variabile utilizzata per la richiesta di offset dall'encoder

long num_sentPrev;
/*variabile network */



/*Variabili Offset servono ad inserire il valore usando i 3 pulsanti*/
const int buttonOkPin = 4;
const int buttonUpPin = 3;  //numero pin a cui è collegato il pulsante di UP
const int buttonDownPin = 2;  //numero pin a cui è collegato il pulsnte di DOWN
int buttonOkState;
int buttonUpState;  //stato attuale del pulsante di UP
int buttonDownState;  //stato attuale del pulsante di Down
long UpDebounceTime;  //Variabilie di appoggio per calcolare il tempo di cambio di stato del pulsante di UP
long DownDebounceTime;  //Variabilie di appoggio per calcolare il tempo di cambio di stato del pulsante di Down
long timerButtonPushed; //tempo che indica da quanto è stato premuto il pulsante
long timerPauseRepeat;  //tempo che indica da quanto è stata fatta l'ultima ripetizione
long time_add_10 = 1200;  //costante per attivare ripetizione da +10  era 1000
long time_add_100 = 9000; //costante per attivare ripetizione da +100  era 5000
long time_pause = 250;  //costante che determina il tempo di ripetizione di UP e di DOWN
long debounceDelay = 80; //Tempo di debounce per i pulsanti UP e DOWN
boolean repeatEnable = LOW; //memoria che indica il repeat del pulsante attivo

int var = 0;  //variabile da aumentare o diminuire come valore
const int varMax = 720; //limite massimo valore della variabile
const int varMin = 0; //limite minimo valore della variabile

int readingUp = 0;  //Lettura ingresso digitale del pulsante di UP
int readingDown = 0;  //Lettura ingresso digitale del pulsante di Down

/*Variabili Offset servono ad inserire il valore usando i 3 pulsanti*/

/*Variabili Pulsanti */
//int pulsante_1 = 0; // non utilizzato
/*Variabili Pulsanti */


int mostraangolo = 0; // serve a riscrivere l'angolo sul display dopo che ha ripreso la connessione (visto che display_angolo refresha solo se angolo cambia)


/* variabili per controllo freschezza dati */
int control;        // variabile contatore che aumenta se i dati vengono ricevuti dall encoder
int controlprec = 9999;
/* variabili per controllo freschezza dati */
int esci = 0;

/**********************************************************************************************************/
/***********************   SETUP   ************************************************************************/
/**********************************************************************************************************/
void setup() {


  /* Setup network */
  Serial.begin(38400);
  SPI.begin();
  radio.begin();
  network.begin(/*channel*/ 90, /*node address*/ this_node);
  /* Setup network */

  /* parte display i2c */
  lcd.init();                      // initialize the lcd
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Crankshaft");
  lcd.setCursor(0, 1);
  lcd.print("Wireless reader");
  delay(3000);
  lcd.clear();
  /* parte display i2c */

  /*Parte Variabile Offset*/
  pinMode (buttonUpPin, INPUT);   //impostazione buttonUpPin come ingresso
  pinMode (buttonDownPin, INPUT); //impostazione buttonDownPin come ingresso
  pinMode (buttonOkPin, INPUT);   //impostazione buttonOkPin come ingresso
  /*Parte Variabile Offset*/


}




/**********************************************************************************************************/
/***********************   LOOP   ************************************************************************/
/**********************************************************************************************************/

void loop()
{
  network.update();
// while ( network.available()) {                                // verifico che il network sia disponibile
    RF24NetworkHeader header(00);                                   // imposto indirizzo
    network.read(header, &payload, sizeof(payload));            // leggo network
//long i = payload.num_sent;
Serial.println(payload.num_sent);
 //}

}


/*    size_t node_id = header.from_node;                          // create a variable node_id that stores the source of the packet ( who is sending it to me? COMP1 with 01 , COMP2 with 02 and so on..
    if (header.from_node == 05) {                               // se i dati arrivano dall'encoder
      insert = payload.OffsetReq;                               // verifico se è richiesto offset e lo metto in variabile insert

      if ((insert == 1)  )                                      // SE E' RICHIESTO L'OFFSET
      {
       testo_richiesta_inserimento_offset();
       Serial.println("testo_richiesta_inserimento_offset()");
       insert = 0;                                                 //  azzero la richiesta di offset per non entrare nella procedura
       ToEnc.OffsetSetted = 1;                                     //  comunico allìencoder che l'offset è stato settato//  a display chiedo l'inserimento
       RF24NetworkHeader header5(05);
       network.write(header5, &ToEnc, sizeof(ToEnc));              //  spedisco
       payload.OffsetReq = 0;

        while (digitalRead(buttonOkPin) == LOW ) {
          
             PROCEDURA_OFFSET();  
          
         
          }

        // insert = 0;                                                 //  azzero la richiesta di offset per non entrare nella procedura
        //  ToEnc.OffsetSetted = 1;                                     //  comunico allìencoder che l'offset è stato settato
        val = var;                                                  //  imposto la variabile da spedire all'encoder (val) al valore di offset impostato (var)
        ToEnc.ValOffset = val;                                      //  spedisco l'offset
        //    RF24NetworkHeader header5(05);                              //  imposto indirizzo encoder
        network.write(header5, &ToEnc, sizeof(ToEnc));              //  spedisco
        //    payload.OffsetReq = 0;                                      //  azzero la richiesta offset
        delay(200);
        network.read(header, &payload, sizeof(payload));
      }


      /*     int controlprec;
           control = payload.control;              // associo a control il valore ricevuto
           if (control != controlprec) {           // se è cambiato da quello prima
             display_angolo();                     // visualizzo l'angolo
             control++;                            // incremento control
           } else if (control == controlprec){         // se non è cambiato rispetto a prima
             display_no_conn();                        // non ho connessione
           }
      */
      /*
    }
  }

  control = payload.control;              // associo a control il valore ricevuto
  if (control != controlprec) {           // se è cambiato da quello prima
    display_angolo();                     // visualizzo l'angolo
    controlprec = control;                // incremento control
  }


  dati_a_seriale();                           // dati a seriale per debug
}

*/


/*****************************************************************************************************************************************************************************************************************/
/*****************************************************************************************************************************************************************************************************************/
/*****************************************************************************************************************************************************************************************************************/

/*
void check_Transmission() {                         // procedura verifica trasmissione (usata???)


  if (millis() - previousSuccessfulTransmission > 500)                   // se non ricevo niente entro tot millisecondi
  {
    transmissionState = false;
#ifdef DEBUG
    Serial.println("Data transmission error, check Transmitter!");
#endif
    do {
      display_no_conn();
    } while (transmissionState = false) ;
  }

}

void readButtonState() {                            //  Debounch

  int readingUp = digitalRead(buttonUpPin); //Lettura ingresso digitale del pulsante di UP
  int readingDown = digitalRead(buttonDownPin); //Lettura ingresso digitale del pulsante di Down

  if (readingUp == HIGH) {
    if ((millis() - UpDebounceTime) > debounceDelay) {
      buttonUpState = HIGH;
    }
  } else {
    buttonUpState = LOW;
    UpDebounceTime = millis();
  }

  if (readingDown == HIGH) {
    if ((millis() - DownDebounceTime) > debounceDelay) {
      buttonDownState = HIGH;
    }
  } else {
    buttonDownState = LOW;
    DownDebounceTime = millis();
  }
}

void PROCEDURA_OFFSET() {                           // mi restituisce un valore var che ho inserito come offset

  
  Serial.print(" SONO NELLA PROCEDURA OFFSET()   ");
  dati_a_seriale();

  readButtonState();  //Lettura stato buttons con controllo antirimbalzo

  if (buttonUpState == HIGH || buttonDownState == HIGH) {
    if ((repeatEnable == HIGH && ((millis() - timerPauseRepeat) > time_pause)) || repeatEnable == LOW) {
      if ((millis() - timerButtonPushed) > time_add_10) {
        if ((millis() - timerButtonPushed) > time_add_100) {
          if (buttonUpState == HIGH) var = var + 100;
          if (buttonDownState == HIGH) var = var - 100;
        } else {
          int resto = 0;
          if (buttonUpState == HIGH) resto = 10 - (var % 10);
          if (buttonDownState == HIGH) resto = (var % 10);
          if (resto == 0) {
            if (buttonUpState == HIGH) var = var + 10;
            if (buttonDownState == HIGH) var = var - 10;
          } else {
            if (buttonUpState == HIGH) var = var + resto;
            if (buttonDownState == HIGH) var = var - resto;
          }
        }
      } else {
        if (buttonUpState == HIGH) var++;
        if (buttonDownState == HIGH) var--;
      }
      timerPauseRepeat = millis();
      repeatEnable = HIGH;
      if (var > varMax) var = varMax;
      if (var < varMin) var = varMin;
      lcd.setCursor(0, 0);
      lcd.print("                 ");    //disegnare caratteri vuoti dovrebbe essere piu veloce del clear
      lcd.setCursor(0, 1);
      lcd.print("                 ");
      lcd.setCursor(0, 0);
      lcd.print("Inserire Offset:");
      lcd.setCursor(1, 1);
      lcd.print(var);
      lcd.setCursor(10, 1);
      lcd.print("Gradi");
      delay(200);
    }
  } else {
    timerButtonPushed = millis();
    timerPauseRepeat = millis();
    repeatEnable = LOW;
  }

esci = 1;
}

void display_no_conn() {                            // scrive No Connection Check encoder

  lcd.setCursor(0, 0);
  lcd.println("NO CONNECTION   ");
  lcd.setCursor(0, 1);
  lcd.println("Check Encoder   ");
}

void testo_richiesta_inserimento_offset() {         // Scrive inserire valore angolo volano
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.println("INSERIRE VALORE ");
  lcd.setCursor(0, 1);
  lcd.println("ANGOLO VOLANO   ");
}

void display_angolo() {                             //ridisegno il display unicamente se il dato dell'angolo cambia (evito flashamenti )
  float angolo = payload.num_sent / 100.0;
  float angoloprec;
  if (angolo != angoloprec) {
    angoloprec = angolo;
    lcd.setCursor(0, 0);
    lcd.print("    Angolo:     ");
    lcd.setCursor(10, 1);
    lcd.print("Gradi");
    lcd.setCursor(0, 1);
    lcd.print("          ");
    lcd.setCursor(1, 1);
    lcd.print(angolo);
  }
}


/*
  void increase_counter(size_t id) {                  // on every cycle it increases all the counters ecxcept that of the node it just read
  for (size_t ii = 0; ii < N_rec; ii++) {
    counter[ii]++;
  }
  counter[id]--;
  }

  void fix_values() {                                 // if the couter reach the max number of fails setted with Nnosignal variable, data.num_sent will be setted to 8888
  for (size_t ii = 0; ii < N_rec; ii++) {
    if (counter[ii] >= Nnosignal) {
      data[ii].num_sent = dummy_val;
      counter[ii] = 0;
    }
  }
  }

  void test() {                                       // debug da consolle

  if (Serial.available() > 0) {
    char attesa = Serial.read();
    // while (attesa = ! "a") {
    // };
    if (attesa == 'A' || attesa == 'a') {

      data[4].OffsetReq = 1;
    }
    if (attesa == 'S' || attesa == 's') {

      data[4].OffsetReq = 0;
    }
    if (attesa == 'z' || attesa == 'Z') {

      val++;
    }
    if (attesa == 'x' || attesa == 'X') {

      val--;
    }

    if (attesa == 'h' || attesa == 'H') {

      ToEnc.OffsetSetted = 1;
    }

    if (attesa == 'j' || attesa == 'J') {

      ToEnc.OffsetSetted = 0;
    }
  };
  }
*/
void dati_a_seriale () {                            // Stampa a seriale
  Serial.print("num_sent ");
  Serial.print(payload.num_sent);
  Serial.print(", control ");
  Serial.print(payload.control);
  Serial.print(", OffsetReq ");
  Serial.print(payload.OffsetReq);
  Serial.print("     OffsetSetted ");
  Serial.print(ToEnc.OffsetSetted);
  Serial.print(" ValoreOffset ");
  Serial.print(ToEnc.ValOffset);
  Serial.print (" controlprec ");
  Serial.print (controlprec);
  Serial.print (" control ");
  Serial.print (control);
  Serial.println();
}
