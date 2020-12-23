/*master per progetto comparatori radio con NRF24Network*/

#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>

//#define DEBUG

RF24 radio(10, 9);               // nRF24L01(+) radio attached using Getting Started board

RF24Network network(radio);      // Network uses that radio
const uint16_t this_node = 00;    // Address of our node in Octal format ( 04,031, etc)
const uint16_t node_1 = 01;   // Address of the other node in Octal format
const uint16_t node_2 = 02;   // Address of the other node in Octal format
const uint16_t node_3 = 03;   // Address of the other node in Octal format
const uint16_t node_4 = 04;   // Address of the other node in Octal format

int lettura_1 = 8888;
int lettura_2 = 8888;
int lettura_3 = 8888;
int lettura_4 = 8888;
int lettura_1p = 5555;
int lettura_2p = 5555;
int lettura_3p = 5555;
int lettura_4p = 5555;
int control_1;
int control_2;
int control_3;
int control_4;
int control_1o;
int control_2o;
int control_3o;
int control_4o;


struct payload_t {                 // Structure of our payload
  int num_sent;
  int control;
};


void setup(void)
{
  Serial.begin(38400);
#ifdef DEBUG
  Serial.println("Ricevitore v0.1");
#endif
  SPI.begin();
  radio.begin();
  network.begin(/*channel*/ 90, /*node address*/ this_node);

}

















void loop(void) {

  control_1o = control_1;
  control_2o = control_2;
  control_3o = control_3;
  control_4o = control_4;


#ifdef DEBUG
  Serial.println("check 10 riparte il loop");
#endif DEBUG

  network.update();                  // Check the network regularly
#ifdef DEBUG
  Serial.println("check 20 net update");
#endif DEBUG

  while ( network.available() ) {     // Is there anything ready for us?
#ifdef DEBUG
    Serial.println("check 22 sono nel loop del network available");
#endif DEBUG

 //   control_1o = control_1;
  //  control_2o = control_2;
 //   control_3o = control_3;
//    control_4o = control_4;
#ifdef DEBUG
    Serial.println("check 24 ho impostato le variabili di controllo all'ultimo valore");
#endif DEBUG

#ifdef DEBUG
    Serial.println("check 30 net available");
#endif DEBUG
    RF24NetworkHeader header;        // If so, grab it and print it out
    payload_t payload;



    
    network.read(header, &payload, sizeof(payload));
#ifdef DEBUG
    Serial.println("check 40 ho letto il payload ");
#endif DEBUG


    switch (header.from_node) {


      case 01: //=============================================================================================
        lettura_1 = payload.num_sent;
#ifdef DEBUG
        Serial.println("check 50 HO LETTO IL DATO  1");
#endif DEBUG
        control_1 = payload.control;
#ifdef DEBUG
        Serial.println("check 60 HO LETTO LA VARIABILE CONTATORE  1");
#endif DEBUG
        break;


      case 02: //=============================================================================================
        lettura_2 = payload.num_sent;
#ifdef DEBUG
        Serial.println("check 70 HO LETTO IL DATO  2");
#endif DEBUG
        control_2 = payload.control;
#ifdef DEBUG
        Serial.println("check 80 HO LETTO LA VARIABILE CONTATORE  2");
#endif DEBUG
        break;


      case 03: //=============================================================================================
        lettura_3 = payload.num_sent;
#ifdef DEBUG
        Serial.println("check 90 HO LETTO IL DATO  3");
#endif DEBUG
        control_3 = payload.control;
#ifdef DEBUG
        Serial.println("check 100 HO LETTO LA VARIABILE CONTATORE  3");
#endif DEBUG
        break;


      case 04: //=============================================================================================
        lettura_4 = payload.num_sent;
#ifdef DEBUG
        Serial.println("check 90 HO LETTO IL DATO  4");
#endif DEBUG
        control_4 = payload.control;
#ifdef DEBUG
        Serial.println("check 100 HO LETTO LA VARIABILE CONTATORE  4");
#endif DEBUG
        break;

    }








// PARTE SOPRA TUTTO OK NON TOCCARE




    // ENNESIMA PROVA DI CONTROLLO CONCATENATO

    if ((control_1 != control_1o) && (header.from_node == 01)) {
      lettura_1p = lettura_1;
      raspy();
#ifdef DEBUG
      Serial.println("check 110 confronto letture 1 e 1p");
#endif DEBUG
    } else if ((control_1 == control_1o) && (header.from_node == 01)) {
      lettura_1p = 7777;
      raspy();

#ifdef DEBUG
      Serial.println("check 120 else lettura 1");
#endif DEBUG
    }

raspy();

    if ((control_2 != control_2o) && (header.from_node == 02)) {
      lettura_2p = lettura_2;
      raspy();

#ifdef DEBUG
      Serial.println("check 130 confronto letture 2 e 2p");
#endif DEBUG
    } else if ((control_2 == control_2o) && (header.from_node == 02)) {
      lettura_2p = 7777;
      raspy();

#ifdef DEBUG
      Serial.println("check 140 else lettura 2");
#endif DEBUG
    }




    if (control_3 != control_3o) {
      lettura_3p = lettura_3;
      control_1++;
      control_2++;
      control_4++;

#ifdef DEBUG
      Serial.println("check 150 confronto letture 3 e 3p");
#endif DEBUG
    } else {
      lettura_3p = 7777;
      control_1++;
      control_2++;
      control_4++;

#ifdef DEBUG
      Serial.println("check 160 else lettura 3");
#endif DEBUG
    }




    if (control_4 != control_4o) {
      lettura_4p = lettura_4;
      control_1++;
      control_2++;
      control_3++;

#ifdef DEBUG
      Serial.println("check 170 confronto letture 4 e 4p");
#endif DEBUG
    } else {
      lettura_4p = 7777;
      control_1++;
      control_2++;
      control_2++;

#ifdef DEBUG
      Serial.println("check 180 else lettura 4");
#endif DEBUG
    }
raspy();
  }
    

} 



void raspy() {
  Serial.print(lettura_1p);
  Serial.print(",");
  Serial.print(lettura_2p);
  Serial.print(",");
  Serial.print(lettura_3p);
  Serial.print(",");
  Serial.println(lettura_4p);
}
