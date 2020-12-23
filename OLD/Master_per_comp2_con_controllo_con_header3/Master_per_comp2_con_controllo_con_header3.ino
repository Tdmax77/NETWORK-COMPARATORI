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
int lettura_1p = 999;
int lettura_2p = 999;
int lettura_3p = 999;
int lettura_4p = 999;
int alive;
int from;

struct payload_t {                 // Structure of our payload
  int num_sent;
  int alive2;
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


  network.update();                  // Check the network regularly

  RF24NetworkHeader header3(node_2);
  RF24NetworkHeader header4(node_3);
  RF24NetworkHeader header5(node_4);
  RF24NetworkHeader header2(node_1);
 RF24NetworkHeader header;

  while ( network.available() ) { // Is there anything ready for us?



    // ======================= Reading packets from all slave nodes ========================

 //   RF24NetworkHeader header;        // If so, grab it and print it out
    payload_t payload;
    network.read(header, &payload, sizeof(payload));
    from = header.from_node;
    switch (from) {
      case 01:
        lettura_1 = payload.num_sent;
        break;

      case 02:
        lettura_2 = payload.num_sent;
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
 //   RF24NetworkHeader header2(node_1);
    bool ok = network.write(header2, &alive, sizeof(alive)); // Send the data
    if (ok == true) {
      lettura_1p = lettura_1;
    } else {
      lettura_1p = 7777;
    }


    // ======================= Call node 02 ================================================
    // RF24NetworkHeader header3(node_2);
    bool ok2 = network.write(header3, &alive, sizeof(alive)); // Send the data
    if (ok2 == true) {
      lettura_2p = lettura_2;
    } else {
      lettura_2p = 7777;
    }
  }
 /*
    // ======================= Call node 03 ================================================
 //   RF24NetworkHeader header4(node_3);
    bool ok3 = network.write(header4, &alive, sizeof(alive)); // Send the data
    if (ok3 == true) {
      lettura_3p = lettura_3;
    } else {
      lettura_3p = 7777;
      Serial.print("ciao");
    }
  }
 
      // ======================= Call node 04 ================================================
      RF24NetworkHeader header5(node_4);
      bool ok4 = network.write(header5, &alive, sizeof(alive)); // Send the data
      if (ok4 == true) {
        lettura_4p = lettura_4;
      } else {
        lettura_4p = 7777;
      }
  */



  /*
    #ifdef DEBUG
      debug();
    #endif
  */
  //delay(1500);
  send_raspy();

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
  /*  if (alive_1 != alive_1prec) {
      Serial.print(lettura_1);
    } else {
      Serial.print ("9999");
    }
    alive_1prec = alive_1;

  */
  Serial.print(lettura_1p);
  Serial.print(",");
  Serial.print(lettura_2p);
  Serial.print(",");
  Serial.print(lettura_3p);
  Serial.print(",");
  Serial.println(lettura_4p);
}
