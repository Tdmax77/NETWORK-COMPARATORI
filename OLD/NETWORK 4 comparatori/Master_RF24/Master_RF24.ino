/*This is the code for the master in our project " WARTSILA WIRELESS COMPARATORS READING SYSTEM "
 * 
 * The target is to read 4 comparators that continuously send data to a central unit.
 * 
 * 
 * WARNING: LET'S START FROM THE ASSUMPTION THAT THE VALUES READED FROM THE NODES (COMPARATORS) ARE INCLUDED BETWEEN 0 AND 1360
 * OTHERWISE WE COULD HAVE ABNORMAL BEHAVIOR
 *
 *How it works:
 *
 * Every comparator has a transmitter and continuosly sends data to the network.
 * Every node ( comparator + transmitter) sends  a payload with 3 informations: 
 * header.from_node  is its adress 
 * payload.num_sent  is the value readed from comparator 
 * payload.control   is a number that increases on every cycle. We need it for to decide if the the node is active or not. If it fails, control will not increase anymore.
 * 
 * The central unit (for which this code is written), on every cycle read a packet from the network and analyzes it:
 * It reads who is the sender and the data, then if they are valid, stores them into data array.
 * The reading from the network is not sequential, it is random so i need to create the array for to assure that i read all the nodes of my net (all the comparators)
 * On every cycle i incrase the counter of all nodes excepted the one i read. If a node is down his counter will increase untill it will reach the maximum allowed value (Nnosignal)
 * In this case the the data sended to raspberry will be 8888.
 * 
 * Example: if i read comp1, comp2, comp4 correctly but comp3 is power off i will read something like:
 * 0000,0100,8888,0099
 * 0000,0101,8888,0099
 * 
 * In this 2 lines i see that comp 1 is not changed , comp2 is increased by 1 cents, comp3 is not reachable, comp4 is not changed.
 * 
 * On every cycle the code sends all the data to the raspberry which is demanded to control graphic interface and data storage.
 * 
 * 
 * Powered by Massimo Riosa and Michael Magris 
 * 
 * Release 1.0
*/



#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>

//#define DEBUG

RF24 radio(10, 9);               // nRF24L01(+) radio attached using Getting Started board n.b. i can't modify them because of hardware design on modules RF-NANO

RF24Network network(radio);     // Network uses that radio
const uint16_t this_node = 00;  // Address of our node in Octal format ( 04,031, etc)   this is the master (raspy display)
const uint16_t node_1 = 01;     // Address of the other node in Octal format            this is COMP_1
const uint16_t node_2 = 02;     // Address of the other node in Octal format            this is COMP_2
const uint16_t node_3 = 03;     // Address of the other node in Octal format            this is COMP_3
const uint16_t node_4 = 04;     // Address of the other node in Octal format            this is COMP_4

struct payload_t {              // Structure of  payload received from slaves
  long num_sent;                 // this is the data readed from COMP, it is the value readed from Mitutoyo Comparator
  int control;                  // this is a control variable that continuously incrases on every data transmission, i need it for to understand if COMP is alive
};


const int N_rec (4);      // define number of slaves (receiver = COMP)
int dummy_val (8888);     // define a value that will be printed if COMP fails
payload_t data[N_rec];    // create an array data
int counter [N_rec];      // Array defined for validation of data (ses main loop)
int Nnosignal (30);       // number of acceptable failed reads



void setup(void)
{
  Serial.begin(38400);                                       // initialize serial for to send data to raspberry
  SPI.begin();                                               // initialize SPI
  radio.begin();                                             // initialize radio module
  network.begin(/*channel*/ 90, /*node address*/ this_node); // start network

  for (size_t ii = 0; ii < N_rec; ii++) {                    // initialize values in data array
    data[ii].num_sent = 7777;                                // num_sent starts from 7777
    data[ii].control = 0;                                    // control starts from 0 
    counter[ii] = 0;                                         // reset counter
  }

}


void loop(void) {

  network.update();                                          // looking for news on the network
  while ( network.available() ) {                            // if network is up  ... I need at least one slave active for to keep active the network, otherwise no network will be available

    int cnt (0);                                             // reset counter
    RF24NetworkHeader header(00);                                // declare the address of this node: every packet sent from slaves to header will be readed from this node (00, the master)
    payload_t payload;                                       // create object payload
    network.read(header, &payload, sizeof(payload));         // read the packet netwotk sent for this node
    size_t node_id = header.from_node;                       // create a variable node_id that stores the source of the packet ( who is sending it to me? COMP1 with 01 , COMP2 with 02 and so on..

    if (data[node_id - 1].control != payload.control) {      // if control readed from network is different from control stored i assume that the packet is new, so the slave is alive and the data is valid
      data[node_id - 1].num_sent = payload.num_sent;         // so i store readed values in data.num_sent ( this is comparator's readed value from slave)
      data[node_id - 1].control = payload.control;           // update data.control to new value received  (this is the counter sent from slave that increase on every sending)
      counter[node_id - 1] = 0;                              // reset couter of the node i readed in this cycle.
    } else {
      data[node_id - 1].num_sent = dummy_val;                // if control is not increasing the slave is not alive so i haven't a valid num_sent, therefore i set num_sent as 8888
                                                             // A valid data is included in a range from 0 to 1360 that is the values readed from Mitutoyo comparator (0-1360 cent)
    }
    increase_counter(node_id - 1);                           // on every cycle it increases all the counters ecxcept that of the node it just read
    fix_values();                                            // if the couter reach the max number of fails setted with Nnosignal variable, data.num_sent will be setted to 8888
    raspy();                                                 // this routine send to serial the data as.  COMP1,COMP2,COMP3,COMP4  and then go to new line
  }
}


void raspy() {                                               // this routine send to serial the data as.  COMP1,COMP2,COMP3,COMP4  and then go to new line
  for (size_t ii = 0; ii < N_rec; ii++) {
    if (ii != 0) {
      Serial.print(",");
    }
    Serial.print(data[ii].num_sent);
  }
  Serial.println("");
}


void increase_counter(size_t id) {                            // on every cycle it increases all the counters ecxcept that of the node it just read
    for (size_t ii = 0; ii < N_rec; ii++) {
    counter[ii]++;
  }
  counter[id]--;
}


void fix_values() {                                           // if the couter reach the max number of fails setted with Nnosignal variable, data.num_sent will be setted to 8888
  for (size_t ii = 0; ii < N_rec; ii++) {
        if (counter[ii] >= Nnosignal) {
      data[ii].num_sent = dummy_val;
      counter[ii] = 0;
    }
  }
}
