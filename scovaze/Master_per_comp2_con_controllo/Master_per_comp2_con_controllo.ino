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

int lettura_1 = 8888;     //imposto la variabile a 8888 in modo da capire se il sensore ha fatto almeno 1 lettura
int lettura_2 = 8888;
int lettura_3 = 8888;
int lettura_4 = 8888;

struct payload_t {                 // Structure of our payload
  int num_sent;
  int alive;
};

int from; //per switch

int alive_1;
int alive_1prec;
int alive_2;
int alive_2prec;

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


  network.update();                  // Check the network regularly

  while ( network.available() ) {     // Is there anything ready for us?

    // ======================= Reading packets from all slave nodes ========================

    RF24NetworkHeader header;        // If so, grab it and print it out
    payload_t payload;
    network.read(header, &payload, sizeof(payload));
    from = header.from_node;
    switch (from) {
      case 01:
        alive_1 = payload.alive;
        lettura_1 = payload.num_sent;
        break;

      case 02:
        alive_2 = payload.alive;
        if (alive_2 != alive_2prec) {
          lettura_2 = payload.num_sent;
        } else {
          lettura_2 = 9999;
        }
        //alive_2prec = alive_2;
        break;

      case 03:
        lettura_3 = payload.num_sent;
        break;
      case 04:
        lettura_4 = payload.num_sent;
        break;
        delay(50);
    }


    // ======================= Call node 01 ================================================
    //   RF24NetworkHeader header3(node_2);

    //   bool ok = network.write(header3, &alive, sizeof(alive)); // Send the data
/*
    if (ok == true){
      lettura_1 = payload.num_sent;
    } else {
    lettura_1 = 7777;}
  }
*/
  /*
    #ifdef DEBUG
      debug();
    #endif
  */


  delay(1500);
  send_raspy();
}
}

/*void debug() {
  Serial.print(" from ");
  Serial.print(header.from_node);
  Serial.print("Received ");
  Serial.print(payload.num_sent);
  Serial.print(" nuovo_dato ");
  Serial.println(payload.nuovo_dato);
  }
*/
void send_raspy() {
  /*
    Serial.print("valore 1    ");//
    Serial.print(",");
    Serial.println("valore 2    ");//
    Serial.print("            ");//
    Serial.print(lettura_1);
    Serial.print(",");
    Serial.print("            ");//
    Serial.println(lettura_2);
    // Serial.print(",");
    // Serial.print(lettura_3);
    // Serial.print(",");
    // Serial.println(lettura_4);

    Serial.print("alive_1     ");
    Serial.print(alive_1);
    Serial.print(",");
    Serial.print("alive_2     ");
    Serial.println(alive_2);
    // Serial.print(",");
    // Serial.print("            ");
    // Serial.print(",");
    // Serial.println("            ");

    Serial.print("alive_1prec ");
    Serial.print(alive_1prec);
    Serial.print(",");
    Serial.print("alive_2prec ");
    Serial.println(alive_2prec);
    // Serial.print(",");
    // Serial.print("            ");
    // Serial.print(",");
    //  Serial.println("            ");
    Serial.println();
    Serial.println();

  */


 // Serial.print("valori inviati al raspberry");
  if (alive_1 != alive_1prec) {
    Serial.print(lettura_1);
  } else {
    Serial.print ("9999");
  }
  alive_1prec = alive_1;
  Serial.print(",");

  if (alive_2 != alive_2prec) {
    Serial.print(lettura_2);
  } else {
    Serial.print ("9999");
  }
  alive_2prec = alive_2;
  Serial.print(",");


  Serial.print(lettura_3);
  Serial.print(",");
  Serial.println(lettura_4);
}
