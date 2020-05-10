//MKZ4 LED light + BGM2(by DFpalyermini)
/*
 * Copyright (c) 2015, Majenko Technologies
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 * 
 * * Neither the name of Majenko Technologies nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Create a WiFi access point and provide a web server on it. */

#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <EEPROM.h>

#include "Arduino.h"
#include <SoftwareSerial.h>
#include "DFRobotDFPlayerMini.h"

/* Set these to your desired credentials. */
const char *ssid = "MKZ4_BGM2";
const char *password = "";
const char command_offset[6]={'o','f','f','s','e','t'};
const char command_left[4]={'l','e','f','t'};
const char command_right[5]={'r','i','g','h','t'};

ESP8266WebServer server(80);
ESP8266WebServer server_8080(8080);

SoftwareSerial mySoftwareSerial(13, 5); // RX, TX
DFRobotDFPlayerMini myDFPlayer;
void printDetail(uint8_t type, int value);

/* set I2C library*/
#include <Wire.h>
#define ADDR1  0x64

#define command_start  0
#define command_stop   1
#define command_back  2
#define forward       0x01
#define reverse       0x02

#define LED_H       (digitalWrite( 12, HIGH ))
#define LED_L       (digitalWrite( 12, LOW ))

//TP1=>R6  GPIO15
#define HLED_H       (analogWrite( 15, 0 ))
#define HLED_M       (analogWrite( 15, 768 ))

#define eeprom_size   (128)
#define offset_address (0) 
#define left_address   (1)
#define right_address  (2) 


char state = command_stop;
int offset = 0;
int servo_left=65;
int servo_right=105;

String form ="<html>"
"<head>"
"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, minimum-scale=1, maximum-scale=1\">"
"<style>"
"* { padding: 0; margin: 0; }"
"body { background-color: #0097C1; }"
"</style>"
"</head>"
"<body>"
"<div style=\"position:fixed;top: 50pt; text-align:center; width: 100%; color:white; font-size:300%; font-weight:bold; text-transform:uppercase; font-family:sans-serif\" id=\"value\">connected</div>"
"<form action=\"\" target=\"tif\" id=\"form\">"
"<iframe src=\"javascript: false;\" style=\"display: none;\" name=\"tif\" id=\"tif\"></iframe>"
"</form>"
"<script>"
"var offset = 50;"
"document.body.style.height = document.body.clientHeight + offset + \'px\';"
"document.body.style.width = document.body.clientWidth + offset + \'px\';"
"document.getElementsByTagName(\"html\")[0].style.height = document.body.style.height + \'px\';"
"document.getElementsByTagName(\"html\")[0].style.width = document.body.style.width + \'px\';"
"var moveHomePosition = function() {"
    "document.body.scrollTop = offset / 2;"
    "document.body.scrollLeft = offset / 2;"
"};"
"setTimeout(moveHomePosition, 500);"
"var startX = 0;var startY = 0;var command =\'/stop\';"
"var threshold = 40;"
"var esp_port = \'http://192.168.4.1:8080\';"
"var el_form = document.getElementById(\'form\');"
"document.body.ontouchstart = function(event) {"
    "startX = event.touches[0].clientX;"
    "startY = event.touches[0].clientY;"
"};"
"document.body.ontouchmove = function(event) {"
    "var x = parseInt(event.touches[0].clientX - startX);"
    "var y = parseInt(event.touches[0].clientY - startY);"
    "var url = null;"
    "if (x < (threshold * -1)) {"
       "if (y < (threshold * -1)){"
          "url = \'/left<br>forward\';"
       "} else if (y > threshold) {"
          "url = \'/left<br>back\';"
       "}else {"
        "url = \'/left\';"
       "}"
    "} else if (x > threshold) {"
       "if (y < (threshold * -1)) {"
          "url = \'/right<br>forward\';"
      "} else if (y > threshold) {"
          "url = \'/right<br>back\';"
      "}else{"
      
          "url = \'/right\';"
      "}"
     "} else {"
        "if (y < (threshold * -1)) {"
            "url = \'/forward\';"
        " } else if (y > threshold) {"
            "url = \'/back\';"
        "}"
     "}"
    "if (command != url) {"
      "if (url) {"
          "el_form.action = esp_port + url.replace(\"<br>\",\"\");"
          "el_form.submit();"
          "document.getElementById(\'value\').innerHTML = url.replace(\"/\",\"\");"
      "}"
    "}"
    "command = url;"
"};"
"document.body.ontouchend = function(event) {"
    "el_form.action = esp_port + \'/stop\';"
    "el_form.submit();"
    "setTimeout(moveHomePosition, 50);"
   "document.getElementById(\'value\').innerHTML = \'stop\';"
"};"
"</script>"
"</body>"
"</html>";

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
 * connected to this access point to see it.
 */
void setup() {
  
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point...");

  mySoftwareSerial.begin(9600);

  Wire.begin(4, 14);
  delay(40);

  EEPROM.begin(eeprom_size);
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
 
  server.on("/", handleRoot);
  server_8080.on("/stop", handle_stop);
  server_8080.on("/forward", handle_forward);
  server_8080.on("/back", handle_back);
  server_8080.on("/left", handle_left);
  server_8080.on("/right", handle_right);
  server_8080.on("/leftforward", handle_f_left);
  server_8080.on("/rightforward", handle_f_right);
  server_8080.on("/leftback", handle_b_left);
  server_8080.on("/rightback", handle_b_right);
 
  server.begin();
  server_8080.begin();
  
  Serial.println("HTTP server started");
  pinMode(16,OUTPUT);
  pinMode(12,OUTPUT);

  LED_H;
  HLED_M;
  
//DFPlayer setup
  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

  if (!myDFPlayer.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true);
  }
  Serial.println(F("DFPlayer Mini online."));
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  myDFPlayer.volume(25);  //Set volume value. From 0 to 30


  myDFPlayer.loop(1);  //Loop the first mp3 

   
  delay(100);

  if((EEPROM.read(offset_address) != 0)
   && (EEPROM.read(offset_address) != 0xff)){
     offset =(int8)( EEPROM.read(offset_address));
   }

  Serial.print(offset);
  Serial.println();

  if((EEPROM.read(left_address) != 0)
   && (EEPROM.read(left_address) != 0xff)){
    servo_left = EEPROM.read(left_address);
  }
  Serial.print(servo_left);
  Serial.println();
  
  if((EEPROM.read(right_address) != 0)
   && (EEPROM.read(right_address) != 0xff)){
    servo_right = EEPROM.read(right_address);
  }
  
  Serial.print(servo_right);
  Serial.println();
}

void loop() {
  server.handleClient();
  server_8080.handleClient();
  read_calib_data();
}


void handleRoot() {
  server.send(200, "text/html", form);
}


void handle_stop() {
  Serial.print("stop\r\n");
  LED_L;
  HLED_M;
    stop_motor();
    state = command_stop;
  LED_H;
  server_8080.send(200, "text/html", "");
}

void handle_forward() {
 Serial.print("forward\r\n");
  HLED_H;
  drive();
  servo_control(90);
  server_8080.send(200, "text/html", "");
}

void handle_back() {
  Serial.print("back\r\n");
  HLED_H;
  back();
  servo_control(90);
  server_8080.send(200, "text/html", "");
}

void handle_left(){
  Serial.print("left\r\n");
  HLED_H;
  servo_control(servo_left);
  server_8080.send(200, "text/html", "");
}

void handle_right(){
  Serial.print("right\r\n");
  HLED_H;
  servo_control(servo_right);
  server_8080.send(200, "text/html", "");
}

void handle_f_left(){
  Serial.print("f_left\r\n");
  HLED_H;
  drive();
  servo_control(servo_left);
  server_8080.send(200, "text/html", "");
}

void handle_f_right(){
  Serial.print("f_right\r\n");
  HLED_H;
  drive();
  servo_control(servo_right);
 server_8080.send(200, "text/html", "");
}

void handle_b_left(){
  Serial.print("b_left\r\n");
  HLED_H;
  back();
  servo_control(servo_left );
  server_8080.send(200, "text/html", "");
}


void handle_b_right(){
  Serial.print("b_right\r\n");
  HLED_H;
  back();
  servo_control(servo_right);
  server_8080.send(200, "text/html", "");
}

void drive(){
    if(state == command_back){
     stop_motor();

     delay(10);

     start_motor();
  
  }else if(state == command_stop){
    start_motor();
  }
  state = command_start;
}

void back(){
    if(state == command_start){
     stop_motor();
     delay(10);
     reverse_motor();
  }else if(state == command_stop){
    reverse_motor();
  }
  state = command_back;
}

void start_motor(){
  char i, volt;
  volt = 0x20;
  for(i=0;i<5;i++){ 
    volt = volt + ((0x40)*i);
    volt = volt | forward;
    motor_func(ADDR1 , volt);
    delay(10);
  }
}

void reverse_motor(){
    char i, volt;
  volt = 0x20;
  for(i=0;i<5;i++){ 
    volt = volt + ((0x40)*i);
    volt = volt | reverse;
    motor_func(ADDR1 , volt);
    delay(10);
  }
}

void stop_motor(){
  motor_func(ADDR1 , 0x18);
  delay(10);
  motor_func(ADDR1 , 0x1B);
  delay(10);
}

void motor_func(char add , char duty){
  Wire.beginTransmission(add);
  Wire.write(0x00);
  Wire.write(duty);
  Wire.endTransmission();
}

void servo_control(int angle){
int microsec,i;
      LED_L;
      microsec = (5*(angle+offset))+ 1000;
       
      for(i=0; i<20 ;i++){
        digitalWrite( 16, HIGH );
        delayMicroseconds( microsec ); 
        digitalWrite( 16, LOW );
        delayMicroseconds( 10000 - microsec ); 
      }
      LED_H;
}

void read_calib_data(){

  static char buf[128] = {0};
  size_t len = 0;
  static int index = 0;
  
  while(Serial.available()){
      len = Serial.available();
      index += Serial.readBytes(&buf[index],len);
      if(buf[index-2] ==0x0d && buf[index-1] == 0xa){
        parse_command(buf);
        index = 0;
        memset(buf,0,sizeof(buf));  
      }
    }
}

void parse_command(char *buf){
  uint8 data = 0;
  
  //offset
  if(strncmp(command_offset,buf,sizeof(command_offset))==0){
    data = atoi(&buf[sizeof(command_offset)+1]);
    offset =(int8)data;
    EEPROM.write(offset_address, data);
    EEPROM.commit();
    Serial.print("save offset\t");
    Serial.print((int8)data);
    Serial.println();
  }

  //left
  if(strncmp(command_left,buf,sizeof(command_left))==0){
    data = atoi(&buf[sizeof(command_left)+1]);
    servo_left = data;
    EEPROM.write(left_address, data);
    EEPROM.commit();
    Serial.print("save left\t");
    Serial.print(data);
    Serial.println();
  }

    //right
  if(strncmp(command_right,buf,sizeof(command_right))==0){
    data = atoi(&buf[sizeof(command_right)+1]);
    servo_right = data;
    EEPROM.write(right_address, data);
    EEPROM.commit();
    Serial.print("save right\t");
    Serial.print(data);
    Serial.println();
  }
}

void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}
