//  _SLAVE_COMPARATORE_ARDUINO_NANO
#include <Wire.h>
byte a1 = 0;
byte a2 = 0;
int req = 8; //assegna a req il piedino 11 (D8) through q1 (arduino high pulls request line low)
int dat = 2; // assegna a dat il piedino 5  (D2)   Linea Dati del calibro
int clk = 4; // assegna a clk il piedino 7  (D4)  clock del calibro
int i = 0; int j = 0; int k = 0;
byte mydata[14];
int num;
int sign;


void setup() {

  Wire.begin(4);                // Setta il numero Slave 2
  Wire.onRequest(manda);
  pinMode(req, OUTPUT);         // setta il pin 11 come OUTPUT
  pinMode(clk, INPUT_PULLUP);   // setta il pin 7 come Input abilitando la resistenza di pullup
  pinMode(dat, INPUT_PULLUP);   // setta il pin 5 come input abilitando la resistenza di pullup
  digitalWrite(req, LOW);       // porta a LOW il pin 11
}


void loop() {

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

    num = atol(buf); //converte la riga in long integer

    if (sign == 8) {   // se il segno è negativo fa diventare num negativo num=num-(2xnum)
      num = num - (2 * num);
    }

  }
}
void manda() {
  a1 = byte (num);
  a2 = byte ( num >> 8);
  Wire.write(a1);
  Wire.write(a2);
}
