/*  Prima prova di trasmissione su network
     Leggo il dato del comparatore this node
     lo spedisco al nodo 0 che sarà un arduino collegato al raspberry
     posso usare DEBUG per vedere cosa trasmetto
     posso usare TASTIERA per modificare il dato da tastiera a puro scopo test

     DEVO INSERIRE IL NUMERO CORRETTO DEL COMPARATORE E SE NECESSARIO DEL NODO 0

     Per lo schema dei collegamenti dell'arduino nano al comparatore vedi schema



*/

#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <printf.h>

/* encoder*/
#define encoderPin1  3
#define encoderPin2  2
#define risoluzioneEncoder  0.0125
//#define risoluzioneEncoder  0.05 /* INSERIRE QUI VALORE ENCODER uguale anche nel programma COTO 360/7200=0.05, se usiamo in quadratura va 360/28800
// http://www.sciamannalucio.it/arduino-encoder-conta-impulsi-giri-motore/
//   tecnicamente adesso la risoluzione si ottiene con 360/7200 perchè (vedi sezione encoder) usiamo un solo fronte per il conteggio e non 4 */
volatile int lastEncoded = 0;
volatile long encoderValue = 0;
float Angolo = 0;
long lastencoderValue = 0;

int lastMSB = 0;
int lastLSB = 0;
/* endocder */


// variabili network *********************************************
RF24 radio(9, 10);     // RF_NANO radio (pin fissi cablati sulla scheda)
RF24Network network(radio);          // Network uses that radio
const uint16_t this_node = 05;        // Address of our node in Octal format <<-- INSERIRE IL NUMERO DEL COMPARATORE
const uint16_t other_node = 00;       // Address of the other node in Octal format
struct payload_t {                  // Structure of our payload
  long num_sent;
  int control;
  int OffsetReq = 1;
};
payload_t pl;

struct ToNode05 {
  bool OffsetSetted = 0;
  int ValoreOffset = 0;
};
ToNode05 ToEnc;
// variabili network *********************************************
int VOff = 9999;

void setup() {
  /* encoder*/
  pinMode(encoderPin1, INPUT_PULLUP);
  pinMode(encoderPin2, INPUT_PULLUP);
  attachInterrupt(0, updateEncoder, CHANGE);
  attachInterrupt(1, updateEncoder, CHANGE);
  /* encoder*/

  // setting per network *****************************************
  SPI.begin();
  radio.begin();
  network.begin(/*channel*/ 90, /*node address*/ this_node);
  // setting per network *****************************************

  Serial.begin(38400);

}
//********************************************************************************************************
//********************************************************************************************************
//********************************************************************************************************

void loop() {
  network.update();
  RF24NetworkHeader header(/*to node*/ other_node);
  bool ok = network.write(header, &pl, sizeof(pl));
  delay(5);
  network.update();
  //network.read(header, &ToEnc, sizeof(ToEnc));
 // VOff= ToEnc.ValoreOffset;
  if (VOff != 9999) {
    double auxval = (encoderValue * risoluzioneEncoder) + ToEnc.ValoreOffset;
    double mod = fmod(auxval, 360.0);
    if (mod < 0.0) {
      mod += 360.0;
    }
    pl.num_sent = mod*100;

    pl.OffsetReq = 0;
    
    pl.control = pl.control + 1;
    if (pl.control > 10000) {
      pl.control = 0;
    }

    // while ( network.available() ) {     // Is there any incoming data?
    //RF24NetworkHeader header2(00);
    network.update();
    network.read(header, &ToEnc, sizeof(ToEnc));
  }

  Serial.print("num_sent ");
  Serial.print(pl.num_sent);
  Serial.print(", control ");
  Serial.print(pl.control);
  Serial.print(", OffsetReq ");
  Serial.print(pl.OffsetReq);
  Serial.print("     OffsetSetted ");
  Serial.print(ToEnc.OffsetSetted);
  Serial.print(" ValoreOffset ");
  Serial.print(ToEnc.ValoreOffset);
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
