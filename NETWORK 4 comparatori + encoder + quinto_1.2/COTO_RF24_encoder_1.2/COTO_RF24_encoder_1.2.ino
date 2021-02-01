/*This is the code for the master in our project " WARTSILA WIRELESS COMPARATORS and ENCODER (Kren) READING SYSTEM "

   The target is to read 4 comparators and an encoder  that continuously send data to a central unit.


   WARNING: LET'S START FROM THE ASSUMPTION THAT THE VALUES READED FROM THE NODES (COMPARATORS) ARE INCLUDED BETWEEN 0 AND 1360
   OTHERWISE WE COULD HAVE ABNORMAL BEHAVIOR

  How it works:

   Every comparator has a transmitter and continuosly sends data to the network.
   Every node ( comparator + transmitter) sends  a payload with 3 informations:
   header.from_node  is its adress
   payload.num_sent  is the value readed from comparator
   payload.control   is a number that increases on every cycle. We need it for to decide if the the node is active or not. If it fails, control will not increase anymore.

   The central unit (for which this code is written), on every cycle read a packet from the network and analyzes it:
   It reads who is the sender and the data, then if they are valid, stores them into data array.
   The reading from the network is not sequential, it is random so i need to create the array for to assure that it read all the nodes of my net (all the comparators and the encoder)
   On every cycle i incrase the counter of all nodes excepted the one i read. If a node is down his counter will increase untill it will reach the maximum allowed value (Nnosignal)
   In this case the the data sended to raspberry will be 8888.

   Example: if i read comp1, comp2, comp4 correctly but comp3 is power off i will read something like:
   0000,0100,8888,0099
   0000,0101,8888,0099

   In this 2 lines i see that comp 1 is not changed , comp2 is increased by 1 cents, comp3 is not reachable, comp4 is not changed.
   On every cycle the code sends all the data to the raspberry which is demanded to control graphic interface and data storage.

   The encoder (kren) is the node_5, it requires a different approach because its value needs to be aligned with flywheel index using an OFFSET Value (VO).
   Kren asks for an Offset value when it starts (OffsetReq = 1), Offset value is setted to 9999 (VO = 9999).
   So Master wait for this value from user. Then it stores the data in val, send it to encoder and esc from Offset task.
   Because of a "strange problem" it asks 4-5 times the offset value (probably a timing problem) so i introduced a condition y.
   y is setted to 1 all the time,this is a condition for enter into Offset task, after the task y becomes 0 for only 0,1 sec so the program can't enter again in Offset task.
   After 0,1 sec y goes to 1 so, if encoder is closed and reopen, master could enter into Offset task again.

   Powered by Massimo Riosa
   Release 1.1
*/



#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#include "printf.h"

/* variabile display*/
LiquidCrystal_I2C lcd(0x27, 16, 2);
/* variabile display*/


RF24 radio(9/*CE*/, 10/*CSN*/);             // Coto (9,10)  Kren (7, 8)  RF - NANO with ext ant. (9, 10) RF - NANO without ext ant (10, 9)

RF24Network network(radio);     // Network uses that radio
const uint16_t this_node = 00;  // Address of our node in Octal format ( 04,031, etc)   this is the master (raspy display)
const uint16_t node_5 = 05;     // Address of the encoder node in Octal format          this is ENCODER

struct payload_t {              // Structure of  payload received from slaves
  long num_sent;                // this is the data readed from COMP, it is the value readed from Mitutoyo Comparator
  int control;                  // this is a control variable that continuously incrases on every data transmission, i need it for to understand if COMP is alive
  int OffsetReq;                // if 1 Offset is requested, used only by node_5
  int VO;                       // Offset value
};
payload_t payload;

long valoreangolocorrettoPrev;
int mostraangolo = 0;
float angprint;
float angstamp;

int dummy_val (8888);     // define a value that will be printed if COMP fails
payload_t data;    // create an array data
int counter ;      // Array defined for validation of data (ses main loop)
int Nnosignal (30);       // number of acceptable failed reads

//int val = 0;              // This is the value readed on the index of flywheel by user and insered by the OFFSET Request task
unsigned long tempo;      // need for the y counter
int y = 1;                // need for block OFFSET Request loop

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
//int counter;
int counterprec;

//////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// S E T U P  ////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

void setup(void)
{
  Serial.begin(38400);                                       // initialize serial for to send data to raspberry
  SPI.begin();                                               // initialize SPI
  radio.begin();                                             // initialize radio module
  network.begin(/*channel*/ 90, /*node address*/ this_node); // start network

  /*Parte Variabile Offset*/
  pinMode (buttonUpPin, INPUT);   //impostazione buttonUpPin come ingresso
  pinMode (buttonDownPin, INPUT); //impostazione buttonDownPin come ingresso
  pinMode (buttonOkPin, INPUT);   //impostazione buttonOkPin come ingresso
  /*Parte Variabile Offset*/

  /* parte display i2c */
  lcd.init();                      // initialize the lcd
  lcd.backlight();
  /*  lcd.setCursor(0, 0);
    lcd.print("Crankshaft");
    lcd.setCursor(0, 1);
    lcd.print("Wireless reader");
    delay(3000);*/
  lcd.clear();
  /* parte display i2c */



  data.num_sent = 7777;                                // num_sent starts from 7777
  data.control = 0;                                    // control starts from 0
  counter = 0;                                         // reset counter

}




//////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// L O O P //////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

void loop(void) {

  if (millis() > tempo + 300) {                              // y is always 1 so i can enter into Offset task,
    y = 1;                                                   // when Offset is setted y become 0 so the Offset task can't loop.
  } else {                                                   // after 100 milliseconds y become 1 so it is possible to execute Offset task.
    y = 0;
  }


  network.update();           // looking for news on the network
  ser();

  while ( network.available() ) {                            // if network is up  ... I need at least one slave active for to keep active the network, otherwise no network will be available
  
    if (mostraangolo == 1) {
        lcd.setCursor(0, 0);
        lcd.print("    Angolo:     ");
        lcd.setCursor(10, 1);
        lcd.print("Gradi");
        lcd.setCursor(0, 1);
        lcd.print("          ");
        lcd.setCursor(1, 1);
        lcd.print(angstamp);
        mostraangolo = 0;
      }
    
    int cnt (0);                                             // reset counter
    RF24NetworkHeader header;                                // declare the address of this node: every packet sent from slaves to header will be readed from this node (00, the master)
    network.read(header, &payload, sizeof(payload));         // read the packet netwotk sent for this node
    size_t node_id = header.from_node;                       // create a variable node_id that stores the source of the packet ( who is sending it to me? COMP1 with 01 , COMP2 with 02 and so on..

    if ((payload.OffsetReq == 1) && (payload.VO = 9999) && (y == 1)) {     // if Offset is request i start the Offset task
      testo_richiesta_inserimento_offset();
      while (digitalRead(buttonOkPin) == LOW )
      {
        PROCEDURA_OFFSET();
      }
      payload.OffsetReq = 0;                                                // reset the Offset request
      payload.VO = var;                                                     // assign offset value to payload
      RF24NetworkHeader header5(05);                                        // define encoder network address
      network.write(header5, &payload, sizeof(payload));                    // send payload to encoder
      delay(100);                                                           // little delay
      tempo = millis();                                                     // set variable for y status
      y = 0;                                                                // put y to 0
    }
  }

  if (data.control != payload.control) {      // if control readed from network is different from control stored i assume that the packet is new, so the slave is alive and the data is valid
    data.num_sent = payload.num_sent;         // so i store readed values in data.num_sent ( this is comparator's readed value from slave)
    data.control = payload.control;           // update data.control to new value received  (this is the counter sent from slave that increase on every sending)
    counter = 0;                              // reset couter of the node i readed in this cycle.
  } else {
    counter = counter += 1;                                                      // A valid data is included in a range from 0 to 1360 that is the values readed from Mitutoyo comparator (0-1360 cent)
  }

  if (counter >= Nnosignal) {
    display_no_conn();
    delay(2000);
    mostraangolo = 1;
  } else {
    display_angolo();
  }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////  R O U T I N E S  //////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void readButtonState() {

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

void PROCEDURA_OFFSET() // mi restituisce un valore var che ho inserito come offset
{
#ifdef DEBUG
  Serial.println("");
  Serial.println("SONO NELLA PROCEDURA OFFSET()");
  Serial.println("");
#endif
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
}


void display_no_conn() {

  lcd.setCursor(0, 0);
  lcd.println("NO CONNECTION   ");
  lcd.setCursor(0, 1);
  lcd.println("Check Encoder   ");
}

void testo_richiesta_inserimento_offset() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.println("INSERIRE VALORE ");
  lcd.setCursor(0, 1);
  lcd.println("ANGOLO VOLANO   ");
}

void display_angolo() {
  if (payload.num_sent != valoreangolocorrettoPrev)//ridisegno il display unicamente se il dato dell'angolo cambia (evito flashamenti )
  {
    valoreangolocorrettoPrev = payload.num_sent;
     angprint = payload.num_sent;
     angstamp = angprint/100;
    lcd.setCursor(0, 0);
    lcd.print("    Angolo:     ");
    lcd.setCursor(10, 1);
    lcd.print("Gradi");
    lcd.setCursor(0, 1);
    lcd.print("          ");
    lcd.setCursor(1, 1);
    lcd.print(angstamp);
  }
}

void ser() {
  Serial.print("num_sent ");
  Serial.print(payload.num_sent);
  Serial.print(", control ");
  Serial.print(payload.control);
  Serial.print(", OffsetReq ");
  Serial.print(payload.OffsetReq);
  Serial.print("     VO ");
  Serial.print(payload.VO);
  Serial.print("Counter: ");
  Serial.print(counter);
  Serial.println();
}
