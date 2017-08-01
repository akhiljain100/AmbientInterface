# AmbientInterface
An interactive sensor application using BRIX2. 

Master/slave model of communication in between brix.

Switch on the brix and upload the CenterNodeBrix.ino file in it. Likewise for the slave(s) upload SatelliteNodeBrix.ino file.
Master will register automatically all the slave node if present. Once the master/slave relationship is established (LED will turn pink), the direction of control is always from the master to the slave(s). Send the request to slave node via command line. 
Open the command line of Center node.

Two different mode execution can be done in each slave (Continuous (LED color: yellow) and discrete (LED color: cyan) mode)
Type 'a' for sending alternate mode to nodes. i.e, Continuous mode to 1 node, Discrete mode to 2nd node and likewise. 
Type 'b' for Continuous mode to all nodes
Type 'c' for Discrete mode to all nodes

Users can also type 'a,2,3', 'b,2' or 'c,2,3' for sending the parameters.(Currently maximum two parameter)

Once the mode execution is started, User can type 's' or 'g'
  
  's' : User can only switch to other mode by stopping the current mode, and for that user 
        will have to type 's' command, entire system become idle and the led color will become pink (Both slave and master), 
        it waits for the next instruction. 

  'g' : User can get data back from slave node back to center node by typing 'g' (Only when slave is not idle)
  
Please note that mode (Discrete and Continuous) has not been yet implemented, its upto user to define their functionality.
Check the SatelliteNodeBrix.ino file for further details.
