
// MASTER + COMPARATORE + ARDUINO NANO
#include <Wire.h>
byte f0 = 0;
byte f1 = 0;
byte f2 = 0;
byte f3 = 0;
byte f4 = 0;
byte f5 = 0;
int slave2 = 0;
int slave3 = 0;
int slave4 = 0;
int req = 8; //assegna a req il piedino 11 (D8) through q1 (arduino high pulls request line low)
int dat = 2; // assegna a dat il piedino 5  (D2)   Linea Dati del calibro
int clk = 4; // assegna a clk il piedino 7  (D4)  clock del calibro
int i = 0; int j = 0; int k = 0;
byte mydata[14];
int num;
int sign;


void setup() {

    Wire.begin();
    Serial.begin(38400);
    

    pinMode(req, OUTPUT);         // setta il pin 11 come OUTPUT
    pinMode(clk, INPUT_PULLUP);   // setta il pin 7 come Input abilitando la resistenza di pullup
    pinMode(dat, INPUT_PULLUP);   // setta il pin 5 come input abilitando la resistenza di pullup
    digitalWrite(req,LOW);        // porta a LOW il pin 11
}


void loop() { 
      
      digitalWrite(req, HIGH);      // genera set request portando il pin 11 a HIGH

      for(i = 0; i < 13; i++ ) {
      k = 0;

      for (j = 0; j < 4; j++) {
      while( digitalRead(clk) == LOW) {// hold until clock is high
}

      while( digitalRead(clk) == HIGH) {// hold until clock is low
}

      bitWrite(k, j, (digitalRead(dat) & 0x1)); // read data bits, and reverse order )
}
      // estrai i dati da mydata
      mydata[i] = k;
}

      // sign = mydata[4]    decimal = mydata[11]  units = mydata[12];
      sign = mydata[4];
      
      // assemble measurement from bytes
      char buf[8];

      for(int lp=0;lp<6;lp++) {

      buf[lp]=mydata[lp+5]+'0';
      buf[6]=0;

      num=atol(buf); //converte la riga in long integer
      
    if (sign == 8) {   
        num = num-(2*num);
        }
    
    Serial.print(num);		//manda in seriale il vaore del comparatore letto dall'arduino master
    Serial.print(",");		//manda in seriale il segno , come separatore
    
    Wire.requestFrom(2,2);	//richiede 2 Bytes dall'arduino SLAVE 2
    f0 = Wire.read();
    f1 = Wire.read();
    slave2 = ((f1 << 8) | f0);
    Serial.print(slave2);
    Serial.print(",");		//manda in seriale il segno , come separatore

    Wire.requestFrom(3,2);	//richiede 2 Bytes dall'arduino SLAVE 3
    f2 = Wire.read();
    f3 = Wire.read();
    slave3 = ((f3 << 8) | f2);
    Serial.print(slave3);
    Serial.print(",");		//manda in seriale il segno , come separatore
    
    Wire.requestFrom(4,2);	//richiede 2 Bytes dall'arduino SLAVE 4
    f4 = Wire.read();
    f5 = Wire.read();
    slave4 = ((f5 << 8) | f4);
    Serial.println(slave4);	
    
    delay(0);
}
}
