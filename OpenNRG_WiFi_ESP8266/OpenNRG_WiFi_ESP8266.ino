/*
 * Copyright (c) 2015 Chris Howell
 *
 * This file is part of OpenNRG.
 * OpenNRG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * OpenNRG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with OpenNRG; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <EEPROM.h>

ESP8266WebServer server(80);

const char* ssid = "OpenNRG";
const char* password = "openenergy";
String st;
String privateKey = "";
String node = "";

const char* host = "data.openevse.com";
const char* e_url = "/emoncms/input/post.json?node=";
const char* inputID_CT1   = "OpenNRG_CT1:";
const char* inputID_CT2   = "OpenNRG_CT2:";
const char* inputID_CT3   = "OpenNRG_CT3:";
const char* inputID_VOLT   = "OpenNRG_VOLT:";
const char* inputID_TEMP1   = "OpenNRG_TEMP1:";
const char* inputID_TEMP2   = "OpenNRG_TEMP2:";
const char* inputID_TEMP3   = "OpenNRG_TEMP3:";

int ct1 = 0;
int ct2 = 0;
int ct3 = 0;
int volt = 0;
int temp1 = 0;
int temp2 = 0;
int temp3 = 0;

int wifi_mode = 0; 
int buttonState = 0;
int clientTimeout = 0;
int i = 0;
unsigned long Timer;

void ResetEEPROM(){
  //Serial.println("Erasing EEPROM");
  for (int i = 0; i < 512; ++i) { 
    EEPROM.write(i, 0);
    //Serial.print("#"); 
  }
  EEPROM.commit();   
}

void handleRoot() {
  String s;
  s = "<html><font size='20'><font color=006666>Open</font><b>Energy</b></font><p><b>Open Source Hardware</b><p>Wireless Configuration<p>Networks Found:<p>";
        //s += ipStr;
        s += "<p>";
        s += st;
        s += "<p>";
        s += "<form method='get' action='a'><label><b><i>WiFi SSID:</b></i></label><input name='ssid' length=32><p><label><b><i>Password  :</b></i></label><input name='pass' length=64><p><label><b><i>Emon Key:</b></i></label><input name='ekey' length=32><p><label><b><i>OpenEVSE:</b></i></label><select name='node'><option value='0'>0 - Default</option><option value='1'>1</option><option value='2'>2</option><option value='3'>3</option><option value='4'>4</option><option value='5'>5</option><option value='6'>6</option><option value='7'>7</option><option value='8'>8</option></select><p><input type='submit'></form>";
        s += "</html>\r\n\r\n";
  server.send(200, "text/html", s);
}

void handleCfg() {
  String s;
  String qsid = server.arg("ssid");
  String qpass = server.arg("pass");      
  String qkey = server.arg("ekey");
  String qnode = server.arg("node");
 
  qpass.replace("%23", "#");
  qpass.replace('+', ' ');
  
  if (qsid != 0){
     ResetEEPROM();
     for (int i = 0; i < qsid.length(); ++i){
      EEPROM.write(i, qsid[i]);
    }
    //Serial.println("Writing Password to Memory:"); 
    for (int i = 0; i < qpass.length(); ++i){
      EEPROM.write(32+i, qpass[i]); 
    }
    //Serial.println("Writing EMON Key to Memory:"); 
    for (int i = 0; i < qkey.length(); ++i){
      EEPROM.write(96+i, qkey[i]); 
    }
     
    EEPROM.write(129, qnode[i]);
    EEPROM.commit();
    s = "<html><font size='20'><font color=006666>Open</font><b>Energy</b></font><p><b>Open Source Hardware</b><p>Wireless Configuration<p>SSID and Password<p>";
    //s += req;
    s += "<p>Saved to Memory...<p>Wifi will reset to join your network</html>\r\n\r\n";
    server.send(200, "text/html", s);
    delay(2000);
    WiFi.disconnect();
    ESP.reset(); 
  }
  else {
     s = "<html><font size='20'><font color=006666>Open</font><b>Energy</b></font><p><b>Open Source Hardware</b><p>Wireless Configuration<p>Networks Found:<p>";
        //s += ipStr;
        s += "<p>";
        s += st;
        s += "<p>";
        s += "<form method='get' action='a'><label><b><i>WiFi SSID:</b></i></label><input name='ssid' length=32><p><font color=FF0000><b>SSID Required<b></font><p></font><label><b><i>Password  :</b></i></label><input name='pass' length=64><p><label><b><i>Emon Key:</b></i></label><input name='ekey' length=32><p><label><b><i>OpenEVSE:</b></i></label><select name='node'><option value='0'>0 - Default</option><option value='1'>1</option><option value='2'>2</option><option value='3'>3</option><option value='4'>4</option><option value='5'>5</option><option value='6'>6</option><option value='7'>7</option><option value='8'>8</option></select><p><input type='submit'></form>";
        s += "</html>\r\n\r\n";
     server.send(200, "text/html", s);
  }
}
void handleRst() {
  String s;
  s = "<html><font size='20'><font color=006666>Open</font><b>Energy</b></font><p><b>Open Source Hardware</b><p>Wireless Configuration<p>Reset to Defaults:<p>";
  s += "<p><b>Clearing the EEPROM</b><p>";
  s += "</html>\r\n\r\n";
  ResetEEPROM();
  EEPROM.commit();
  server.send(200, "text/html", s);
  WiFi.disconnect();
  delay(1000);
  ESP.reset();
}


void handleStatus() {
  String s;
  s = "<html><iframe style='width:480px; height:320px;' frameborder='0' scrolling='no' marginheight='0' marginwidth='0' src='http://data.openevse.com/emoncms/vis/rawdata?feedid=1&fill=0&colour=008080&units=W&dp=1&scale=1&embed=1'></iframe>";
  s += "</html>\r\n\r\n";

  server.send(200, "text/html", s);
  
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  pinMode(0, INPUT);
  // read eeprom for ssid and pass
  char tmpStr[40];
  String esid;
  String epass = "";
 
  for (int i = 0; i < 32; ++i){
    esid += char(EEPROM.read(i));
  }
  for (int i = 32; i < 96; ++i){
    epass += char(EEPROM.read(i));
  }
  for (int i = 96; i < 128; ++i){
    privateKey += char(EEPROM.read(i));
  }
  node += char(EEPROM.read(129));
    
   if ( esid != 0 ) { 
    //Serial.println(" ");
    //Serial.print("Connecting as Wifi Client to: ");
    //Serial.println(esid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(); 
    WiFi.begin(esid.c_str(), epass.c_str());
    delay(50);
    int t = 0;
    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED){
      // test esid
      //Serial.print("#");
      delay(500);
      t++;
      if (t >= 20){
        //Serial.println(" ");
        //Serial.println("Trying Again...");
        delay(2000);
        WiFi.disconnect(); 
        WiFi.begin(esid.c_str(), epass.c_str());
        t = 0;
        attempt++;
        if (attempt >= 5){
          //Serial.println();
          //Serial.print("Configuring access point...");
          WiFi.mode(WIFI_STA);
          WiFi.disconnect();
          delay(100);
          int n = WiFi.scanNetworks();
          //Serial.print(n);
          //Serial.println(" networks found");
          st = "<ul>";
          for (int i = 0; i < n; ++i){
            st += "<li>";
            st += WiFi.SSID(i);
            st += "</li>";
          }
          st += "</ul>";
          delay(100);
          WiFi.softAP(ssid, password);
          IPAddress myIP = WiFi.softAPIP();
          //Serial.print("AP IP address: ");
          //Serial.println(myIP);
          wifi_mode = 1;
          break;
        }
      }
    }
  }
  else {
    //Serial.println();
    //Serial.print("Configuring access point...");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    int n = WiFi.scanNetworks();
    st = "<ul>";
    for (int i = 0; i < n; ++i){
      st += "<li>";
      st += WiFi.SSID(i);
      st += "</li>";
    }
    st += "</ul>";
    delay(100);
    WiFi.softAP(ssid, password);
    IPAddress myIP = WiFi.softAPIP();
    //Serial.print("AP IP address: ");
    //Serial.println(myIP);
    wifi_mode = 2; //AP mode with no SSID in EEPROM
  }
  
  if (wifi_mode == 0){
    //Serial.println(" ");
    //Serial.println("Connected as a Client");
    IPAddress myAddress = WiFi.localIP();
    //Serial.println(myAddress);
  }
  
 
  server.on("/", handleRoot);
  server.on("/a", handleCfg);
  server.on("/reset", handleRst);
  server.on("/status", handleStatus);
  server.begin();
  //Serial.println("HTTP server started");
  delay(100);
  Timer = millis();
}

void loop() {
server.handleClient();

int erase = 0;  
buttonState = digitalRead(0);
while (buttonState == LOW) {
    buttonState = digitalRead(0);
    erase++;
    if (erase >= 1000) {
        ResetEEPROM();
        int erase = 0;
        WiFi.disconnect();
        Serial.println("Finished...");
        delay(1000);
        ESP.reset(); 
     } 
   }

   // Remain in AP mode for 5 Minutes before resetting
if (wifi_mode == 1){
   if ((millis() - Timer) >= 300000){
     ESP.reset();
   }
} 

if (wifi_mode == 0 && privateKey != 0){
   if ((millis() - Timer) >= 30000){
     Timer = millis();
     Serial.flush();
     Serial.println("$GP \r");
     delay(100);
       while(Serial.available()) {
         String apiString = Serial.readStringUntil('\r');
         if ( apiString.startsWith("$OK ") ) {
           String qapi; 
           qapi = apiString.substring(apiString.indexOf(' '));
           ct1 = qapi.toInt();
           String qapi1;
           int firstapiCmd = apiString.indexOf(' ');
           qapi1 = apiString.substring(apiString.indexOf(' ', firstapiCmd + 1 ));
           ct2 = qapi1.toInt();
           String qapi2;
           qapi2 = apiString.substring(apiString.lastIndexOf(' '));
           ct3 = qapi2.toInt();
         }
       } 
       delay(100);
       Serial.flush();
       Serial.print("$GV \r");
       delay(100);
       while(Serial.available()) {
         String apiString = Serial.readStringUntil('\r');
         Serial.print(apiString);
         if ( apiString.startsWith("$OK") ) {
           String qapi; 
           qapi = apiString.substring(apiString.indexOf(' '));
           volt = qapi.toInt();
         }
       }  
       delay(100);
       Serial.flush();
       Serial.print("$GT \r");
       delay(100);
       while(Serial.available()) {
         String apiString = Serial.readStringUntil('\r');
         Serial.print(apiString);
         if (apiString.startsWith("$OK") ) {
           String qapi; 
           qapi = apiString.substring(apiString.indexOf(' '));
           temp1 = qapi.toInt();
           String qapi1;
           int firstapiCmd = apiString.indexOf(' ');
           qapi1 = apiString.substring(apiString.indexOf(' ', firstapiCmd + 1 ));
           temp2 = qapi1.toInt();
           String qapi2;
           qapi2 = apiString.substring(apiString.lastIndexOf(' '));
           temp3 = qapi2.toInt();
         }
       } 
 
// Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    return;
  }
  
// We now create a URL for OpenNRG api data upload request
  String url = e_url;
  String url_ct1 = inputID_CT1;
    url_ct1 += ct1;
    url_ct1 += ",";
  String url_ct2 = inputID_CT2;
    url_ct2 += ct2;
    url_ct2 += ",";
  String url_ct3 = inputID_CT3;
    url_ct3 += ct3;
    url_ct3 += ",";
  String url_volt = inputID_VOLT;
    url_volt += volt;
    url_volt += ",";
  String url_temp1 = inputID_TEMP1;
    url_temp1 += temp1;
    url_temp1 += ",";
  String url_temp2 = inputID_TEMP2;
    url_temp2 += temp2;
    url_temp2 += ","; 
  String url_temp3 = inputID_TEMP3;
    url_temp3 += temp3;
     
  
  url += node;
  url += "&json={";
  url += url_ct1;
  url += url_ct2;
  url += url_ct3;
  url += url_volt;
  url += url_temp1;
  url += url_temp2;
  url += url_temp3;
  url += "}&apikey=";
  url += privateKey.c_str();
    
// This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");
  delay(10);
  while(client.available()){
    String line = client.readStringUntil('\r');
  }
  //Serial.println(host);
  //Serial.println(url);
  
 

  }
}

}
