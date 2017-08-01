//////////////////////////////////////////////////////////////////////////////////////////
// The Central Node: CSMA/CA Scheme in BRIX2                                            //
//                                                                                      //
// The central node starts with a "RESET" broadcast msg to all satellite nodes.         //
//                                                                                      //
// The DATA FRAME FORMAT used in this example:                                          //
//                                                                                      //
// |     Byte 0     |    Byte 1  | Byte 2 |     Byte 3 - 9      |                       //
// | Sender Address | Frame Ctrl | Length |    Data Payload     |                       //
//                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

#include <Wire.h> // If you need Wire.h ALWAYS include it first!
#include <InertialSensor.h>
#include <BRIX2.h>

// Frame Control Definitions (choose specific No.)
#define RESET_FRAME       0x01
// Sent to satellite nodes by the center node for joining the communication
#define NWK_JOIN_FRAME    0x02  
// Received from satellite node (Confirmation frame)  
#define NWK_JOIN_CONFIRM  0x03   
// To stop the communication in between center and slave, 
// once this is sent the entire communication is stopped. 
// To begin again, select the patch you want to start with 
// Frames for selecting the mode. Mode can be discrete 
// or continuous for different satellite/slave nodes based on the
// patch you select i.e, "a" patch will assign both continuous 
// and discrete mode alternatievly to satellite brix. 
#define STOP_CMD_FRAME    0x04    
#define CSMA_MODE1_FRAME   0x05  // Mode 1 for discrete mode 
#define CSMA_MODE2_FRAME   0x06  // Mode 2 for continuous mode
#define CSMA_GET_FRAME    0x07  // To get data frame from Slave
#define BACK_CENTER_FRAME 0x08
#define BROADCAST       0x0A  // Broadcast data TXion in BRIX2
#define MAX_DATA_SIZE   10

// Generate Sensor Object and BRIX₂ Object
InertialSensor mySensor;
BRIX2 myBrick;

// Header definitions for Data Frame Format
enum {
  /* Header */
  ADD,  // Sender Address
  CTRL, // Frame Control
  LEN  // Lenth of Transmitted/Received data from the brix
};

// Booleans for the idle loop
boolean dataAVAILABLE, commandAVAILABLE;
char patch;
// Parameter to send to brix
int param1 = 0, param2 = 0;
// communication arrays for Receiving and Transmitting data
int receivedData[MAX_DATA_SIZE] = {}; // BRIX2 can hold up to 20 bytes (10 int)
int transmittedData[MAX_DATA_SIZE] = {};

// Parameter for Lookup table at Center node
int addressTable[8];
int nodesCounter = 0;


void setup() {
  // it is important to start Serial1 in order
  // to use the RF capabilities of the BRIX2 library
  Serial.begin(38400);
  Serial1.begin(38400);
  myBrick.initialize();
  mySensor.initialize();
  // At the beginning we set command and data available to false,
  // the moment we receive any data from slave node we set data available
  // to true and if we type some command in serial, then we set
  // the command available to true.
  commandAVAILABLE = false;
  dataAVAILABLE = false;

  // get own Address and do not accept a zero!
  while (transmittedData[ADD] == 0) {
    delay(100);
    transmittedData[ADD] =  myBrick.getAddress();
  }

  transmittedData[CTRL] = RESET_FRAME; // Reset Frame (Rejoin the network)
  transmittedData[LEN] = 0;
  // Broadcast the data with reset frame,
  // if slave receive this packet it will send 
  // NWK_JOIN_FRAME for estabilishing the communication
  myBrick.sendInt(0x0A, transmittedData, 3);
}

void loop() {

  // idle loop
  while (!dataAVAILABLE && !commandAVAILABLE) {

    if (Serial1.available() > 0) { // check for RF data
      myBrick.setRGB(255, 0, 255);

      receivedData[ADD] = Serial1.parseInt();
      //Check if address is -ve (Sometimes I got bizzare data from brix)
      // then parse the integer again
      if (receivedData[ADD] < 0) {
        receivedData[ADD] = Serial1.parseInt();
      }
      receivedData[CTRL] = Serial1.parseInt();
      receivedData[LEN] = Serial1.parseInt();


      // Check the acceptable Data Frame and length
      if ((receivedData[CTRL] == BACK_CENTER_FRAME ||
           receivedData[CTRL] == NWK_JOIN_FRAME )
          && receivedData[LEN] <= MAX_DATA_SIZE) {
          // Proceed if the frame is correct. i.e, make 
          // data available true and call different function
          // based on the frame 
        dataAVAILABLE = true;
        for (int i = 0; i < receivedData[LEN]; i++) {
          receivedData[i + 3] = Serial1.parseInt();
        }
      }
      else {
        /* It's faulty data. Discard it. */
        dataAVAILABLE = false;
        if (Serial1.read() == '\r') {
          break; // Find end of faulty data and break it.
        }
        continue; // If ends is not find, loop it again.
      }
    } // end check for RF


    // check for command input. Read the parameter values from command line. 
    // Right now just sending parameters for 3 patches.
    // To stop the communication or getting data back from slave
    // we dont use any parameter, but its implemented in the command line if-else condition
    if (Serial.available() > 0) {
      myBrick.setRGB(0, 0, 255);
      while (Serial.available() > 0) {
        patch = Serial.read();
        if (patch == 'a') {
          Serial.println(patch);
          param1 = Serial.parseFloat();
          param2 = Serial.parseFloat();
        } else if (patch == 'b') {
          param1 = Serial.parseFloat();
          param2 = Serial.parseFloat();
        } else if (patch == 'c') {
          param1 = Serial.parseFloat();
          param2 = Serial.parseFloat();
        }
      } // end while
      //if no further chars are availible leave idle loop
      commandAVAILABLE = true;
    } // eo if serial 
  } // END OF IDLE
  
  // Continue if something happend
  // otherwise go back to the start

  // if an new comand was given...
  if (commandAVAILABLE) {
    // Command to stop sending data
    if (patch == 's') {
      for (int i = 0; i < nodesCounter ; i++) {
        myBrick.setRGB(255, 255, 255);
        transmittedData[CTRL] = 0x4; // control frame
        transmittedData[LEN] = 0 ; // packet length in payload
        Serial.println("Data sent for Stop");
        Serial.println(transmittedData[ADD]);
        Serial.println(transmittedData[CTRL]);
        Serial.println(transmittedData[LEN]);
        Serial.println("Data End");
        myBrick.sendInt(addressTable[i], transmittedData, 3); 
        delay(10);
      }
    }
    else if ( patch == 'a')
    {
      Serial.println("a protocol, mode 0 and 1 for every alternate brix");
      // Choose alternate mode for brix
      for (int i = 0; i < nodesCounter; i++)
      {
        if (i % 2 == 0)
        {
          transmittedData[CTRL] = CSMA_MODE1_FRAME ; // control frame
          transmittedData[LEN] = 2;
          transmittedData[LEN + 1] = param1;
          transmittedData[LEN + 2] = param2;
          //packet length in payload
          Serial.println("Data sent to brix for continuous change");
          Serial.println("Brix Address");
          Serial.println(addressTable[i]);
          Serial.println(transmittedData[ADD]);
          Serial.println(transmittedData[CTRL]);
          Serial.println(transmittedData[LEN]);
          Serial.println("Data End");
          delay(100);
          myBrick.sendInt(addressTable[i], transmittedData, 5);
        }
        else
        {
          transmittedData[CTRL] = CSMA_MODE2_FRAME ; // control frame
          transmittedData[LEN] = 2;
          transmittedData[LEN + 1] = param1;
          transmittedData[LEN + 2] = param2;
          Serial.println("Data sent to brix for discrete change");
          Serial.println("Brix Address");
          Serial.println(addressTable[i]);
          Serial.println(transmittedData[CTRL]);
          Serial.println(transmittedData[LEN]);
          Serial.println("Data End");
          delay(100);
          myBrick.sendInt(addressTable[i], transmittedData, 5);
        }
      }
    } // EO patch a
    else if ( patch == 'b')
    {
      Serial.println("b protocol mode 1 for all the brix");
      // Choose mode 1 for all brix
      for (int i = 0; i < nodesCounter; i++)
      {
        transmittedData[CTRL] = CSMA_MODE1_FRAME ; // control frame
        transmittedData[LEN] = 2;
        transmittedData[LEN + 1] = param1;
        transmittedData[LEN + 2] = param2;
        Serial.println("Data sent for protocol b");
        Serial.println("Brix Address");
        Serial.println(addressTable[i]);
        Serial.println(transmittedData[ADD]);
        Serial.println(transmittedData[CTRL]);
        Serial.println(transmittedData[LEN]);
        Serial.println("Data End");
        delay(100);
        myBrick.sendInt(addressTable[i], transmittedData, 3);
      }
    }// EO patch b
    // Command to send actual data reading with TDMA
    else if (patch == 'c') {
      // Choose mode 2 for all brix
      Serial.println("c protocol, mode 2 for all the brix");
      for (int i = 0; i < nodesCounter; i++)
      {
        transmittedData[CTRL] = CSMA_MODE2_FRAME ; // control frame
        transmittedData[LEN] = 2;
        transmittedData[LEN + 1] = param1;
        transmittedData[LEN + 2] = param2;
        Serial.println("Data sent for c protocol");
        Serial.println(transmittedData[ADD]);
        Serial.println(transmittedData[CTRL]);
        Serial.println(transmittedData[LEN]);
        Serial.println("Receiver address ");
        Serial.println(addressTable[i]);
        Serial.println("Data End");
        delay(100);
        myBrick.sendInt(addressTable[i], transmittedData, 3);
      }
    }// EO patch c
    // Send the patch 'g' to get the data from slave brix
    else if ( patch == 'g') {
      transmittedData[CTRL] = CSMA_GET_FRAME ; // control frame
      transmittedData[LEN] = 0;
      Serial.println("Data sent for getting data back from Slaves");
      Serial.println(transmittedData[ADD]);
      Serial.println(transmittedData[CTRL]);
      Serial.println(transmittedData[LEN]);
      Serial.println("Data End");
      // BROADCAST because many brix are connected
      myBrick.sendInt(BROADCAST, transmittedData, 3);
    }
    else
      Serial.println("Invalid command");

    // everything was checked, so invalidate comand
    myBrick.setRGB(0, 0, 0);
    commandAVAILABLE = false;
  }

  // if a new data word has been read...
  if (dataAVAILABLE) {
    Serial.println("Data is available");
    // New satellite nodes inclusion to the Address Table of the
    // center Node and sending back the confirmation
    
    if (receivedData[CTRL] == NWK_JOIN_FRAME && receivedData[LEN] == 0) { 
      myBrick.setRGB(0, 0, 255);
      Serial.println("new participant arrived");
      Serial.print("Data recieved: ");
      Serial.print(receivedData[ADD]);
      Serial.print(" , ");
      Serial.print(receivedData[CTRL]);
      Serial.print(" , ");
      Serial.print(receivedData[LEN]);
      for (int i = 0; i < receivedData[LEN]; i++) {
        Serial.print(" , ");
        Serial.print(receivedData[i]);
      }
      Serial.println("");
      // send confirm frame with ID no., add the slave address to address table
      // of Center node (Check if the address already exist or not )
      transmittedData[CTRL] = NWK_JOIN_CONFIRM;//reponse frame for inclusion in the Nwk
      transmittedData[LEN] = 1;
      transmittedData[3] = nodesCounter;
      for (int i = 0; i < nodesCounter; i++)
        if (addressTable[i] == receivedData[0]) {
          //Serial.print("already ");
          transmittedData[3] = i;
        }
      addressTable[transmittedData[3]] = receivedData[ADD];
      Serial.println(transmittedData[0]);
      // Send the data back to slave with NWK_JOIN_CONFIRM frame
      myBrick.sendInt(receivedData[ADD], transmittedData, 4);
      if (transmittedData[3] == nodesCounter)
        nodesCounter++;
    }
    // Data received from the brix
    else if (receivedData[CTRL] == BACK_CENTER_FRAME) { 
      Serial.print("Data received from satellite node : ");
      Serial.println(receivedData[ADD]);
      Serial.println(receivedData[CTRL]);
      Serial.println(receivedData[LEN]);
      for (int i = 0; i < receivedData[LEN] ; i++)
      {
        Serial.println(receivedData[i + 3]);
      }
      Serial.println("Data End");
    }

    else {
      Serial.println("Unexpected Data is received! ");
    }

    dataAVAILABLE = false;
    myBrick.setRGB(0, 0, 0);
  }
}
