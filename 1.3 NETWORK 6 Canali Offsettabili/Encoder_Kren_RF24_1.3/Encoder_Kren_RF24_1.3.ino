/*  This program is a new release of Kren program.
 *  The program read data from an encoder and send it to the master that could be: 
 *  - the COTO display 
 *  - the BSC unit powered by a raspberry
 *  - a tablet with BSC Dongle 
 *  
 *  On the first loop the program sends to the master an offset request: we need it because, usually, the crankshaft is not in the mechanical 0°,
 *  so we need only to move it to a integer value of rotation readed on the index and to insert this value, so it will start to transmit correct position.
 *  
 *  After that the program will send continuously the data to the master, if for some reasons it shutdown (battery, someone close it etc) the master notice that 
 *  and return a 9999 value instead of the angle. 
 *  when encoder will rised on, it will asks again for the offset.
 *  
 *  N.B. the offset value is stored into the Encoder, so if master fails and will be restarded, it will receive correct angle whitout any request.
 *  
 *  
 Rel. 1.3


*/

#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <printf.h>

#define SERIALE

/* encoder*/
#define encoderPin1  3
#define encoderPin2  2
#define risoluzioneEncoder  0.0125
//#define risoluzioneEncoder  0.05 /* INSERIRE QUI VALORE ENCODER uguale anche nel programma COTO 360/7200=0.05, se usiamo in quadratura va 360/28800
// http://www.sciamannalucio.it/arduino-encoder-conta-impulsi-giri-motore/
volatile int lastEncoded = 0;
volatile long encoderValue = 0;
float Angolo = 0;
long lastencoderValue = 0;

int lastMSB = 0;
int lastLSB = 0;
/* endocder */


// variabili network *********************************************
RF24 radio(9,10);     //  // Coto(7,8)  Kren e RF-NANO with ext ant. (9,10) RF-NANO without ext ant (10,9)
RF24Network network(radio);         // Network uses that radio
const uint16_t this_node = 05;      // Address of our node in Octal format                    <<<<<<<<<<<<<<<<<-- INSERT CHANNEL
const uint16_t other_node = 00;     // Address of the other node in Octal format
struct payload_t {                  // Structure of our payload
  long num_sent;                    // value sended to master node
  int control;                      // this is a control variable that continuously incrases on every data transmission, i need it for to understand if COMP is alive
  int OffsetReq = 5;                // same number of the channel where encoder is linked     <<<<<<<<<<<<<<<<< -- INSERT CHANNEL without 0
  int  VO = 999999;                   // offset value
};
payload_t pl;


// variabili network *********************************************
int VOff = 9999;                    // offset value variable

void setup() {
  /* encoder*/                        // settings for encoder 
  pinMode(encoderPin1, INPUT_PULLUP);
  pinMode(encoderPin2, INPUT_PULLUP);
  attachInterrupt(0, updateEncoder, CHANGE);
  attachInterrupt(1, updateEncoder, CHANGE);
  /* encoder*/

  // setting per network *****************************************
  SPI.begin();
  radio.begin();
  network.begin(/*channel*/ 80, /*node address*/ this_node); // 80 orange 85 blue 90 black    <<<<<<<<<<<<<<<<< -- SET THIS VALUE DEPENDING OF WHICH CHANNEL YOU ARE USING 
  // setting per network *****************************************

  Serial.begin(38400);

}
//********************************************************************************************************
//********************************************************************************************************
//********************************************************************************************************

void loop() {
  network.update();                                   // Update network 

  RF24NetworkHeader header(00);                       // set master's address on the network    <<<<<<<<<<<<<<<<< -- SET THIS VALUE DEPENDING OF WHICH CHANNEL YOU ARE USING
  network.write(header, &pl, sizeof(pl));             // on the first loop it sends to  master the  OffsetReq and  VO = 9999
  RF24NetworkHeader header5(05);                      // set this node address                  <<<<<<<<<<<<<<<<< -- SET THIS VALUE DEPENDING OF WHICH CHANNEL YOU ARE USING 
  network.read(header5, &pl, sizeof(pl));             // leggo le variabili del payload 
  VOff = pl.VO;                                       // The offset value i will send is = to VOff

  if  ((VOff != 9999)) {                                           // If VOff is != 999999 (it means that it is already setted) 
    double auxval = (encoderValue * risoluzioneEncoder) + pl.VO;     // auxval = data readed from encoder + offset
    double mod = fmod(auxval, 360.0);                                // if data exceed 360 restart from 0  
    if (mod < 0.0) {
      mod += 360.0;
    }
    pl.num_sent = mod * 100;                                      // multiply data for to send it as integer
    pl.OffsetReq = 0;                                             // clear offset request
    pl.control = pl.control + 1;                                  // update control variable (needed from master to understand if the data is up to date or not 
    if (pl.control > 10000) {
      pl.control = 0;
    }
    network.write(header, &pl, sizeof(pl));                       // send data to master node
  } else {                                                        // otherwise if VO is still not setted asks for him again
    encoderValue = 0;                                             // clear data readed from encoder 
    pl.OffsetReq = 5;                                             // offset request remain active  <<<<<<<<<<<<<<<<< -- SET THIS VALUE DEPENDING OF WHICH CHANNEL YOU ARE USING
    pl.control = pl.control + 1;                                  // update control variable (needed from master to understand if the data is up to date or not 
    if (pl.control > 10000) {
      pl.control = 0;
    }
    network.write(header, &pl, sizeof(pl));                       // send data to master node

  }

  Serial.print("num_sent ");
  Serial.print(pl.num_sent);
  Serial.print(", control ");
  Serial.print(pl.control);
  Serial.print(", OffsetReq ");
  Serial.print(pl.OffsetReq);

  Serial.print(" VO ");
  Serial.print(pl.VO);
  Serial.println();
}

//== == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == ==
//== == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == ==
//== == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == ==
//== == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == ==
//== == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == ==
//== == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == ==





/* routine encoder, non toccare!*/
void updateEncoder() {
  int MSB = digitalRead(encoderPin1); //MSB = most significant bit
  int LSB = digitalRead(encoderPin2); //LSB = least significant bit

  int encoded = (MSB << 1) | LSB; //converting the 2 pin value to single number
  int sum  = (lastEncoded << 2) | encoded; //adding it to the previous encoded value

  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderValue ++; // modificato per non lavorare in quadratura
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderValue --; // modificato per non lavorare in quadratura

  lastEncoded = encoded; //store this value for next time
}
/* routine encoder, non toccare!*/
