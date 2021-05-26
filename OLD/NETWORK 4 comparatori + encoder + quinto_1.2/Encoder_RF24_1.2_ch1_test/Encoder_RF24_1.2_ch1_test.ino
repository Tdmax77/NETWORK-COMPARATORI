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
const uint16_t this_node = 01;        // Address of our node in Octal format <<-- INSERIRE IL NUMERO DEL COMPARATORE
const uint16_t other_node = 00;       // Address of the other node in Octal format
struct payload_t {                  // Structure of our payload
  long num_sent;
  int control;
  int OffsetReq = 1;
  int VO = 9999;
};
payload_t pl;


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
  network.update();                                   // aggiorno

  RF24NetworkHeader header(00);                       // imposto indirizzo del master
  network.write(header, &pl, sizeof(pl));             // scrivo al master che OffsetReq = 1 e VO = 9999
  RF24NetworkHeader header1(01);                      // imposto idirizzo di questo nodo
  network.read(header1, &pl, sizeof(pl));             // leggo le variabili del payload
  VOff = pl.VO;

  if  ((VOff != 9999)) {                                             // Se Voff è stato impostato e risulta diverso da 9999
    double auxval = (encoderValue * risoluzioneEncoder) + pl.VO;     // sommo il valore dell'OFFset VO alla lettura
    double mod = fmod(auxval, 360.0);                                // se il dato supera i 360 ritorno a zero    
    if (mod < 0.0) {
      mod += 360.0;
    }
    pl.num_sent = mod * 100;                                      // moltiplico il dato per avere un numero intero da spedire
    pl.OffsetReq = 0;                                             // azzero la richiesta di Offset
    pl.control = pl.control + 1;                                  // aumento la variabile contatore di controllo
    if (pl.control > 10000) {
      pl.control = 0;
    }
    network.write(header, &pl, sizeof(pl));                         // invio il dato letto al master 

  } else {                                                          // altrimenti se il VO è ancora a 9999
    encoderValue = 0;                                               // azzero la lettura dell'encoder in modo da partire solo con il valore di offset impostato al prossimo ciclo
    pl.OffsetReq = 1;                                               // resta attiva la richiesta di offset
    pl.control = pl.control + 1;
    if (pl.control > 10000) {
      pl.control = 0;
    }
    network.write(header, &pl, sizeof(pl));                         // riscrivo la richiesta di offset verso il  master

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
