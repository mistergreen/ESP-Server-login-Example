// © 2noodles llc
// minh@2noodles.com

#define bufferMax 628
#define queryMax 350
#define SDCARD_CS D8

#include <ESP8266WiFi.h>

#include <SPI.h>
#include <SD.h>

#include <WebParser.h>



/************* network & server configuration ***************/

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,1,177);
IPAddress gateway(192,168,1,1);  
IPAddress subnet(255, 255, 255, 0);

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
WiFiServer server(80);

int status = WL_IDLE_STATUS;
int bufferSize;
char queryBuffer[bufferMax];
char param_value[queryMax];
WebParser webParser;

File webFile;


/************* website login **************/
// wifi
char ssid[] = "WATERFALL"; //  your network SSID (name)
char password[] = "quynhnhi";    // your network password (use for WPA, or use as key for WEP)

// site login
char username[] = "admin";
char loginpass[] = "waac";


unsigned long arduinoSession = 1;

  
/****************************************** sketch Logic **********************************************************/

void setup() 
{
 
  // start serial port:
  Serial.begin(115200);
  //SerialUSB.begin(115200);  // Due nativeUSB port, no reset of board on open serial port

  //Wire.begin();

  //************ wifi *******************
  
  WiFi.config(ip, gateway, subnet, gateway);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected to wifi");

  // set up server for the rest of the sockets
  server.begin();
  

   //************ initialize SD card *******************
  
  //Serial.println("Initializing SD card...");
  if (!SD.begin(SDCARD_CS)) {
        Serial.println("ERROR - SD card initialization failed!");
        return;    // init failed
  }
  //Serial.println("SUCCESS - SD card initialized.");
    // check for index.htm file
    //it's picky with file extension, only 3 letters
  
  if (!SD.exists("index.htm")) {
        Serial.println("ERROR - Can't find index.htm file!");
        return;  // can't find index file
  }
 
  Serial.println("SUCCESS - Found index.htm file.");

  
}


/************************************************************ loop *********************************/
  
void loop() 
{
 
  // listen for incoming clients
  WiFiClient client = server.available();
 
  if (client) {
    
     boolean currentLineIsBlank = true;
     bufferSize = 0;
     
        while (client.connected()) {
            if (client.available()) {   // client data available to read
                char c = client.read(); // read 1 byte (character) from client
               //Serial.print(c);
                if (bufferSize < bufferMax) queryBuffer[bufferSize++] = c;
                 if (c == '\n' && currentLineIsBlank) {
                   parseReceivedRequest(client);
                   bufferSize = 0;
                   webParser.clearBuffer(queryBuffer, bufferMax);
                   break;
                     
                 }
                 if (c == '\n') {
                    // last character on line of received text
                    // starting new line with next character read
                    currentLineIsBlank = true;
                } 
                else if (c != '\r') {
                    // a text character was received from client
                    currentLineIsBlank = false;
                }
               
                
            }//end if available
          
        }//end while connected
    
    
      // give the web browser time to receive the data
      delay(1);
    // close the connection:
    client.flush();
    client.stop();
    
  }//end client
 
}

/*********************************/


void renderHtmlPage(char *page, WiFiClient client) 
{

        //disconnect the W5200
        //digitalWrite(W5200_CS,HIGH);      
        byte tBuf[64];
        int clientCount = 0;
        unsigned long lastPosition = 0;
        
        File myFile = SD.open(page);        // open web page file
        if (myFile) {
             // send a standard http response header
             client.println(F("HTTP/1.1 200 OK"));
             client.println(F("Connection: close"));
             if(webParser.contains(page, ".jpg")){
               client.println(F("Content-Type: image/jpeg"));
               client.println(F("Cache-Control: max-age=36000, public"));  
             } else if(webParser.contains(page, ".gif")){
               client.println(F("Content-Type: image/gif"));
               client.println(F("Cache-Control: max-age=36000, public"));
             } else if(webParser.contains(page, ".png")){
               client.println(F("Content-Type: image/png"));
               client.println(F("Cache-Control: max-age=36000, public"));
             } else if(webParser.contains(page, ".htm")){
               client.println(F("Content-Type: text/html"));
             } else if(webParser.contains(page, ".js")){
               client.println(F("Content-Type: application/javascript"));
             } else if(webParser.contains(page, ".css")){
               client.println(F("Content-Type: text/css"));
             }
             
             client.println();   
        
           while(myFile.available())
            {
               myFile.read(tBuf,64); // or myFile.read(&tBuf,64)
               client.write((const uint8_t *)tBuf, myFile.position() - lastPosition);
               lastPosition = myFile.position();
    
             }
              
            myFile.close();
             // disconnect the SD card

        } else {
          Serial.println("file not found"); 
          Serial.println(page);
           client.println(F("HTTP/1.1 404 Not Found"));
           client.println(F("Content-Type: text/html"));
           client.println(F("Connection: close"));
           client.println();
           client.println(F("<h2>File Not Found!</h2>"));
        }
        
       
        
}

void parseReceivedRequest(WiFiClient client) 
{
  //find query vars
  //Serial.println(" ");
  Serial.println("*************");
  Serial.println(queryBuffer);
  Serial.println("*************");
  
  //  GET /index.htm HTTP/1.1
  // GET / HTTP/1.1
  if(webParser.contains(queryBuffer, "GET / HTTP/1.1") || webParser.contains(queryBuffer, ".htm ")) {
    // *********** Render HTML ***************
   // code not to render form request.
   // GET /index.htm?devicelist=1&nocache=549320.8093103021 HTTP/1.1

    if(loggedIn()) {
       //render html pages only if you've logged in
       webParser.clearBuffer(param_value, queryMax);
       webParser.fileUrl(queryBuffer, param_value);
       // default page 
       if(strcmp(param_value, "/") == 0) {
         strcpy(param_value, "entry.htm");
         client.println(F("HTTP/1.1 302 Found"));
         client.println(F("Location: /entry.htm")); 

       }
       //else load whatever
       renderHtmlPage(param_value, client);
       
     } else {  
        //loggin form
        char page[] = "logtut.htm";
        //set it so it's not the same all the time.
        arduinoSession = millis();
        renderHtmlPage(page, client);
        
    }//login

   } else {
    webParser.clearBuffer(param_value, queryMax);
    
    if(webParser.contains(queryBuffer, "login")) 
    {
      webParser.parseQuery(queryBuffer, "username", param_value);

      char user[30];
      strcpy(user,param_value);

      webParser.clearBuffer(param_value, queryMax);
      webParser.parseQuery(queryBuffer, "password", param_value);
      char pass[30];
      strcpy(pass,param_value);

       // ***************** LOGIN ********************   

      if(webParser.compare(username,user) && webParser.compare(loginpass,pass)) {
          
          arduinoSession = millis();
          //***** print out Session ID
          // Serial.println(arduinoSession);
          // successful login and redirect to a page
          client.println(F("HTTP/1.1 302 Found"));
          client.print(F("Set-cookie: ARDUINOSESSIONID="));
          client.print(arduinoSession);
          client.println(F("; HttpOnly"));       
          client.println(F("Location: /entry.htm")); 
          client.println();
        
      } else {
        // redirect back to login if wrong user / pass
          client.println(F("HTTP/1.1 302 Found"));
          client.println(F("Location: /logtut.htm"));
          client.println();
      } // if login
        
    } else if(webParser.contains(queryBuffer, "logout")) 
    {
          // kill session ID
          arduinoSession = 1;
          // redirect back to login if wrong user / pass
          client.println(F("HTTP/1.1 302 Found"));
          client.println(F("Location: /logtut.htm"));
          client.println();
    }

  }//end main else
}// end function

boolean loggedIn() 
{
   webParser.clearBuffer(param_value,queryMax);
   //going to need a parse cookie function
   webParser.parseQuery(queryBuffer, "ARDUINOSESSIONID", param_value);
  
   if(arduinoSession == atol(param_value)) {
      return true;
   } else {
      return false; 
   }
}


