# AmbientInterface
An interactive sensor application using BRIX2. 

Master/slave model of communication in between brix.

Switch on the brix and upload the CenterNodeBrix.ino file in it. Likewise for the slave(s) upload SatelliteNodeBrix.ino file.
Master will register automatically all the slave node if present. Once the master/slave relationship is established,
the direction of control is always from the master to the slave(s). Send the request to slave node via command line.
