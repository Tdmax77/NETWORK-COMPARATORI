/*master per progetto comparatori radio con NRF24Network*/

#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>


RF24 radio(10,9);                // nRF24L01(+) radio attached using Getting Started board 

RF24Network network(radio);      // Network uses that radio
const uint16_t this_node = 00;    // Address of our node in Octal format ( 04,031, etc)
const uint16_t node_1 = 01;   // Address of the other node in Octal format
const uint16_t node_2 = 02;   // Address of the other node in Octal format
const uint16_t node_3 = 03;   // Address of the other node in Octal format
const uint16_t node_4 = 04;   // Address of the other node in Octal format

struct payload_t {                 // Structure of our payload
  int num_sent;
};


void setup(void)
{
  Serial.begin(115200);
  Serial.println("RF24Network/examples/helloworld_rx/");
 
  SPI.begin();
  radio.begin();
  network.begin(/*channel*/ 90, /*node address*/ this_node);

   // RF24Module
   //radio.setDataRate(RF24_2MBPS);
   // RF24Module
}

void loop(void){
  
  network.update();                  // Check the network regularly

  
  while ( network.available() ) {     // Is there anything ready for us?
    
    RF24NetworkHeader header;        // If so, grab it and print it out
    payload_t payload;
    network.read(header,&payload,sizeof(payload));
    Serial.print("Received packet ");
    Serial.println(payload.num_sent);

  }
}
