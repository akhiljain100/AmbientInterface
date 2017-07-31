//////////////////////////////////////////////////////////////////////////////////////////
// The Satellite Node: CSMA/CA Scheme in BRIX2                                          //
//                                                                                      //
// The code should be uploaded in accordance with its companion the central node.       //
// When "csma" cmd arrives, the data will be transmitted until "stop" cmd is active.    //
// Satellite nodes compete to send data to the center Node. RTS signal is first send to //
// the center to access the available channel. If CTS signal is received back from the  //
// center, satellite node can transmit the actual payload data.                         //
//                                                                                      //
// Commands-> csma: start CSMA/CA scheme in star network topology(RXver is the center)  //
//            stop: stop the current data streaming scheme                              //
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
#include <AmbiSenseExtension.h>

// Frame Control Definitions (choose specific No.)
#define RESET_FRAME       0x01 
#define NWK_JOIN_FRAME    0x02    // used by satellite nodes to join the central node
#define NWK_JOIN_CONFIRM  0x03
#define STOP_CMD_FRAME    0x04  // stop command frame
#define CSMA_MODE1_FRAME  0x05
#define CSMA_MODE2_FRAME  0x06
#define CSMA_GET_FRAME    0x07
#define BACK_CENTER_FRAME 0x08
#define BROADCAST         0x0A
#define MAX_DATA_SIZE     10

// Generate Sensor Object and BRIX₂ Object
InertialSensor mySensor;
BRIX2 myBrick;
AmbiSenseExtension myAmbi;

// Header definitions for Data Frame Format
enum {
  /* Header */
  ADD,  // Sender Address
  CTRL, // Frame Control
  LEN  // Lenth of TX/RX Data
};

// boolean for idling untile data have been arrived
boolean idle = true;
boolean stopComputingData = false;
int centerID = 0;
int receivedData[MAX_DATA_SIZE]={};//BRIX2 can hold up to 20bytes (10 int)
int transmittedData[MAX_DATA_SIZE]={};


int count = 0 ; //For Acknowledgement counter

void setup(){
  delay(50);
  // it is important to start Serial1 in order
  // to use the RF capabilities of the BRIX2 library
  Serial.begin(38400);
  Serial1.begin(38400);
  myBrick.initialize();
  mySensor.initialize();
  //mySensor.setAccelerometerRange(2)
  //mySensor.setGyroscopeRange(250)
  // get own Address and do not accept a zero!
  while(transmittedData[ADD] == 0){
    transmittedData[ADD] =  myBrick.getAddress();
    Serial.println(transmittedData[ADD]);
    delay(100);
  }
  delay(50);
  // find central node!
  while(!init_RF())
 
    delay(100);
}

// init the network position
// try to get an ID from the central node
boolean init_RF(){
  myBrick.setRGB(0,0,255);
  transmittedData[CTRL] = NWK_JOIN_FRAME;
  transmittedData[LEN] = 0;
  //Serial.println(transmittedData[ADD]);
  delay(transmittedData[0]*10); 
  Serial.println("sending hello");
  // send a Hello
  myBrick.sendInt(0x0A,transmittedData,3);
  //Serial.println(NWK_JOIN_CONFIRM);
  myBrick.setRGB(0,0,0);
  delay(15);
  
  //look for an answer
  if(Serial1.available()>0){   
    
    myBrick.setRGB(255,0,0);
    //int toAskData= Serial1.parseInt();
    receivedData[0] = Serial1.parseInt();
    receivedData[1] = Serial1.parseInt();
    receivedData[2] = Serial1.parseInt(); 
    if(receivedData[CTRL] != NWK_JOIN_CONFIRM  ){ // check for Nwk join confirm from center
      while(Serial1.available()>0){
        delay(1);
        Serial1.read();
        
        Serial.println(receivedData[ADD]);
      }
      myBrick.setRGB(0,0,0);
      return false; 
    }
    Serial.println("NWK join confirm"); // Debug
    //receivedData[LEN] = Serial1.parseInt();
    for(int i=0; i < receivedData[LEN]; i++)
     { receivedData[i+3] = Serial1.parseInt();   
      Serial.println(receivedData[i+3]);
     }delay(10);
    if (Serial1.read() == '\r' && receivedData[CTRL] == NWK_JOIN_CONFIRM){ // confirm RXed
      Serial.print("received ID init! I am ID number: ");
      centerID = receivedData[ADD];
      Serial.print("the central node is Address: ");
      Serial.println(centerID);
      while(Serial1.available()>0){
        delay(1);
        Serial1.read();
      }
      myBrick.setRGB(0,0,0);
      return true;  
    }
  }

  return false;
}

// main loop
// look for data and process them
void loop(){

  // IDLE if there is nothing to do!
  while(idle){ 
    myBrick.setRGB(255,0,255);
    if(Serial1.available()>0){
      myBrick.setRGB(255,0,0);
      delay(100);
      Serial.println("Sometimes getting some bizzare negative values so checking if its negative then parsing the integer again");
      receivedData[ADD] = Serial1.parseInt();
      if(receivedData[ADD] < 0)
      {
         receivedData[ADD] = Serial1.parseInt();
        }
      receivedData[CTRL] = Serial1.parseInt(); 
      receivedData[LEN] = Serial1.parseInt();
      
       delay(100);     
      if(receivedData[CTRL] == RESET_FRAME || receivedData[CTRL] == NWK_JOIN_CONFIRM || \
         receivedData[CTRL] == CSMA_MODE1_FRAME ||  receivedData[CTRL] == CSMA_MODE2_FRAME){
         myBrick.setRGB(0,255,0); 
         myBrick.sendInt(centerID,receivedData,3);
         delay(100);
         idle = false;
        /* It is our data. Proceed it */
        for(int i=0;i<receivedData[2];i++)
          receivedData[i+3] = Serial1.parseInt();
      }
      else {
        /* It's not our data or faulty data. Discard it. */
        idle = true;
        break;
      }
      
//      delay(2);
      myBrick.setRGB(0,0,0);

    } 
  } // while(idle)

  // wake up and process received data
  // reset from the central node
  if(receivedData[CTRL]== RESET_FRAME && receivedData[LEN] == 0){ // Check for Reset Frame 
    while(!init_RF())
      delay(100);
  }

  // Check for command frame in CSMA/CA (Unsloted) data sending protocol
  else if (receivedData[CTRL] == CSMA_MODE1_FRAME ){ // CSMA/CA command Frame
    //Here come necessary for data sending codes
    Serial.println("Mode 1 started ");
    centerID = receivedData[ADD];
    Serial.println(centerID);
    int param1 = receivedData[3]; 
    int param2 = receivedData[4];
    stopComputingData = false ;
    myBrick.setRGB(255,255,0);
    delay(100);
    
    // Proceed until stop command is activated
    while(!stopComputingData){ 
      
      if(Serial1.available()>0)
      {
        receivedData[ADD] = Serial1.parseInt();
        receivedData[CTRL] = Serial1.parseInt(); 
        receivedData[LEN] = Serial1.parseInt();
        if(receivedData[CTRL] == STOP_CMD_FRAME)
        {
          myBrick.setRGB(0,255,255);
        delay(1000);
          stopComputingData = true;
          }
          else if(receivedData[CTRL] == CSMA_GET_FRAME)
          {
            myBrick.setRGB(255,0,255);
        delay(1000);
            Serial.println("Sending data to center node ");
            myBrick.sendInt(centerID,transmittedData,8);
            }
          
         }
         Serial.println("In mode 1 section");
      myBrick.setRGB(255,255,0);
      delay(500);
          transmittedData[CTRL] = BACK_CENTER_FRAME; // CSMA data Sending
          transmittedData[LEN] = 6; // size of data depending on what data are reading
          transmittedData[3]=mySensor.getRotationX();
          transmittedData[4]=mySensor.getRotationY();
          transmittedData[5]=mySensor.getRotationZ();
          transmittedData[6]=mySensor.getAccelerationX(0,5);
          transmittedData[7]=mySensor.getAccelerationY(0,5);
          transmittedData[8]=mySensor.getAccelerationZ(0,5);
          
          
    }
  }
  else if (receivedData[CTRL] == CSMA_MODE2_FRAME ){ // CSMA/CA command Frame
    //Here come necessary for data sending codes
    Serial.println("Mode 2 started ");
    centerID = receivedData[ADD];
    Serial.println(centerID);
    int param1 = receivedData[3]; 
    int param2 = receivedData[4];
    stopComputingData = false ;
    myBrick.setRGB(0,255,255);
    delay(100);
    
    // Proceed until stop command is activated
    while(!stopComputingData){ 
      
      if(Serial1.available()>0)
      {
        receivedData[ADD] = Serial1.parseInt();
        receivedData[CTRL] = Serial1.parseInt(); 
        receivedData[LEN] = Serial1.parseInt();
        if(receivedData[CTRL] == STOP_CMD_FRAME)
        {
          stopComputingData = true;
          }
          else if(receivedData[CTRL] == CSMA_GET_FRAME)
          {
            delay(100);
            myBrick.sendInt(centerID,transmittedData,8);
            }
            else
            {
              Serial.println("Unidentified frame received");
              }
          
         }
         myBrick.setRGB(0,255,255);
         delay(1000);
       Serial.println("In the computing section");
          transmittedData[CTRL] = BACK_CENTER_FRAME; // CSMA data Sending
          transmittedData[LEN] = 6; // size of data depending on what data are reading
          transmittedData[3]=mySensor.getRotationX();
          transmittedData[4]=mySensor.getRotationY();
          transmittedData[5]=mySensor.getRotationZ();
          transmittedData[6]=mySensor.getAccelerationX(0,5);
          transmittedData[7]=mySensor.getAccelerationY(0,5);
          transmittedData[8]=mySensor.getAccelerationZ(0,5);
          
         // myBrick.sendInt(centerID,transmittedData,8); 
          delay(100);
    }
  }


  else {
    Serial.println("The requested data is not supported");
  }

  idle = true;
}
