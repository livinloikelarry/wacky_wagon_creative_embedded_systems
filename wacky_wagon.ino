/**********************************************************************
  Filename    : Control_Motor_by_L293D
  Description : Use PWM to control the direction and speed of the motor.
  Auther      : www.freenove.com
  Modification: 2020/07/11  
**********************************************************************/
// distance sensor setup 
#define trigPin 2 // define TrigPin
#define echoPin 32 // define EchoPin.
#define MAX_SENSOR_DISTANCE 700 // Maximum sensor distance is rated at 400-500cm.
float timeOut = MAX_SENSOR_DISTANCE * 60; 
int soundVelocity = 340; // define sound speed=340m/s
int dist = 0;

// variables for web server 
int winner = 0;
char color = 'N';
int freePlay = 0;
int q_count = 0;

// wifi set up 
#include <WiFi.h>
#include <WebServer.h>

/* Put SSID & Password */
const char* ssid = "ESP32";  // Enter SSID here
const char* password = "12345678";  //Enter Password here

/* Put IP Address details */
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

WebServer server(80);

bool LED1status = LOW;
bool LED2status = LOW;


// Motor A connections
int in1 = 26;      // Define L293D channel 1 pin
int in2 = 33;      // Define L293D channel 2 pin
int enA = 25;  // Define L293D enable 1 pin
int channel = 0; // we chose pwm channel 0 
// Motor B connections
int enB = 13;
int in3 = 12;
int in4 = 15;

boolean rotationDir;  // Define a variable to save the motor's rotation direction
int rotationSpeed;    // Define a variable to save the motor rotation speed
unsigned long move_interval = 250;
unsigned long previous_time = 0;
int motor_speed = 2000;
int car_action = 0; // 1 forward 2 is backwards 
int isMoving = 0;

void robot_stop();
void robot_fwd();
void robot_back();
void robot_left();
void robot_right();
void checkDistance();


/* parallel core variables 
 *  
 */
static int taskCore = 0;  // the core that we want our other motor's code to run on 

/* this function is run by core 0 
 *  inside of the function I call checkDistance() which contains an infinite loop 
 */
void coreTask( void * pvParameters ){
      checkDistance();
}


void checkDistance(){
  while(true){
    dist = getSonar();
    if( dist < 30 && dist > 0){
      handle_crash();
      Serial.println("waiting for the turns");
      delay(5000);
      Serial.printf("Distance: ");
      Serial.print(getSonar()); // Send ping, get distance in cm and print result 
      Serial.println("cm");
    }
    delay(500);
  }
}

void setup() {
  Serial.begin(115200);
//  sensor set up
  pinMode(trigPin,OUTPUT);// set trigPin to output mode
  pinMode(echoPin,INPUT); // set echoPin to input mode
  
//  wifi set up 
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);

/* running checkDistance on a different core 
 *  
 */
  Serial.print("Starting to create task on core ");
  Serial.println(taskCore);
  xTaskCreatePinnedToCore(
                    coreTask,   /* Function to implement the task */
                    "coreTask", /* Name of the task */
                    10000,      /* Stack size in words */
                    NULL,       /* Task input parameter */
                    0,          /* Priority of the task */
                    NULL,       /* Task handle. */
                    taskCore);  /* Core where the task should run */
 
  Serial.println("Task created...");

  
  
  server.on("/", handle_OnConnect);
  server.on("/free", handle_free);
  server.on("/forward", handle_forward);
  server.on("/backward", handle_backward);
  server.on("/left", handle_left);
  server.on("/right", handle_right);
  server.on("/f_right", handle_right);
  server.on("/f_left", handle_left);
  server.on("/f_forward", handle_forward);
  server.on("/f_backward", handle_backward);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");

 previous_time = millis();  //initial start time

  //  motors 
  // Initialize the pin into an output mode:
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  pinMode(enA, OUTPUT);
  pinMode(enB, OUTPUT);

  // Turn off motors - Initial state
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
//  channel, frequency, resolution because 2^11 is 2048 
  ledcSetup(channel,1000,11);         //Set PWM to 11 bits, 
  // 2^11 is 2048 range is 0-2047 which means this is range of our speed 
  ledcAttachPin(enA,channel); // gpio that will output the signal, channel that will generate the signal 
  ledcAttachPin(enB,channel); 
}

void loop() {
  server.handleClient();
  Serial.println("handle client");
  // get read of sensor data but not as often as we are running the loop 
  
  if(isMoving){
    rotationSpeed = abs(2000);
    ledcWrite(0, rotationSpeed);
    unsigned long currentMillis = millis(); // number of milliseconds since the program started
    if (currentMillis - previous_time >= move_interval) { // time to stop 
      previous_time = currentMillis;
      robot_stop();
      Serial.println("Stop");
      isMoving = 0;
    }
    if(car_action == 1){
      robot_fwd();
    }else if(car_action ==2){
      robot_back();
    }else if(car_action == 3){
      robot_left();
    }else if(car_action ==4){
      robot_right();
    }else if(car_action ==5){
      robot_back();
      delay(2000);
      robot_left();
    }
  delay(500);
  yield();
 }else{
  robot_stop();
 }



  delay(500);
}

// function for sensor 
float getSonar() {
  unsigned long pingTime;
  float distance;
  // make trigPin output high level lasting for 10μs to triger HC_SR04
  digitalWrite(trigPin, HIGH); 
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Wait HC-SR04 returning to the high level and measure out this waitting time
  pingTime = pulseIn(echoPin, HIGH, timeOut); 
  // calculate the distance according to the time
  distance = (float)pingTime * soundVelocity / 2 / 10000; 
  return distance; // return the distance value
}

// functions that help set up web server 
void handle_OnConnect() {
  Serial.println("connected");
  winner= 0;
  freePlay=0;
  q_count=0;
  color= 'N';
  server.send(200, "text/html", SendHTML(winner, freePlay, q_count, color)); 
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}


String SendHTML(int winner, int freePlay, int q_count, char color){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>Wacky Wagon</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;";
  if(color == 'G' && freePlay == 0){ // screen should be green 
    ptr += "background-color: rgb(79, 220, 79);\n";
  }else if(color == 'R' && freePlay == 0){
    ptr += "background-color: rgb(215, 56, 56);\n";
  }else if(color == 'N'){ // regular white page
    ptr += "background-color: white;\n";   
  }
  ptr += "} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr += ".wrong {background-color: rgb(215, 56, 56);}\n";
  ptr += ".right {background-color: rgb(79, 220, 79);}\n";
  ptr += "button { width: 100px; height: 50px; margin: 10px;}\n";
  ptr += ".back_button {background-color: yellowgreen;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr += "<script>\n";
  if(freePlay == 1){
    ptr+= "function forward() {window.location.assign('/f_forward');}\n";
    ptr += "function backward() { window.location.assign('/f_backward');}\n";
    ptr += "function left() { window.location.assign('/f_left');}\n";
    ptr += "function right() { window.location.assign('/f_right');}\n";
   }
  ptr += "let i = 0;\n let mult_table = [['3 x 1', '3'],['3 x 2', '6'],['3 x 3', '9'],['3 x 4', '12'],['3 x 5', '15'],['3 x 6', '18'],['3 x 7', '21'],['4 x 1', '4'],['4 x 2', '8'],['4 x 3', '12'],['4 x 4', '16'],['4 x 5', '20'],['4 x 6', '24'],['4 x 7', '28'],['5 x 1', '5'],['5 x 2', '10'],['5 x 3', '15'],['5 x 4', '20'],['5 x 5', '25'],['5 x 6', '30'],['5 x 7', '35']];\n";
  ptr += "function new_mult_fact(){ i = Math.floor(Math.random() * mult_table.length);\n let math_fact = document.getElementById('math_fact');\n";
  ptr += "math_fact.innerHTML = ''; \n math_fact.append(mult_table[i][0]);}\n";
  ptr += "function check() { let user_resp = document.getElementById('user_answer').value;\n";
  ptr += "if (user_resp != mult_table[i][1]) {document.body.classList.add('wrong');\n";
  ptr += "window.location.assign('/backward');}\n else {document.body.classList.add('right');\n";
  ptr += "document.body.classList.add('right');\n";
  ptr += "document.getElementById('user_answer').value = '';\n window.location.assign('/forward');\n}}\n";
  ptr += "function restart() {\nwindow.location.assign('/');\n}";
  ptr += "function free_play() {\nwindow.location.assign('/free');\n}";
  ptr += " </script>\n";
  ptr +="</head>\n";
  if(freePlay == 1){
    ptr += "<body>\n<h1>Wacky Wagon</h1>\n<h3>Free play!</h3>\n";
    ptr += "<div class='button_area'>\n <div><button onclick='forward()'>Forward</button></div>\n";
    ptr += "<div><button onclick='left()'>Left</button>\n<button onclick='right()'>right</button>\n";
    ptr += "</div>\n<div><button onclick='backward()'>Backward</button></div>\n</div>\n";
    ptr += "<div><button class='back_button' onclick='reset()'>Back to Math</button></div>\n";
  }else{
    ptr +="<body onload='new_mult_fact()'>\n";
    ptr +="<h1>Wacky Wagon</h1>\n";
    ptr +="<h3 onclick='new_mult_fact()'>mutiplication game</h3>\n";
  //  if they have won and answered 5 or more q's the page should display that 
    if(winner == 1 && q_count >= 10){
      ptr += "<h2>Good Job!</h2>\n<h2>Good Job!</h2>\n<h2>Good Job!</h2>\n";
      ptr += "<button onclick='restart()'>play again</button>\n";
       ptr += "<button onclick='free_play()'>free play!</button>\n";
    }else{
      ptr += "<div>\n<h1 id='math_fact'></h1>\n<input type='text' id='user_answer' name='user_answer' />\n";
      ptr += "<div class='button_area'>\n";
      ptr += "<button onclick='check()'>Check</button>\n </div>\n</div>\n";
      }
  }
  ptr += "</body>\n</html>";

  return ptr;
}


void handle_free(){
  isMoving = 0;
  q_count = 0;
  color = 'N';
  freePlay = 1;
  Serial.println("free play");
  server.send(200, "text/html", SendHTML(winner, freePlay, q_count, color));
}
void handle_crash(){
  isMoving = 1;
  car_action = 5;
}
void handle_forward() {
  isMoving = 1;
  q_count += 1;
  color = 'G';
  Serial.println(q_count);
  if(q_count >= 10){
    winner = 1;
  }else{
    winner = 0;
  }
  car_action = 1;
  Serial.println("forward");
  server.send(200, "text/html", SendHTML(winner, freePlay, q_count, color));
}

void handle_backward() {
  isMoving = 1;
  car_action = 2;
  color = 'R';
  Serial.println("backward");
  server.send(200, "text/html", SendHTML(winner, freePlay, q_count, color));
}

void handle_left() {
  car_action = 3;
  isMoving = 1;
  Serial.println("left");
  freePlay = 1;
  winner = 0;
  color = 'N';
  server.send(200, "text/html", SendHTML(winner, freePlay, q_count, color));
}

void handle_right() {
  car_action = 4;
  isMoving = 1;
  winner = 0;
  Serial.println("right");
  freePlay = 1;
  color = 'N';
  server.send(200, "text/html", SendHTML(winner, freePlay, q_count, color));
}

// functions to move motors 
void robot_stop()
{
  isMoving = 0;
  digitalWrite(in1,LOW);
  digitalWrite(in2,LOW);
  digitalWrite(in3,LOW);
  digitalWrite(in4,LOW);

}
 
void robot_fwd()
{
  digitalWrite(in1,HIGH);
  digitalWrite(in2,LOW);
  digitalWrite(in3,HIGH);
  digitalWrite(in4,LOW);
  move_interval=250;
  previous_time = millis();  
}
 
void robot_back()
{
  digitalWrite(in1,LOW);
  digitalWrite(in2,HIGH);
  digitalWrite(in3,LOW);
  digitalWrite(in4,HIGH);
  move_interval=250;
   previous_time = millis();  
}
void robot_right()
{
  digitalWrite(in1,LOW);
  digitalWrite(in2,HIGH);
  digitalWrite(in3,HIGH);
  digitalWrite(in4,LOW);
  move_interval=100;
   previous_time = millis();
}
 
void robot_left()
{
  digitalWrite(in1,HIGH);
  digitalWrite(in2,LOW);
  digitalWrite(in3,LOW);
  digitalWrite(in4,HIGH);
  move_interval=100;
   previous_time = millis();
}
