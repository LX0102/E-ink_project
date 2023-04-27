#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "scripts.h"    
#include "css.h"        
#include "html.h"   
#include "epd.h"        
ESP8266WebServer server(80);
IPAddress myIP;       

void setup(void) {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA); //设置WIFI模块为STA模式
  Serial.println("\r\nWaiting for connection");
  WiFi.beginSmartConfig();
  Serial.println("Connecting to ");
  //判断网络状态是否连接上，没连接上就延时500ms，并且打出一个点，模拟正在连接
  while (WiFi.status() != WL_CONNECTED){
    Serial.print("<waiting...>");
    digitalWrite(LED_BUILTIN, 0);
    delay(2000);
    Serial.println(".");
  }
  Serial.println("");
  Serial.println("Connected, IP address: ");
  //输出station IP地址，这里的IP地址由DHCP分配
  Serial.println(WiFi.localIP());
  Serial.println("Setup End");

    // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("\r\nIP address: ");
  Serial.println(myIP = WiFi.localIP());

  // SPI initialization
  pinMode(CS_PIN  , OUTPUT);
  pinMode(RST_PIN , OUTPUT);
  pinMode(DC_PIN  , OUTPUT);
  pinMode(BUSY_PIN,  INPUT);
  SPI.begin();

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/styles.css", sendCSS);
  server.on("/processingA.js", sendJS_A);
  server.on("/processingB.js", sendJS_B);
  server.on("/processingC.js", sendJS_C);
  server.on("/processingD.js", sendJS_D);
  server.on("/LOAD", EPD_Load);
  server.on("/EPD", EPD_Init);
  server.on("/NEXT", EPD_Next);
  server.on("/SHOW", EPD_Show);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
  
}
void loop(void) {
  server.handleClient();
}
void EPD_Init()
{
  EPD_dispIndex = ((int)server.arg(0)[0] - 'a') +  (((int)server.arg(0)[1] - 'a') << 4);
  // Print log message: initialization of e-Paper (e-Paper's type)
  Serial.printf("EPD %s\r\n", EPD_dispMass[EPD_dispIndex].title);

  // Initialization
  EPD_dispInit();
  server.send(200, "text/plain", "Init ok\r\n");
}

void EPD_Load()
{
  //server.arg(0) = data+data.length+'LOAD'
  String p = server.arg(0);
  if (p.endsWith("LOAD")) {
    int index = p.length() - 8;
    int L = ((int)p[index] - 'a') + (((int)p[index + 1] - 'a') << 4) + (((int)p[index + 2] - 'a') << 8) + (((int)p[index + 3] - 'a') << 12);
    if (L == (p.length() - 8)) {
      Serial.println("LOAD");
      // if there is loading function for current channel (black or red)
      // Load data into the e-Paper
      if (EPD_dispLoad != 0) EPD_dispLoad();
    }
  }
  server.send(200, "text/plain", "Load ok\r\n");
}

void EPD_Next()
{
  Serial.println("NEXT");

  // Instruction code for for writting data into
  // e-Paper's memory
  int code = EPD_dispMass[EPD_dispIndex].next;

  // If the instruction code isn't '-1', then...
  if (code != -1)
  {
    // Do the selection of the next data channel
    EPD_SendCommand(code);
    delay(2);
  }
  // Setup the function for loading choosen channel's data
  EPD_dispLoad = EPD_dispMass[EPD_dispIndex].chRd;

  server.send(200, "text/plain", "Next ok\r\n");
}

void EPD_Show()
{
  Serial.println("\r\nSHOW\r\n");
  // Show results and Sleep
  EPD_dispMass[EPD_dispIndex].show();
  server.send(200, "text/plain", "Show ok\r\n");
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(200, "text/plain", message);
  Serial.print("Unknown URI: ");
  Serial.println(server.uri());
}
