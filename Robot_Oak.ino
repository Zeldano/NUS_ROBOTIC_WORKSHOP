
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <WebSocketsServer.h>
#include <WebServer.h>
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include <Adafruit_NeoPixel.h>

SSD1306AsciiWire oled;
WiFiMulti WiFiMulti;
WebSocketsServer webSocket = WebSocketsServer(81);
WebServer server(80);

IPAddress local_IP(192, 168, x, xxx);
// replace with own ip adress----------------------------------------------------------------------------------------------------------------------------------------
/* 
 * type ping 192.168.x.xxx in your terminal to see if this adress is available. 
 * if you see a request time out, then that means this adress is available and you can use it
 * if you do not see a request time out, then try a different number for the leats significant part, the leftmost 3 numbers
 * you can try any number between 1 and 255.
 */

IPAddress gateway(192, 168, 1, 254);
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8); 
IPAddress secondaryDNS(8, 8, 4, 4);

#define PEXP_I2CADDR 0x23
#define I2CADDR 0x13
#define I2CADDR_B 0x12
#define I2C_SDA 19
#define I2C_SCL 18
#define LED 2
#define IR_RECV 4
#define NEO_PIXEL 5
#define LED_COUNT 3
#define CW 1
#define OLED_I2CAADR 0x3C
#define CCW 2

Adafruit_NeoPixel strip(3, NEO_PIXEL, NEO_GRB + NEO_KHZ800);


const char* ssid = "";
// replace with own ssid

const char* password = "";
// replace with own password

int robot_direction = 0;
bool ledState = 0;
byte attinySlaveArrayBoard[3];
char text[10];
uint8_t  i, nec_state = 0; 
unsigned int command, address;
static unsigned long nec_code;
static boolean nec_ok = false;
static unsigned long timer_value_old;
static unsigned long timer_value;
uint8_t expanderdata;
int mainled1_state = 0;
int mainled2_state = 0;
int min1 = random(0, 180);
int min2 = random(0, 180);
int r1 = 255;
int g1 = min1;
int b1 = min1;
int r2 = 255;
int g2 = min2;
int b2 = min2;
int mainled1_command;
int mainled2_command;
int intenval = 4;
int pid_command = 0;
float Kp=4.2, Ki=0.05, Kd=0;
const float minSpeed = 0.5;
const float maxSpeed = 15;
float error=0, P=0, I=0, D=0, PID_value=0;
float previousError=0;
uint8_t sensor[5] = {0, 0, 0, 0, 0};
int start = 0;
int8_t pidErrorMap[9][6] = 
{
    {1, 1, 1, 1, 0, 4},
    {1, 1, 1, 0, 0, 3},
    {1, 1, 1, 0, 1, 2},
    {1, 1, 0, 0, 1, 1},
    {1, 1, 0, 1, 1, 0},
    {1, 0, 0, 1, 1, -1},
    {1, 0, 1, 1, 1, -2},
    {0, 0, 1, 1, 1, -3},
    {0, 1, 1, 1, 1, -4},    
};
uint8_t expanderData;


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
  html {
    font-family: Arial, Helvetica, sans-serif;
    text-align: center;
  }
  </style>
<title>ESP Web Server</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" href="data:,">
</head>
<body>
  <div class="topnav">
    <h1>C3 CoreModule - CockroachBot WebSocket Server</h1>
  </div>
  <div class="content">
    <div class="card">
      <h2>LED - GPIO 2</h2>
      <p class="state">state: <span id="state">-</span></p>
      <p><button id="button" class="button">Toggle</button></p>
    </div>
  </div>
<script>
  var gateway = `ws:
  var websocket;
  window.addEventListener('load', onLoad);
  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage; 
  }
  function onOpen(event) {
    console.log('Connection opened');
  }
  function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
  }
  function onMessage(event) {
    var state;
    if (event.data == "LED=1"){
      state = "ON";
    }
    else if (event.data == "LED=0"){
      state = "OFF";
    }
    document.getElementById('state').innerHTML = state;
  }
  function onLoad(event) {
    initWebSocket();
    initButton();
  }
  function initButton() {
    document.getElementById('button').addEventListener('click', toggle);
  }
  function toggle(){
    websocket.send('toggle');
  }
</script>
</body>
</html>
)rawliteral";



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
  server.send(404, "text/plain", message);
}



void setinten(int intenoper){
  if (intenval > 3 or intenoper > 0){
    if (intenval < 65 or intenoper < 0){
      intenval += intenoper;
    }
  }
  strip.setBrightness(intenval);
  Serial.println(intenval);
  strip.show();
}


void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket.remoteIP(num);
                Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        webSocket.sendTXT(num, "Connected");
        webSocket.sendTXT(num, ledState? "LED=1": "LED=0");
        webSocket.sendTXT(num, "DIRECTION=" + robot_direction);
            }
            break;
        case WStype_TEXT:
            Serial.printf("[%u] get Text: %s\n", num, payload);
            if (strcmp((char*)payload, "toggle") == 0) {
              Serial.println("Recieved switching lED");
              ledState = !ledState;
              webSocket.broadcastTXT(ledState? "LED=1": "LED=0");
            }
            else if (strcmp((char*)payload, "foward") == 0 | strcmp((char*)payload, "Foward") == 0) {
              Serial.println("Recieved moving forward");
              robot_direction = 1;
              webSocket.broadcastTXT("DIRECTION=1");
            }
            else if (strcmp((char*)payload, "backward") == 0 | strcmp((char*)payload, "Backward") == 0) {
              Serial.println("Recieved moving backward");
              robot_direction = 2;
              webSocket.broadcastTXT("DIRECTION=2");
            }
            else if (strcmp((char*)payload, "left") == 0 | strcmp((char*)payload, "Left") == 0) {
              Serial.println("Recieved turning left");
              robot_direction = 3;
              webSocket.broadcastTXT("DIRECTION=3");
            }
            else if (strcmp((char*)payload, "leftnin") == 0) {
              Serial.println("Recieved turning left 90??");
              robot_direction = 390;
              webSocket.broadcastTXT("DIRECTION=5");
            }
            else if (strcmp((char*)payload, "right") == 0 | strcmp((char*)payload, "Right") == 0) {
              Serial.println("Recieved turning right");
              robot_direction = 4;
              webSocket.broadcastTXT("DIRECTION=4");
            }
            else if (strcmp((char*)payload, "rightnin") == 0) {
              Serial.println("Recieved turning right 90??");
              robot_direction = 490;
              webSocket.broadcastTXT("DIRECTION=6");
            }
            else if (strcmp((char*)payload, "stop") == 0 | strcmp((char*)payload, "Stop") == 0) {
              Serial.println("Recieved to stop");
              robot_direction = 0;
              webSocket.broadcastTXT("DIRECTION=0");
            }
            else if (strcmp((char*)payload, "led1sw") == 0) {
              Serial.println("Recieved to switch on/off main LED1");
              mainled1_command = 1;
            }
            else if (strcmp((char*)payload, "led1colorsw") == 0) {
              Serial.println("Recieved to change colors of main LED1");
              mainled1_command = 2;
            }
            else if (strcmp((char*)payload, "led1colorcycle") == 0) {
              Serial.println("Recieved to turn on/off color cycle for main LED1");
              mainled1_command = 3;
            }
            else if (strcmp((char*)payload, "led2sw") == 0) {
              Serial.println("Recieved to switch on/off main LED2");
              mainled2_command = 1;
            }
            else if (strcmp((char*)payload, "led2colorsw") == 0) {
              Serial.println("Recieved to change colors of main LED2");
              mainled2_command = 2;
            }
            else if (strcmp((char*)payload, "led2colorcycle") == 0) {
              Serial.println("Recieved to turn on/off color cycle for main LED2");
              mainled2_command = 3;
            }   
            else if (strcmp((char*)payload, "ledintenup") == 0) {
              Serial.println("Recieved to increase brightness");
              setinten(3);
            }
            else if (strcmp((char*)payload, "ledintendown") == 0) {
              Serial.println("Recieved to decrease brightness");
              setinten(-3);
            }
            else if (strcmp((char*)payload, "pid") == 0) {
              Serial.println("Recieved to activate/deactivate pid");
              (pid_command == 0) ? pid_command = 1 ? pid_command = 0;       
            break;
        case WStype_BIN:
            break;
    case WStype_ERROR:      
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;
    }
}




void IOexpanderWrite(byte address, byte _data) {
    Wire.beginTransmission(address);
    Wire.write(_data);
    Wire.endTransmission(); 
}


uint8_t IOexpanderRead(int address) {
    uint8_t _data;
    Wire.requestFrom(address, 1);
    if(Wire.available()) {
        _data = Wire.read();
    }
    return _data;
}




int setDirection(int motor, byte direction) {
    attinySlaveArrayBoard[0] = motor == 0 ? 0x13 : 0x23;  
    attinySlaveArrayBoard[1] = (direction >= 0) && (direction <= 2) ? direction: 0;  
    attinySlaveArrayBoard[2] = 0x00;  
    delay(10);
    Wire.beginTransmission(I2CADDR_B);
    Wire.write(attinySlaveArrayBoard, 3);
    if (Wire.endTransmission () == 0) { 
        Serial.println("i2c Write to 0x12 Sucessfull");
        return 0;
    }
    else {
        Serial.println(Wire.endTransmission());
        Serial.println("i2c Write Failed");
        return 1;
    }
}
int setMotorRunning(uint8_t motorState) {
    attinySlaveArrayBoard[0] = 0x01;  
    attinySlaveArrayBoard[1] = motorState? 0x01:0x00;  
    attinySlaveArrayBoard[2] = 0x00;  
    delay(20);
    Wire.beginTransmission(I2CADDR_B);
    Wire.write(attinySlaveArrayBoard, 3); 
    if (Wire.endTransmission () == 0) { 
        Serial.println("i2c Write to 0x12 Sucessfull");
        return 0;
    }
    else {
        Serial.println(Wire.endTransmission());
        Serial.println("i2c Write Failed");
        return 1;
    }
}
int setRPM(int motor, float rpm) {
    unsigned int rpm_x_100 = (int) (rpm * 100);
    attinySlaveArrayBoard[0] = motor == 0 ? 0x14 : 0x24;
    attinySlaveArrayBoard[1] = (rpm_x_100 & 0xff);
    attinySlaveArrayBoard[2] = (rpm_x_100 >> 8) & 0xff;   
    delay(10);
    Wire.beginTransmission(I2CADDR_B);
    Wire.write(attinySlaveArrayBoard, 3); 
    if (Wire.endTransmission () == 0) { 
        Serial.println("i2c Write to 0x12 Sucessfull");
        return 0;
    }
    else {
        Serial.println(Wire.endTransmission());
        Serial.println("i2c Write Failed");
        return 1;
    }
}
int turnDegree(int motor, int degree) {
    attinySlaveArrayBoard[0] = motor == 0 ? 0x16 : 0x26;
    attinySlaveArrayBoard[1] = (degree & 0xff);
    attinySlaveArrayBoard[2] = (degree >> 8) & 0xff;   
    delay(10);
    Wire.beginTransmission(I2CADDR_B);
    Wire.write(attinySlaveArrayBoard, 3); 
    if (Wire.endTransmission () == 0) { 
        Serial.println("i2c Write to 0x12 Sucessfull");
        return 0;
    }
    else {
        Serial.println(Wire.endTransmission());
        Serial.println("i2c Write Failed");
        return 1;
    }
}
int checkMotorStatus(int motor){
  Wire.requestFrom(I2CADDR_B, 1);
  attinySlaveArrayBoard[0] = Wire.read();
  Wire.endTransmission();
  return (motor == 1) ? (attinySlaveArrayBoard[0] & 0x01) : ((attinySlaveArrayBoard[0] >> 1) & 0x01);
}





void readSensorValues()
{    expanderData = IOexpanderRead(PEXP_I2CADDR);
    sensor[0] = bitRead(expanderData, 0);
    sensor[1] = bitRead(expanderData, 1);
    sensor[2] = bitRead(expanderData, 2);
    sensor[3] = bitRead(expanderData, 3);
    sensor[4] = bitRead(expanderData, 4);
    for (byte i = 0; i < 9; i++) {     
        if (sensor[0] == pidErrorMap[i][0] && sensor[1] == pidErrorMap[i][1] && 
            sensor[2] == pidErrorMap[i][2] && sensor[3] == pidErrorMap[i][3] && 
            sensor[4] == pidErrorMap[i][4]) {
            error = pidErrorMap[i][5];
        }
        if (sensor[0] + sensor[1] + sensor[2] + sensor[3] + sensor[4] == 5) {
        } 
        else if (sensor[0] + sensor[1] + sensor[2] + sensor[3] + sensor[4] == 0) {
        } 
        else {            
        }
    }
}
void calculatePID(){
    P = error;
    I = I + error;
    D = error - previousError;
    PID_value = (Kp*P) + (Ki*I) + (Kd*D);
    previousError = error;
}
void setMotorSpeed(int left, int right){                          
    Serial.print("Left = "); Serial.print(String(left));
    Serial.print(" Right = ");Serial.println(String(right));
    delay(100);    
    setRPM(0, left);
    setRPM(1, right);  
}

void motorControl(){
    float leftMotorSpeed = maxSpeed - PID_value;
    float rightMotorSpeed = maxSpeed + PID_value;
    leftMotorSpeed = constrain(leftMotorSpeed, minSpeed, maxSpeed);
    rightMotorSpeed = constrain(rightMotorSpeed, minSpeed, maxSpeed);    
    setMotorSpeed(leftMotorSpeed, rightMotorSpeed);  
}



void color_cycle(int r, int g, int b, int minx, int whichled){
    if (r == 255 & g == minx & b < 255) {
      (whichled == 0) ? b1++ : b2 ++;
    }
    else if (r > minx & g == minx & b == 255){
      (whichled == 0) ? r1 -= 1 : r2 -= 1;
    }
    else if (r == minx & g != 255 & b == 255){
      (whichled == 0) ? g1++ : g2++;
    }
    else if (r == minx & g == 255 & b > minx){
      (whichled == 0) ? b1 -= 1 : b2 -=1;
    }
    else if (r < 255 & g == 255 & b == minx){
      (whichled == 0) ? r1++ : r2++;
    }
    else if (r == 255 & g > minx & b == minx){
      (whichled == 0) ? g1 -= 1 : g2 -= 1;
    }
    strip.setPixelColor((whichled == 0) ? 0 : 1, strip.Color(r, g, b));
    strip.show();
    delay(2);
    Serial.println("in color cycle");
}



void irISR() {
    timer_value = micros() - timer_value_old;  
    Serial.println("in irISR");    
    switch(nec_state){
        case 0:                                       
            if (timer_value > 67500) {         
                timer_value_old = micros();     
                nec_state = 1;                  
                i = 0;
            }
        break;
        case 1:
            if (timer_value >= (13500 - 1000) && timer_value <= (13500 + 1000)) {
                i = 0;
                timer_value_old = micros();
                nec_state = 2;
            }
            else 
                nec_state = 0;
        break;
        case 2: 
            if (timer_value < (1125 - 562) || timer_value > (2250 + 562)) nec_state = 0; 
            else { // it's M0 or M1
                  nec_code = nec_code << 1; 
                  if(timer_value >= (2250 - 562)) nec_code |= 0x01; 
                  i++;
                  if (i==32) { 
                      nec_ok = true;
                      nec_state = 0;
                      timer_value_old = micros();
                  }
                  else {
                      nec_state = 2; 
                      timer_value_old = micros();
                  }
              }
        break;
        default:
            nec_state = 0;
        break;
    }
}




void setup(){
  Wire.begin(I2C_SDA, I2C_SCL);   
  strip.begin();           
  strip.show();            
  strip.setBrightness(15); 
  oled.begin(&Adafruit128x64, OLED_I2CAADR); 
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  delay(2000);
  Serial.begin(115200);
  oled.setFont(Adafruit5x7);
  oled.clear();
  oled.println("Oak");
  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
  WiFiMulti.addAP(ssid, password);
  while(WiFiMulti.run() != WL_CONNECTED) {
    Serial.println("in loop, cannot connect");
      delay(100);
  }
  Serial.println(WiFi.localIP());
  oled.println(WiFi.localIP());
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  server.on("/", []() {
      server.send(200, "text/html", index_html);
  });
  server.onNotFound(handleNotFound);
  server.begin();
  attinySlaveArrayBoard[0] = 2;
  attinySlaveArrayBoard[1] = 1;
  attinySlaveArrayBoard[2] = 1;
  setRPM(1, 15);
  setRPM(0, 15);
  attachInterrupt(digitalPinToInterrupt(IR_RECV), irISR, FALLING);
  IOexpanderWrite(PEXP_I2CADDR, 0xff);
}



void loop() {
  webSocket.loop();
  server.handleClient();
  digitalWrite(LED, ledState);
  delay(2); 
  
  if (nec_ok){
    nec_ok = false; 
    Serial.println("in if necok");
    address = nec_code >> 16;
    command = (nec_code & 0xFFFF) >> 8; 

    if (command == 0x18){
      robot_direction = 1;
    }
    else if (command == 0x4A){
      robot_direction = 2;
    }
    else if (command == 0x5A){
      Serial.println("here");
      robot_direction = 4;
    }
    else if (command == 0x10){
      robot_direction = 3;
    }
    else if (command == 0x38){
      robot_direction = 0;
    }
    else if (command == 0xA2){
      mainled1_command = 1;
    }
    else if (command == 0x22){
      mainled1_command = 2;
    }
    else if (command == 0xE0){
      mainled1_command = 3;
    }
    else if (command == 0x62){
      mainled2_command = 1;
    }
    else if (command == 0x02){
      mainled2_command = 2;
    }
    else if (command == 0xA8){
      mainled2_command = 3;
    }
    else if (command == 0x68){
      setinten(3);
    }
    else if (command == 0xB0){
      setinten(-3);
    }
    else if (command == 0x98){
      setMotorRunning(0);
      (pid_command == 0) ? pid_command = 1 : pid_command = 0;
    }
  }
  if (robot_direction == 1){
    Serial.println("Moving forward");
    setDirection(0, CCW);
    setDirection(1, CW);
    setMotorRunning(HIGH);
    delay(10);
  }
  else if (robot_direction == 2){
    Serial.println("Moving backward");
    setDirection(0, CW);
    setDirection(1, CCW);
    setMotorRunning(HIGH);
    delay(10);
  }
  else if (robot_direction == 3){
    Serial.println("Turning left");
    setDirection(0, CCW);
    setDirection(1, CCW);
    setMotorRunning(HIGH);
    delay(10);
  }
  else if (robot_direction == 390){
    Serial.println("Turning left 90??");
    setDirection(0, CCW);
    setDirection(1, CCW);
    turnDegree(0, 1670);
    turnDegree(1, 1670);
    while (checkMotorStatus(0) == 1){
      delay(5);
    }
    robot_direction = 0;
    delay(10);
  }
  else if (robot_direction == 4){
    Serial.println("Turning right");
    setDirection(0, CW);
    setDirection(1, CW);
    setMotorRunning(HIGH);
    delay(10);
  }
  else if (robot_direction == 490){
    Serial.println("Turning right 90??");
    setDirection(0, CW);
    setDirection(1, CW);
    turnDegree(0, 1670);
    turnDegree(1, 1670);
    while (checkMotorStatus(0) == 1){
      delay(10);
    }
    robot_direction = 0;
    delay(100);
  }
  else if (robot_direction == 0){
    Serial.println("Stop");
    setMotorRunning(LOW);
    delay(50);
  }
  if (mainled1_command == 1){
    strip.setPixelColor(0, strip.Color((mainled1_state == 0) ? 250 : 0, 0, 0));
    strip.show();
    delay(5);
    (mainled1_state == 0) ? mainled1_state = 1 : mainled1_state = 0;
    mainled1_command = 0;
  }
  else if (mainled1_command == 2 & mainled1_state == 1){
    strip.setPixelColor(0, strip.Color(random(0, 255), random(0, 255), random(0, 255)));
    strip.show();
    mainled1_command = 0;
  }
  else if (mainled1_command == 3 & mainled1_state == 1){
    color_cycle(r1, g1, b1, min1, 0);
  }
  if (mainled2_command == 1){
    strip.setPixelColor(1, strip.Color((mainled2_state == 0) ? 250 : 0, 0, 0));
    strip.show();
    delay(5);
    (mainled2_state == 0) ? mainled2_state = 1 : mainled2_state = 0;
    mainled2_command = 0;
  }
  else if (mainled2_command == 2 & mainled2_state == 1){
    strip.setPixelColor(1, strip.Color(random(0, 255), random(0, 255), random(0, 255)));
    strip.show();
    mainled2_command = 0;
  }
  else if (mainled2_command == 3 & mainled2_state == 1){
    color_cycle(r2, g2, b2, min2, 1);
  }
  if (pid_command == 0){
    setMotorRunning(1);
    readSensorValues();
    calculatePID();
    motorControl();
  }
}


                    

                
