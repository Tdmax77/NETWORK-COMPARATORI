/*This is the code for the master in our project " WARTSILA WIRELESS COMPARATORS and ENCODER (Kren) READING SYSTEM "
   Release 1.3

   The target is to read 6 channels that continuously send data to a central unit.


   WARNING: LET'S START FROM THE ASSUMPTION THAT THE VALUES READED FROM THE NODES (COMPARATORS) ARE INCLUDED BETWEEN 0 AND 1360
   OTHERWISE WE COULD HAVE ABNORMAL BEHAVIOR

  How it works:

   Every channel has a transmitter and continuosly sends data to the network.
   Every node ( sensor + transmitter) sends  a payload with 5 informations:
   header.from_node    is its adress
   payload.num_sent    is the value readed from sensor
   payload.control     is a number that increases on every cycle. We need it for to decide if the the node is active or not. If it fails, control will not increase anymore.
   payload.OffsetReq   is a request to set a starting value, each channel send his number so the master can understand which node is asking it
   payload.VO          is the Offset value setted by master and sended to channel


   The central unit (for which this code is written), on every cycle read a packet from the network and analyzes it:
   It reads who is the sender and the data, then if they are valid, stores them into data array.
   The reading from the network is not sequential, it is random so i need to create the array for to assure that it read all the nodes of my net (all six channels)
   On every cycle i incrase the counter of all nodes excepted the one i read. If a node is down his counter will increase untill it will reach the maximum allowed value (Nnosignal)
   In this case the the data sended to serialmonitor or raspberry will be 888888.

   Example: if i read ch1, ch2, ch4, ch5, ch6 correctly but ch3 is power off i will read something like:
   0000,0100,8888,0099,1245,0010,0
   0000,0101,8888,0099,1245,0011,0

   In this 2 lines i see that ch1 is not changed , ch2 and ch6 are increased by 1 cents, comp3 is not reachable, ch4 and ch5 are not changed.
   On every cycle the code sends all the data to the raspberry which is demanded to control graphic interface and data storage.

   Each channel can be used with different kind of devices:
   comparators
   encoders (like Kren :-)
   pressure sensors
   and any other sensors.

   If the device attached on the channel need a offset value (like the encoder) it will ask it sending his number (1-6) to the master using the OffsetReq.
   example:
   The encoder (kren) is the node_5, it needs to be aligned with flywheel index using an OFFSET Value (VO), otherwise you need to rotate crankshaft to 0° indexed value.
   Kren asks for an Offset value when it starts (OffsetReq = 5), his Offset value is setted to 9999 (VO = 9999).
   So Master receive the request from channel 5 and wait for this value from user. Then it stores the data in val, send it to encoder (case 5)  and esc from Offset task.

   Because of a "strange problem" it asks 4-5 times the offset value (probably a timing problem) so i introduced a condition y.
   y is setted to 1 all the time,this is a condition for enter into Offset task, after the task y becomes 0 for only 0,1 sec so the program can't enter again in Offset task.
   After 0,1 sec y goes to 1 so, if encoder is closed and reopen, master could enter into Offset task again.

   Powered by Massimo Riosa

*/



#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>

//  select which device to use for to set Offset value:       <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
//  You must select only ONE!                                 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
//#define RASPY       // master is linked to raspberry so you will use Rpi to set values and read data.
#define SERIALE     // master is standalone so you will use serialmonitor to set values and read data.

//offset from serialmonitor *******************************************************************
byte idx;
char caratteri[4]; // max 3 chars + end string 0x00
int numero;
//offset from serialmonitor *******************************************************************



RF24 radio(9/*CE*/, 10/*CSN*/);             // Coto and Kren (7,8)  RF-NANO with ext ant. (9,10) RF-NANO without ext ant (10,9)

RF24Network network(radio);     // Network uses that radio
const uint16_t this_node = 00;  // Address of our node in Octal format ( 04,031, etc)   this is the master (raspy display)
const uint16_t node_1 = 01;     // Address of the other node in Octal format            this is CHANNEL 1 usually COMP_1
const uint16_t node_2 = 02;     // Address of the other node in Octal format            this is CHANNEL 2 usually COMP_2
const uint16_t node_3 = 03;     // Address of the other node in Octal format            this is CHANNEL 3 usually COMP_3
const uint16_t node_4 = 04;     // Address of the other node in Octal format            this is CHANNEL 4 usually COMP_4
const uint16_t node_5 = 05;     // Address of the encoder node in Octal format          this is CHANNEL 5 usually ENCODER
const uint16_t node_6 = 06;     // Address of the encoder node in Octal format          this is CHANNEL 6 usually PRESSURE SENSOR

struct payload_t {              // Structure of  payload received from slaves
  long num_sent;                // this is the data readed from COMP, it is the value readed from Mitutoyo Comparator
  int control;                  // this is a control variable that continuously incrases on every data transmission, i need it for to understand if COMP is alive
  int OffsetReq;                // if 1 Offset is requested, used only by node_5
  int VO;                       // Offset value
};
payload_t payload;

const int N_rec (6);      // define number of slaves (receiver = 4 COMP + 1 ENCODER)
long dummy_val (888888);  // define a value that will be printed if COMP fails
payload_t data[N_rec];    // create an array data
int counter [N_rec];      // Array defined for validation of data (ses main loop)
int Nnosignal (30);       // number of acceptable failed reads

int val = 0;              // This is the value readed on the index of flywheel by user and insered by the OFFSET Request task
unsigned long tempo;      // need for the y counter
int y = 1;                // need for block OFFSET Request loop

//////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// S E T U P  ////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

void setup(void)
{
  Serial.begin(38400);                                       // initialize serial for to send data to raspberry
  SPI.begin();                                               // initialize SPI
  radio.begin();                                             // initialize radio module
  network.begin(/*channel*/ 90, /*node address*/ this_node); // start network

  for (size_t ii = 0; ii < N_rec; ii++) {                    // initialize all values in data array
    data[ii].num_sent = 7777;                                // num_sent starts from 7777
    data[ii].control = 0;                                    // control starts from 0
    counter[ii] = 0;                                         // reset counter
  }
}




//////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// L O O P //////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

void loop(void) {

  if (millis() > tempo + 300) {                              // y is always 1 so i can enter into Offset task,
    y = 1;                                                   // when Offset is setted y become 0 so the Offset task can't loop.
  } else {                                                   // after 100 milliseconds y become 1 so it is possible to execute Offset task.
    y = 0;
  }


  network.update();                                          // looking for news on the network
  while ( network.available() ) {                            // if network is up  ... I need at least one slave active for to keep active the network, otherwise no network will be available

    int cnt (0);                                             // reset counter
    RF24NetworkHeader header;                                // declare the address of this node: every packet sent from slaves to header will be readed from this node (00, the master)

    network.read(header, &payload, sizeof(payload));         // read the packet netwotk sent for this node
    size_t node_id = header.from_node;                       // create a variable node_id that stores the source of the packet ( who is sending it to me? COMP1 with 01 , COMP2 with 02 and so on..


    if ((payload.OffsetReq != 0)  && (payload.OffsetReq <= 6)  && (payload.VO = 9999) && (y == 1)) {      // if Offset is request i start the Offset task
      // every channel can asks for offset sending his number, so value could be 1-6
      // if 0 no offset requested
#ifdef RASPY
      raspy();
#endif
      // ROUTINE INSERIMENTO OFFSET ////////////////////////////////////////////////////////////////////////////////////
#ifdef SERIALE
      Serial.print("insert Offset value for channel: ");
      Serial.println (payload.OffsetReq);
#endif
      idx = 0;                                //reset a counter
      while (1) {                             // asks for number in serial
        if ( Serial.available() ) {
          caratteri[idx] = Serial.read();
          if (caratteri[idx] == 0x0D) break;  // if CR received, esc
          idx++;
          if (idx > 3) break;                 // if num of characters is >3 esc
        }
      }
      caratteri[idx] = 0x00;                  // terminate the string
      val = atoi(caratteri);                  // Convert string to integer
#ifdef SERIALE
      Serial.print("valore registrato ");
      Serial.println(val);
#endif
      // ROUTINE INSERIMENTO DA TERMINALE  ////////////////////////////////////////////////////////////////////////////////////

      payload.VO = val;                                                     // assign offset value to payload
      switch (payload.OffsetReq) {                                          // send the VO to the correct channel
        case 1:
          {
            RF24NetworkHeader header1(01);                                        // define encoder network address
            network.write(header1, &payload, sizeof(payload));                    // send payload to channel
          }
          break;
        case 2:
          {
            RF24NetworkHeader header2(02);                                        // define encoder network address
            network.write(header2, &payload, sizeof(payload));                    // send payload to channel
          }
          break;
        case 3:
          {
            RF24NetworkHeader header3(03);                                        // define encoder network address
            network.write(header3, &payload, sizeof(payload));                    // send payload to channel
          }
          break;
        case 4:
          {
            RF24NetworkHeader header4(04);                                        // define encoder network address
            network.write(header4, &payload, sizeof(payload));                    // send payload to channel
          }
          break;
        case 5:
          {
            RF24NetworkHeader header5(05);                                        // define encoder network address
            network.write(header5, &payload, sizeof(payload));                    // send payload to channel
          }
          break;
        case 6:
          {
            RF24NetworkHeader header6(06);                                        // define encoder network address
            network.write(header6, &payload, sizeof(payload));                    // send payload to channel
          }
          break;

        default:
          break;
      }

      payload.OffsetReq = 0;                                                // reset the Offset request

      delay(200);                                                           // little delay
      tempo = millis();                                                     // set variable for y status
      y = 0;                                                                // put y to 0

    }


    if (data[node_id - 1].control != payload.control) {      // if control readed from network is different from control stored i assume that the packet is new, so the slave is alive and the data is valid
      data[node_id - 1].num_sent = payload.num_sent;         // so i store readed values in data.num_sent ( this is comparator's readed value from slave)
      data[node_id - 1].control = payload.control;           // update data.control to new value received  (this is the counter sent from slave that increase on every sending)
      counter[node_id - 1] = 0;                              // reset couter of the node i readed in this cycle.

    } else {
      //data[node_id - 1].num_sent = dummy_val;                // if control is not increasing the slave is not alive so i haven't a valid num_sent, therefore i set num_sent as 8888
      counter[node_id - 1] *= 1;                                                      // A valid data is included in a range from 0 to 1360 that is the values readed from Mitutoyo comparator (0-1360 cent)
    }
    increase_counter(node_id - 1);                           // on every cycle it increases all the counters ecxcept that of the node it just read
    fix_values();                                            // if the couter reach the max number of fails setted with Nnosignal variable, data.num_sent will be setted to 8888
    raspy();                                                 // this routine send to serial the data as.  COMP1,COMP2,COMP3,COMP4  and then go to new line
  }
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////  R O U T I N E S  //////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



void raspy() {                                               // this routine send to serial the data as.  CH1,CH2,CH3,CH4  and then go to new line
  for (size_t ii = 0; ii < N_rec; ii++) {
    if (ii != 0) {
      Serial.print(",");
    }
    Serial.print(data[ii].num_sent);
  }
  Serial.print(",");
  Serial.print(payload.OffsetReq);
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
