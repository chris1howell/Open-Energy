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
#include "ESP8266WiFi.h"
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <EEPROM.h>

const char* ssid = "OpenNRG";
const char* password = "openenergy";
String st;
String privateKey = "";
String node = "";

const char* host = "data.openevse.com";
String url = "/emoncms/input/post.json?node=";

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

int buttonState = 0;
int clientTimeout = 0;

MDNSResponder mdns;
WiFiServer server(80);

void ResetEEPROM()
{
  Serial.println("Erasing EEPROM");
  for (int i = 0; i < 512; ++i) { 
    EEPROM.write(i, 0);
    //Always print this so user can see activity during button reset
    Serial.print("#"); 
  }
  EEPROM.commit();   
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  pinMode(0, INPUT);
  // read eeprom for ssid and pass
  String esid;
  for (int i = 0; i < 32; ++i)
    {
      esid += char(EEPROM.read(i));
    }
  String epass = "";
  for (int i = 32; i < 96; ++i)
    {
      epass += char(EEPROM.read(i));
    }
  for (int i = 96; i < 128; ++i)
    {
      privateKey += char(EEPROM.read(i));
    }
    node += char(EEPROM.read(129));
    
  if ( esid.length() > 1 ) { 
    if (WiFi.status() != WL_CONNECTED){
      // test esid
      WiFi.mode(WIFI_STA);
      WiFi.disconnect(); 
      WiFi.begin(esid.c_str(), epass.c_str());
      delay(50);
    }
 if ( testWifi() == 20 ) { 
     //launchWeb(0);
      return;
      }
  else {
    setupAP(); 
  }
 }
} 

int testWifi(void) {
  int c = 0;
  Serial.println("Waiting for Wifi to connect");  
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED) { return(20); } 
    delay(500);
    Serial.println(WiFi.status());    
    c++;
    int erase = 0;  
buttonState = digitalRead(0);
while (buttonState == LOW) {
    buttonState = digitalRead(0);
    erase++;
    if (erase >= 1000) {
        ResetEEPROM();
        int erase = 0;
        WiFi.disconnect();
        Serial.print("Finished...");
        delay(1000);
        ESP.reset(); 
     } 
   }

  }
  return(10);
} 

void launchWeb(int webtype) {
          Serial.println(WiFi.localIP());
          Serial.println(WiFi.softAPIP());
          if (!mdns.begin("esp8266", WiFi.localIP())) {
            Serial.println("Error setting up MDNS responder!");
            while(1) { 
              delay(1000);
            }
          }
          server.begin();
          Serial.println("Server started");   
          int b = 20;
          int c = 0;
          while(b == 20) { 
             b = mdns1(webtype);
           }
}

void setupAP(void) {
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
    Serial.print(n);
    Serial.println(" networks found");
    
  st = "<ul>";
  for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      st += "<li>";
      st += WiFi.SSID(i);
      st += "</li>";
    }
  st += "</ul>";
  delay(100);
  WiFi.softAP(ssid, password);
  Serial.println("Access Point Mode");
  Serial.println("");
  delay(2000);
  IPAddress ip = WiFi.softAPIP();
  Serial.println(".....");
  delay(50); 
  launchWeb(1);
}

int mdns1(int webtype)
{
  // Check for any mDNS queries and send responses
  mdns.update();
  
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    delay(1);
    clientTimeout++;
    if (clientTimeout >= 300000){
      Serial.print("Client timout - Rebooting");
      //ESP.deepSleep(10000, WAKE_RF_DEFAULT);
      ESP.reset();
    }
    return(20);
  }
  // Wait for data from client to become available
  int i = 0;
  while(client.connected() && !client.available()){
    delay(1);
   }
  
  // Read the first line of HTTP request
  String req = client.readStringUntil('\r');
  
  // First line of HTTP request looks like "GET /path HTTP/1.1"
  // Retrieve the "/path" part by finding the spaces
  int addr_start = req.indexOf(' ');
  int addr_end = req.indexOf(' ', addr_start + 1);
  if (addr_start == -1 || addr_end == -1) {
    Serial.print("Invalid request: ");
    Serial.println(req);
    return(20);
   }
  req = req.substring(addr_start + 1, addr_end);
  client.flush(); 
  String s;
  if ( webtype == 1 ) {
      if (req == "/")
      {
        IPAddress ip = WiFi.softAPIP();
        String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html><font size='20'><font color=006666>Open</font><b>NRG</b></font><p><b>Open Source Hardware</b><p>Wireless Configuration<p>Networks Found:<p>";
        //s += ipStr;
        s += "<p>";
        s += st;
        s += "<p>";
        s += "<form method='get' action='a'><label><b><i>WiFi SSID:</b></i></label><input name='ssid' length=32><p><label><b><i>Password  :</b></i></label><input name='pass' length=64><p><label><b><i>Emon Key:</b></i></label><input name='ekey' length=32><p><label><b><i>OpenENrg:</b></i></label><select name='node'><option value='9'>9 - Default</option><option value='1'>1</option><option value='2'>2</option><option value='3'>3</option><option value='4'>4</option><option value='5'>5</option><option value='6'>6</option><option value='7'>7</option><option value='8'>8</option></select><p><input type='submit'></form>";
        s += "</html>\r\n\r\n";
        client.print(s);
      }
      else if ( req.startsWith("/a?ssid=") ) {
        
        ResetEEPROM();
        
        String qsid;
        String qpass;
        String qkey;
        String qnode; 
        
        int idx = req.indexOf("ssid=");
        int idx1 = req.indexOf("&pass=");
        int idx2 = req.indexOf("&ekey=");
        int idx3 = req.indexOf("&node=");
        
        qsid = req.substring(idx+5,idx1);
        qpass = req.substring(idx1+6,idx2);
        qkey = req.substring(idx2+6, idx3);
        qnode = req.substring(idx3+6);
        
        Serial.println(qsid);
        Serial.println("");
	
        qpass.replace("%23", "#");
        qpass.replace('+', ' ');
                
        
        for (int i = 0; i < qsid.length(); ++i)
          {
            EEPROM.write(i, qsid[i]);
            Serial.print("Wrote: ");
            Serial.println(qsid[i]); 
          }
        Serial.println("Writing Password to Memory:"); 
        for (int i = 0; i < qpass.length(); ++i)
          {
            EEPROM.write(32+i, qpass[i]);
            Serial.print("Wrote: ");
            Serial.println("*"); 
          }
      Serial.println("Writing EMON Key to Memory:"); 
        for (int i = 0; i < qkey.length(); ++i)
          {
            EEPROM.write(96+i, qkey[i]);
            Serial.print("Wrote: ");
            Serial.println(qkey[i]); 
          }
      Serial.println("Writing EMOM Node to Memory:"); 
            EEPROM.write(129, qnode[i]);
            Serial.print("Wrote: ");
            Serial.println(qnode[i]);        
        EEPROM.commit();
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html><font size='20'><font color=006666>Open</font><b>NRG</b></font><p><b>Open Source Hardware</b><p>Wireless Configuration<p>SSID and Password<p>";
        //s += req;
        s += "<p>Saved to Memory...<p>Wifi will reset to join your network</html>\r\n\r\n";
        client.print(s);
        delay(2000);
        WiFi.disconnect();
        ESP.reset();
      }
      else if ( req.startsWith("/reset") ) {
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html><font size='20'><font color=006666>Open</font><b>NRG</b></font><p><b>Open Source Hardware</b><p>Wireless Configuration<p>Reset to Defaults:<p>";
        s += "<p><b>Clearing the EEPROM</b><p>";
        s += "</html>\r\n\r\n";
        ResetEEPROM();
        EEPROM.commit();
        client.print(s);
        WiFi.disconnect();
        delay(1000);
        ESP.reset();
      }
      else
      {
        s = "HTTP/1.1 404 Not Found\r\n\r\n";
        client.print(s);
       }       
  }
  
  return(20);
}

void loop() {
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

 Serial.flush();
 delay(2000);
 Serial.print("$GP \r");
 delay(1000);
 while(Serial.available()) {
   String apiString = Serial.readStringUntil('\r');
   Serial.print(apiString);
     if (apiString.startsWith("$OK") ) {
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
   
 
 Serial.flush();
 delay(2000);
 Serial.print("$GV \r");
 delay(1000);
 while(Serial.available()) {
   String apiString = Serial.readStringUntil('\r');
   Serial.print(apiString);
     if ( apiString.startsWith("$OK") ) {
        String qapi; 
        qapi = apiString.substring(apiString.indexOf(' '));
        volt = qapi.toInt();
        }
 }  

Serial.flush();
delay(2000);
Serial.print("$GT \r");
 delay(1000);
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
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  delay(10);
 
 
  while(client.available()){
    String line = client.readStringUntil('\r');
    }
    
 
  Serial.println(host);
  Serial.println(url);
  
  //delay(10000);
 ESP.deepSleep(25000000, WAKE_RF_DEFAULT);
 
}

