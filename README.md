hub-server-bridge is based on hub-server in the repo--
GliderWinchCommons/embedand in the directory--GliderWinchCommons/embed/svn_sensor/PC/hub-server
The basic use of hub-server is to take incoming lines (which in the intended use are CAN msgs in an ascii/hex format) and distribute them to multiple connections. These multiple connections can be programs to monitor, or even send CAN msgs. The 'hub' can be thought of as extending the (binary) CAN bus into the PC. Since hub-server uses TCP sockets, connections to remote machines and CAN buses are possible.

hub-server-bridge adds filtering between client socket connections and listening socket connections. If the traffic of two CAN buses are to be merged over the internet there could be bandwidth issues, so restricting some of the CAN msgs might be needed. E.g., it might be useful to filter time tick CAN msgs running at a high rate and triggering responses at the rate from being sent out the socket connection to the remote system. hub-server-bridge modified and added routines to the original hub-server to add the filtering feature.

The filtering is specified in txt file loaded at startup and allows filtering between client connections, and listening socket connection, multiple listening connections grouped as one type of connection.
Incoming "lines" (bytes separated by an 'eol' char, default being '\n) are either passed, or not passed to each of the other connections. (Sending a line to "self" of course is skipped.)
The filtering is based on checking the CAN id embedded in an asc/hex format of CAN msgs. The filtering txt file creates tables upon startup, and the incoming CAN msg is either passed to an output connection or not passed based on the tables. In some cases when the CAN msg is passed to the output, the CAN id is translated according to a table. This provides a way of dealing with CAN msgs from units with the same CAN id, e.g. off-the-shelf units such as chargers, that have the same CAN ids.
The filtering txt file specifies a N by N set of tables. N is the number of client connections, plus one for the listening socket group. The following command line would be an example of a listening socket and two client sockets, where one client socket might be a connection made by 'socat' to convert a usb/serial port to tcp socket, carrying the ascii/hex from a gateway to a CAN bus. The second client socket might be the connection to a remote machine, running a similar setup to a CAN bus. 

The tables are created in memory upon startup by reading in the txt file and calloc'ing memory. A array of small structs can be accessed via indexing on the two dimensions: input connection number, and output connection number. The struct contains the size of the tables and pointers to the tables for each in:out pair. If the size is zero there is no associated table. When the size is not zero there is a table associated with the in:out pair.
In addition, there is a "type" code (0 or 1) that specifies whether the filtering and tables are based on "pass-on-match" (pom) or "block-on-match" (bom). If the type is pom, then an empty table becomes block-all. If the type is bom, then an empty table is pass-all.
For pom and bom if the logic is to "pass" then the CAN id needs to be checked for translation. If the CAN id is in the translation table, then the CAN id in the msg is modified to the translated id before it passed. 

The implementation therefore has one, two column table associated with pom. For bom, there is a one column table and one two column table. For a pom in:out situation the two column table will have the ids to be passed, the second column contains the translated id (if the value is not zero). For the bom in:out situation the one column table contains the ids to be blocked. If the incoming id is not in this table will be passed if it is not found in the two column table and translated.
Most of the extensions for hub-server-bridge are in the directory--GliderWinchItems/hub-server-bridge/sandboxes/sand1/trunkThese routines make use of several .c files in the repoGliderWinchItems/bridging/PCThese two repos should be at the same relative directory level.
To compile and execute hub-server-bridge, for example--cd ~/GliderWinchItems/hub-server-bridge/sandboxes/sand1/trunk./m sand1 && ./hub-server-bridge-sand1 --nodelay --file=CANtest3x3.txt localhost 32130 localhost 32143 localhost 32142
The ./m command sand1 compiles hub-server with the extensions in sand1.The execution specifies the txt file, one listening socket, and two client sockets.
To compile without the bridging features, for example--./m && ./hub-server --nodelay localhost 32130
Notes: - if the server for a client socket connection fails, the startup will exit with an error msg.




