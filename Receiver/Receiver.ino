#include <SoftwareSerial.h>

struct EdgeConnection {
  int srcPort;
  int srcBoardID;
  int connPort;
  int connBoardLevel;
  int connBoardID;
  int connPayloadID;
};

const int maxConnections = 100;  // Maximum number of connections to store
EdgeConnection connections[maxConnections];
int connectionCount = 0;

#define MYPORT1_TX 19
#define MYPORT1_RX 21
#define MYPORT2_TX 22
#define MYPORT2_RX 23
#define MYPORT3_TX 27
#define MYPORT3_RX 26
#define MYPORT4_TX 33
#define MYPORT4_RX 32

EspSoftwareSerial::UART myPort1;
EspSoftwareSerial::UART myPort2;
EspSoftwareSerial::UART myPort3;
EspSoftwareSerial::UART myPort4;

const int myBoardID = 42;
int myBoardLevel = -871;  // Can be overwritten later

const int myPayloadID = 871;

int upstreamPort = -871;

int myNonce = 0;

void setup() {
  Serial.begin(115200);  // Standard hardware serial port

  myPort1.begin(9600, SWSERIAL_8N1, MYPORT1_RX, MYPORT1_TX, false);
  myPort2.begin(9600, SWSERIAL_8N1, MYPORT2_RX, MYPORT2_TX, false);
  myPort3.begin(9600, SWSERIAL_8N1, MYPORT3_RX, MYPORT3_TX, false);
  myPort4.begin(9600, SWSERIAL_8N1, MYPORT4_RX, MYPORT4_TX, false);

  if (!myPort1 || !myPort2 || !myPort3 || !myPort4) {
    Serial.println("Invalid EspSoftwareSerial pin configuration, check config");
    while (1) {
      delay(1000);
    }
  }
}

void printEdgeConnections() {
  Serial.println("Edge Connections:");
  for (int i = 0; i < connectionCount; i++) {
    Serial.print("Connection ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print("srcPort = ");
    Serial.print(connections[i].srcPort);
    Serial.print(", srcBoardID = ");
    Serial.print(connections[i].srcBoardID);
    Serial.print(", connPort = ");
    Serial.print(connections[i].connPort);
    Serial.print(", connBoardLevel = ");
    Serial.print(connections[i].connBoardLevel);
    Serial.print(", connBoardID = ");
    Serial.print(connections[i].connBoardID);
    Serial.print(", connPayloadID = ");
    Serial.println(connections[i].connPayloadID);
  }
}

void loop() {
  // Your loop code here
  if (myBoardID == 0) {
    myNonce = random(55005511);
    connectionCount = 0;
    Serial.println("=======Begin new round=======");
    broadcast();
    delay(100);
  }

  for (int i = 0; i < 100; i++) {
    receive();
    delay(100);
  }

  if (myBoardID == 0) {
    printEdgeConnections();
    delay(100);
  }
}

void receive() {
  if (myPort1.available()) {
    String command1 = myPort1.readStringUntil('\n');
    processCommand(command1, 1);
  }

  // Check for incoming data on myPort2
  if (myPort2.available()) {
    String command2 = myPort2.readStringUntil('\n');
    processCommand(command2, 2);
  }

  if (myPort3.available()) {
    String command3 = myPort3.readStringUntil('\n');
    processCommand(command3, 3);
  }

  if (myPort4.available()) {
    String command4 = myPort4.readStringUntil('\n');
    processCommand(command4, 4);
  }
}

void processCommand(String command, int port) {
  //Serial.print("Received command on port ");
  //Serial.print(port);
  //Serial.println(": " + command);

  // Parse the command to extract integers
  int recvNonce, num1, num2, num3, num4, num5, num6;
  if (sscanf(command.c_str(), "B(%d,%d,%d,%d)", &recvNonce, &num1, &num2, &num3) == 4) {
    Serial.print("#b> ");
    processBroadcast(port, recvNonce, num1, num2, num3);
    Serial.println(" <");
  } else if (sscanf(command.c_str(), "E(%d,%d,%d,%d,%d,%d,%d)", &recvNonce, &num1, &num2, &num3, &num4, &num5, &num6) == 7) {
    Serial.print("#e> ");
    processEdge(port, recvNonce, num1, num2, num3, num4, num5, num6);
    Serial.println(" <");

  } else {
    Serial.print("Failed to parse command ");
    Serial.println(command);
  }
}

void processEdge(int fromPort, int msgNonce, int srcPort, int srcBoardID, int connPort, int connBoardLevel, int connBoardID, int connPayloadID) {
  if (msgNonce != myNonce) {
    Serial.print("stale");
    return;  // old nonce = stale message = ignore
  }
  if (srcBoardID == myBoardID) {
    // process with extra validation
    if (srcPort != fromPort) {
      Serial.print("[!!!] Actual receive port != declared port. TX&RX mixed up");
      return;
    }
  }
  if (myBoardID == 0) {
    Serial.print("OK");
    if (connectionCount < maxConnections) {
      connections[connectionCount].srcPort = srcPort;
      connections[connectionCount].srcBoardID = srcBoardID;
      connections[connectionCount].connPort = connPort;
      connections[connectionCount].connBoardLevel = connBoardLevel;
      connections[connectionCount].connBoardID = connBoardID;
      connections[connectionCount].connPayloadID = connPayloadID;
      connectionCount++;
    } else {
      Serial.print("Connection array full, cannot store more");
    }
    return;
  }
  if (srcBoardID != 0) {
    sendEdge(upstreamPort, msgNonce, srcPort, srcBoardID, connPort, connBoardLevel, connBoardID, connPayloadID);
    Serial.print("relayed");
  }
}

void sendEdge(int toPort, int msgNonce, int srcPort, int srcBoardID, int connPort, int connBoardLevel, int connBoardID, int connPayloadID) {
  // Construct the message
  if (toPort == 1) {
    myPort1.print("E(");
    myPort1.print(msgNonce);
    myPort1.print(",");
    myPort1.print(srcPort);
    myPort1.print(",");
    myPort1.print(srcBoardID);
    myPort1.print(",");
    myPort1.print(connPort);
    myPort1.print(",");
    myPort1.print(connBoardLevel);
    myPort1.print(",");
    myPort1.print(connBoardID);
    myPort1.print(",");
    myPort1.print(connPayloadID);
    myPort1.println(")");
  } else if (toPort == 2) {
    myPort2.print("E(");
    myPort2.print(msgNonce);
    myPort2.print(",");
    myPort2.print(srcPort);
    myPort2.print(",");
    myPort2.print(srcBoardID);
    myPort2.print(",");
    myPort2.print(connPort);
    myPort2.print(",");
    myPort2.print(connBoardLevel);
    myPort2.print(",");
    myPort2.print(connBoardID);
    myPort2.print(",");
    myPort2.print(connPayloadID);
    myPort2.println(")");
  } else if (toPort == 3) {
    myPort3.print("E(");
    myPort3.print(msgNonce);
    myPort3.print(",");
    myPort3.print(srcPort);
    myPort3.print(",");
    myPort3.print(srcBoardID);
    myPort3.print(",");
    myPort3.print(connPort);
    myPort3.print(",");
    myPort3.print(connBoardLevel);
    myPort3.print(",");
    myPort3.print(connBoardID);
    myPort3.print(",");
    myPort3.print(connPayloadID);
    myPort3.println(")");
  } else if (toPort == 4) {
    myPort4.print("E(");
    myPort4.print(msgNonce);
    myPort4.print(",");
    myPort4.print(srcPort);
    myPort4.print(",");
    myPort4.print(srcBoardID);
    myPort4.print(",");
    myPort4.print(connPort);
    myPort4.print(",");
    myPort4.print(connBoardLevel);
    myPort4.print(",");
    myPort4.print(connBoardID);
    myPort4.print(",");
    myPort4.print(connPayloadID);
    myPort4.println(")");
  } else {
    // Handle invalid port number if needed
  }
}

void processBroadcast(int myPort, int msgNonce, int msgPort, int msgBoardLevel, int msgID) {
  if (myBoardID != 0 && msgNonce != myNonce) {
    // reset state, forget everything that was stored;
    myBoardLevel = -871;
    upstreamPort = -871;
    Serial.print("resetnonce; ");
    myNonce = msgNonce;
  }
  if (myBoardLevel == -871) {
    myBoardLevel = msgBoardLevel;
    upstreamPort = myPort;
    Serial.print("blrecv:");
    Serial.print(msgBoardLevel);
    Serial.print("; ");
    // tell upstream about myself
    sendEdge(upstreamPort, msgNonce, msgPort, msgID, myPort, myBoardLevel, myBoardID, myPayloadID);
    Serial.print("tellupstream; ");
  }  // else do not honor the request of setting BoardLevel
  if (msgBoardLevel > myBoardLevel) {
    // do not relay broadcast from higher-level broadcast
  } else {
    Serial.print("rebroadcast; ");
    broadcast();
  }
}

void broadcast() {
  // Sending to myPort1
  myPort1.print("B(");
  myPort1.print(myNonce);
  myPort1.print(",1,");
  myPort1.print(myBoardLevel + 1);
  myPort1.print(",");
  myPort1.print(myBoardID);
  myPort1.println(")");

  // Sending to myPort2
  myPort2.print("B(");
  myPort2.print(myNonce);
  myPort2.print(",2,");
  myPort2.print(myBoardLevel + 1);
  myPort2.print(",");
  myPort2.print(myBoardID);
  myPort2.println(")");

  // Sending to myPort3
  myPort3.print("B(");
  myPort3.print(myNonce);
  myPort3.print(",3,");
  myPort3.print(myBoardLevel + 1);
  myPort3.print(",");
  myPort3.print(myBoardID);
  myPort3.println(")");

  // Sending to myPort4
  myPort4.print("B(");
  myPort4.print(myNonce);
  myPort4.print(",4,");
  myPort4.print(myBoardLevel + 1);
  myPort4.print(",");
  myPort4.print(myBoardID);
  myPort4.println(")");

  //Serial.println("Broadcast sent to both ports.");
}