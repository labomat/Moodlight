/*
 * 
 *  M O O D L I G H T  WebControl
 *  
 *  Steuerung der Moodlight-Animationsmatrix
 *  per ESP8266 via Webseite
 *  
 *  web control for Moodlight animation matrix
 *  via esp8266
 *  
 *  CC-BY SA 2016 Kai Laborenz
 *  based on code from Stefan Thesen 04/2015
 *  https://blog.thesen.eu/stabiler-http-1-1-wlan-webserver-mit-dem-esp8266-microcontroller/
 *  
 */
#include <Wire.h>
#include <ESP8266WiFi.h>
#include "config.h"

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
const char* csspath = CSS_PATH;

unsigned long ulReqcount;
unsigned long ulReconncount;

const int sclPin = D1;
const int sdaPin = D2;
const int ledPin = D6;

#define DEBUG

#ifdef DEBUG
 #define DEBUG_PRINT_LN(x)  Serial.println (x)
 #define DEBUG_PRINT(x)  Serial.print (x)
#else
 #define DEBUG_PRINT_LN(x)
 #define DEBUG_PRINT(x)
#endif


// Create an instance of the server on Port 80
WiFiServer server(80);

void setup() 
{
  // setup globals
  ulReqcount=0; 
  ulReconncount=0;
  
  // prepare GPIO14/D6 (server)
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, 0);

  pinMode(BUILTIN_LED, OUTPUT); // initialize onboard LED as output

  // start i2c
  Wire.begin(sdaPin, sclPin);
  
  // start serial
  Serial.begin(115200);
  delay(1);
  
  // inital connect
  WiFi.mode(WIFI_STA);
  WiFiStart();
}

void WiFiStart()
{
  ulReconncount++;
  
  // Connect to WiFi network

  DEBUG_PRINT("Connecting to ");
  DEBUG_PRINT_LN(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DEBUG_PRINT(".");
  }
  DEBUG_PRINT_LN("");
  DEBUG_PRINT_LN("WiFi connected");
  
  // Start the server
  server.begin();
  DEBUG_PRINT_LN("Server started");
  digitalWrite(ledPin, HIGH); // led on

  // Print the IP address
  DEBUG_PRINT_LN(WiFi.localIP());
}

void loop() 
{
    
  // check if WLAN is connected
  if (WiFi.status() != WL_CONNECTED)
  {
    WiFiStart();
  }
  
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) 
  {
    return;
  }
  
  // Wait until the client sends some data
  DEBUG_PRINT_LN("new client");
  unsigned long ultimeout = millis()+250;
  while(!client.available() && (millis()<ultimeout) )
  {
    delay(1);
  }
  if(millis()>ultimeout) 
  { 
    DEBUG_PRINT_LN("client connection time-out!");
    return; 
  }
  
  // Read the first line of the request
  String sRequest = client.readStringUntil('\r');
  //DEBUG_PRINT_LN(sRequest);
  client.flush();
  
  // stop client, if request is empty
  if(sRequest=="")
  {
    DEBUG_PRINT_LN("empty request! - stopping client");
    client.stop();
    return;
  }
  
  // get path; end of path is either space or ?
  // Syntax is e.g. GET /?pin=MOTOR1STOP HTTP/1.1
  String sPath="",sParam="", sCmd="";
  String sGetstart="GET ";
  int iStart,iEndSpace,iEndQuest;
  char buffer[32];
  iStart = sRequest.indexOf(sGetstart);
  if (iStart>=0)
  {
    iStart+=+sGetstart.length();
    iEndSpace = sRequest.indexOf(" ",iStart);
    iEndQuest = sRequest.indexOf("?",iStart);
    
    // are there parameters?
    if(iEndSpace>0)
    {
      if(iEndQuest>0)
      {
        // there are parameters
        sPath  = sRequest.substring(iStart,iEndQuest);
        sParam = sRequest.substring(iEndQuest,iEndSpace);
      }
      else
      {
        // NO parameters
        sPath  = sRequest.substring(iStart,iEndSpace);
      }
    }
  }
  
  ///////////////////////////////////////////////////////////////////////////////
  // output parameters to i2c
  ///////////////////////////////////////////////////////////////////////////////
  if(sParam.length()>0)
  {
    int iEqu=sParam.indexOf("=");
    if(iEqu>=0)
    {
      sCmd = sParam.substring(iEqu+1,sParam.length());
      digitalWrite(ledPin, LOW); // server led off
      int cmdInt = sCmd.toInt();
      //sCmd.toCharArray(buffer, 32);
      DEBUG_PRINT_LN(sCmd);
      // transmit to device #9
      Wire.beginTransmission(9); 
      Wire.write(cmdInt);
      Wire.endTransmission(); 
      delay(200);
      digitalWrite(ledPin, HIGH); // server led on again
    }
  }
  
  
  ///////////////////////////
  // format the html response
  ///////////////////////////
  String sResponse,sHeader;
  
  ////////////////////////////
  // 404 for non-matching path
  ////////////////////////////
  if(sPath!="/")
  {
    sResponse="<html><head><title>404 Not Found</title></head><body><h1>Not Found</h1><p>The requested URL was not found on this server.</p></body></html>";
    
    sHeader  = "HTTP/1.1 404 Not found\r\n";
    sHeader += "Content-Length: ";
    sHeader += sResponse.length();
    sHeader += "\r\n";
    sHeader += "Content-Type: text/html\r\n";
    sHeader += "Connection: close\r\n";
    sHeader += "\r\n";
  }
  ///////////////////////
  // format the html page
  ///////////////////////
  else
  {
    ulReqcount++;
    sResponse  = "<html><head><title>M O O D L I G H T  Control</title>";
    sResponse += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=yes\">";
    sResponse += "<link rel=\"stylesheet\" href=\"";
    sResponse += csspath;
    sResponse += "\">";
    sResponse += "</head><body>";
    sResponse += "<h1>M O O D L I G H T  Control</h1>"; 
    if (sCmd=="1"){
      sResponse += "<p><a href=\"?mode=1\"><button class=\"act\">Flat Color</button></a></p>";
    }
    else {
      sResponse += "<p><a href=\"?mode=1\"><button>Flat Color</button></a></p>";
    }
    
    if (sCmd=="2"){
      sResponse += "<p><a href=\"?mode=2\"><button class=\"act\">Color Splash</button></a></p>";
    }
    else {
      sResponse += "<p><a href=\"?mode=2\"><button>Color Splash</button></a></p>";
    }
    
    if (sCmd=="3"){
      sResponse += "<p><a href=\"?mode=3\"><button class=\"act\">Party Time</button></a></p>";
    }
    else {
      sResponse += "<p><a href=\"?mode=3\"><button>Party Time</button></a></p>";
    }

    if (sCmd=="4"){
      sResponse += "<p><a href=\"?mode=4\"><button class=\"act\">Twinkling Lights</button></a></p>";
    }
    else {
      sResponse += "<p><a href=\"?mode=4\"><button>Twinkling Lights</button></a></p>";
    }

    if (sCmd=="5"){
      sResponse += "<p><a href=\"?mode=5\"><button class=\"act\">Rainbow</button></a></p>";
    }
    else {
      sResponse += "<p><a href=\"?mode=5\"><button>Rainbow</button></a></p>";
    }

    //////////////////////
    // react on parameters
    //////////////////////
    if (sCmd.length()>0)
    {
      // write received command to html page
      sResponse += "Aktueller Modus: " + sCmd + "<br>";
      
    }
    
    sResponse += "<FONT SIZE=-2>";
    sResponse += "<br>Aufrufz&auml;hler="; 
    sResponse += ulReqcount;
    sResponse += " - Verbindungsz&auml;hler="; 
    sResponse += ulReconncount;
    sResponse += "<br>";
    sResponse += "</body></html>";
    
    sHeader  = "HTTP/1.1 200 OK\r\n";
    sHeader += "Content-Length: ";
    sHeader += sResponse.length();
    sHeader += "\r\n";
    sHeader += "Content-Type: text/html\r\n";
    sHeader += "Connection: close\r\n";
    sHeader += "\r\n";
  }
  
  // Send the response to the client
  client.print(sHeader);
  client.print(sResponse);
  
  // and stop the client
  client.stop();
  DEBUG_PRINT_LN("Client disonnected");
}
