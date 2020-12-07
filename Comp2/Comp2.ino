//  Prima prova di trasmissione su network

// variabili per mitutoyo ****************************************
int req = 8; //assegna a req il piedino 11 (D8) through q1 (arduino high pulls request line low)
int dat = 2; // assegna a dat il piedino 5  (D2)   Linea Dati del calibro
int clk = 4; // assegna a clk il piedino 7  (D4)  clock del calibro
int i = 0; int j = 0; int k = 0;
byte mydata[14];
int num;   //    <-- DATO DA INVIARE
int num_prec;
int sign;
// variabili per mitutoyo ****************************************

// variabili network *********************************************
#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <printf.h>
RF24 radio(10, 9);     // nRF24L01(+) radio attached using Getting Started board (pin fissi cablati sulla scheda)
RF24Network network(radio);          // Network uses that radio
const uint16_t this_node = 02;        // Address of our node in Octal format <<-- INSERIRE IL NUMERO DEL COMPARATORE
const uint16_t other_node = 00;       // Address of the other node in Octal format
struct payload_t {                  // Structure of our payload
  int num_sent;
};
payload_t pl;
// variabili network *********************************************

// variabili debug
//#define DEBUG  //if not commented out, serial.print is active! <<--- DECOMMENTARE SE NECESSARIO 
//#ifdef DEBUG
char tasto = 0;
//#endif
// variabili deguh

void setup() {

  // setting lettura mitutoyo ************************************
  pinMode(req, OUTPUT);         // setta il pin 11 come OUTPUT
  pinMode(clk, INPUT_PULLUP);   // setta il pin 7 come Input abilitando la resistenza di pullup
  pinMode(dat, INPUT_PULLUP);   // setta il pin 5 come input abilitando la resistenza di pullup
  digitalWrite(req, LOW);       // porta a LOW il pin 11
  // setting lettura mitutoyo ************************************

  // setting per network *****************************************
  SPI.begin();
  radio.begin();
  network.begin(/*channel*/ 90, /*node address*/ this_node);
  // setting per network *****************************************

  // Debug *******************************************************
//#ifdef DEBUG
  Serial.begin(115200);
  printf_begin();
  radio.printDetails();
//#endif

#ifndef DEBUG
//  Serial.end(); // make sure, serial is off!
#endif
  // Debug *******************************************************
}

//********************************************************************************************************
//********************************************************************************************************
//********************************************************************************************************

void loop() {

//#ifdef DEBUG
  tastiera();
//#endif


  // fase di lettura dati del mitutoyo ************************
  digitalWrite(req, HIGH);      // genera set request portando il pin 11 a HIGH

  for (i = 0; i < 13; i++ ) {
    k = 0;

    for (j = 0; j < 4; j++) {
      while ( digitalRead(clk) == LOW) { // hold until clock is high
      }

      while ( digitalRead(clk) == HIGH) { // hold until clock is low
      }

      bitWrite(k, j, (digitalRead(dat) & 0x1)); // read data bits, and reverse order )
    }
    // estrai i dati da mydata
    mydata[i] = k;
  }

  // sign = mydata[4]    decimal = mydata[11]  units = mydata[12];
  sign = mydata[4]; // sign memorizza il segno negativo

  // assemble measurement from bytes
  char buf[8];

  for (int lp = 0; lp < 6; lp++) {

    buf[lp] = mydata[lp + 5] + '0';
    buf[6] = 0;

    //num = atol(buf); //converte la riga in long integer

    if (sign == 8) {   // se il segno è negativo fa diventare num negativo num=num-(2xnum)
      num = num - (2 * num);
    }

  }
  // fase di lettura dati del mitutoyo ************************
  // a questo punto mi ritrovo con un dato num da spedire
//#ifdef DEBUG
  Serial.print("Valore letto da Comp");
  Serial.print(this_node);
  Serial.print(" ");
  Serial.println(pl.num_sent);
//#endif
  // scrittura su network ***************************************
  pl.num_sent = num;
  network.update();                          // Check the network regularly

  if (pl.num_sent != num_prec) {
//#ifdef DEBUG
    Serial.print("Sending...");
//#endif
    //payload_t payload = {num};
    RF24NetworkHeader header(/*to node*/ other_node);
    bool ok = network.write(header, &pl, sizeof(pl));
//#ifdef DEBUG
    if (ok)
      Serial.println("ok.");
    else
      Serial.println("failed.");
//#endif
    num_prec = num;
  }

  // scrittura su network ***************************************

}


void tastiera() {
  tasto = Serial.read();
  switch (tasto) {
    case 'a':
      num++;
      break;
    case 's':
      num--;
      break;
  }
}
