#include <LiquidCrystal.h>
#include <DHT.h>
#include"PubSubClient.h"
#include <WiFi.h>


// const char* ssid = "Wokwi-Hen-Farming";
// const char* password = "12345678";


// void wifiConnect() {
//   WiFi.begin(ssid, password);
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.println("Waiting for connecting");
//   }
//   Serial.println("CONNECTED");
// }


// The lightbulb will shine if:
//  - The device is turned on.
//  - It is in night mode.
// When the night mode occurs
//  - the lightbulb will immediately shine, running the timer.

// - - - - - - TUTORIAL - - - - - -
// 1) Run the simulator
// 2) Press the button to open the device
// 3) Set the photoresistor (> 10000 lux: day)
//
//     => Light mode
//          => Turn OFF 'light'
//          => Stop     'timer'
//
//     => Just begin night mode
//          => Turn ON 'light'
//          => Set     'starting_hour'
//          => Run     'timer'
//
//     => Night mode and 'timer' does not reach 'time duration'
//          => Turn ON 'light'
//          => Run     'timer'
//
//     => Night mode but 'timer' reaches 'time duration'
//          => Turn OFF 'light'
//          => Stop and Reset 'timer'
//          => Wait till next period
//
//     => Light mode but 'timer' does not finish
//          => Turn OFF 'light'
//          => STOP     'timer'
//
//     => New period: 12pm (12:00)
//          => Reset    'starting hour'
//          => Update   'day' and 'week'
//        


// List all pins
int LIB_pin = 23;
int ldr_pin = 14;
int pir_pin = 12;
int btn_pin = 15;
DHT dht(13, DHT22);
LiquidCrystal lcd(21, 19, 18, 5, 4, 2);

//mqtt variable
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqttSerVer = "test.mosquitto.org";
int port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

// Suppose an hour in real life is a second in this program
// Light duration to turn on and off automatically
unsigned int time_duration = 0;
unsigned int timer = 0;
bool running_timer = false;

// Traverse [0, 23].
unsigned int start_hour = 24;    // Starting hour to light the lightbulb. Reset when curr_hour = 0
unsigned int prev_hour = 0;
unsigned int curr_hour = 0;

// Notice if a second passes.
unsigned int t = 0;

// Chicken weeks old
unsigned int weeks_old = 0;
unsigned int curr_week_day = 0;  // 7 days [0, 7]

// Help turn on and off the device
bool prev_state = false;
bool device_turning = false;

// Help the LCD to run begin() once when running
bool start_lcd = true;

// - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - Wifi, callback and MQTT function- - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - -

void WifiConnect(){
  WiFi.begin(ssid,password);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.println(".");
  }
  Serial.println("Connected");
}

void mqttReconnect(){
  while(!client.connected()){
    Serial.println("Attemping MQT connection...");
    if(client.connect("chicken_egg_system")){
      Serial.println("Connected");
      client.subscribe("chicken_egg_system/light");
    }
    else{
      Serial.println("try again in 5 seconds");
      delay(5000);
    }
  }

}

void callback(char* topic, byte* message, unsigned int length){
  Serial.println(topic);
  String stMessage;

  for(int i = 0; i < length; i++){
    stMessage += (char)message[i];
  }

  String stTopic = topic;
  if(stTopic == "chicken_egg_system/light"){
    if(stMessage == "true"){
      digitalWrite(LIB_pin, HIGH);
    }  // bat den
    else{
      digitalWrite(LIB_pin, LOW);
    }
    Serial.println(stMessage);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - 
// - - - - - - - - - - - - - - - - - - - - - - 
// - - - - - - - - - - - - - - - - - - - - - - 

// - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - Two main functions  - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - 

void setup() {
  
  
  pinMode(ldr_pin, INPUT);
  pinMode(pir_pin, INPUT);
  pinMode(btn_pin, INPUT);
  pinMode(LIB_pin, OUTPUT);
  
  dht.begin();
  lcd.begin(16, 2);
  Serial.begin(9600);
  
  //constructor(13, 19, 19);

  WifiConnect();
  client.setServer(mqttSerVer,port);
  client.setCallback(callback);
//  wifiConnect();
//  Serial.println(String(WiFi.localIP()) + "Has connected");
  
}



void loop() {
  if(!client.connected()){
    mqttReconnect();
  }
  client.loop();
  toggleButtonOnOff();    // Pressing buttons
  //setTraversingHour();    // Run the pseudo-hour

  if (device_turning) {

    if (start_lcd) {
      lcd.begin(16, 2);
      start_lcd = false;
    }

    // digitalWrite(LIB_pin,
    //   (isNight(analogRead(ldr_pin)) and isInTimer()) ? HIGH : LOW
    // );

    printLCD(dht.readTemperature(), dht.readHumidity());
  }

  else {
    lcd.noDisplay();
    //digitalWrite(LIB_pin, LOW);
    start_lcd = true;
  }

  //send temp and humi to mqtt
  char buffer[50] = {0};
  int temp = dht.readTemperature();
  int humi = dht.readHumidity();
  sprintf(buffer, "{\"temp\":%d, \"humi\": %d}", temp,humi);
  client.publish("chicken_egg_system/temp_&_humi",buffer);

  //send day status
  char day_status[50];
  sprintf(day_status, "%d",digitalRead(ldr_pin));
  client.publish("chicken_egg_system/day_status",day_status);

  //send pir status
  char pir_status[50];
  sprintf(pir_status, "%d",digitalRead(pir_pin));
  client.publish("chicken_egg_system/pir_status",pir_status);
  delay(50);

  //printStatus();
}

// - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - 


// Declares some values
void constructor(int _time_duration, int _weeks_old, int _curr_hour) {
  time_duration = _time_duration;
  weeks_old = _weeks_old;
  curr_hour = _curr_hour;
}


// Print the LCD
void printLCD(float tempe, float humid) {
  lcd.setCursor(0, 0);  lcd.print("Tempe: "); lcd.print(tempe); lcd.print('   ');
  lcd.setCursor(0, 1);  lcd.print("Humid: "); lcd.print(humid); lcd.print('   ');
}


// If the red button is pressed
void toggleButtonOnOff() {
  if (digitalRead(btn_pin) == HIGH) {
    if (prev_state == device_turning)
      device_turning = not device_turning;
  }
  else {
    prev_state = device_turning;
  }
}


// If a second passes
// void setTraversingHour() {
//   if (millis() - t >= 1000) {
//     t = millis();

//     // update curr_hour
//     curr_hour = addHour(curr_hour);

//     // and reset start_hour, if the current hour is 12
//     if (curr_hour == 12) {
//       curr_week_day = addDay(curr_week_day);
//       start_hour = 24;
//     }
//   }
// }


// Check if it is night
// bool isNight(float num) {
//   if (num > 156.0) {

//     // If it is night and the start_hour is not set yet
//     // Update it
//     if (start_hour == 24) {
//       start_hour = curr_hour;
//       if (not running_timer) {
//         timer = 0;
//         running_timer = true;
//       }
//     }
//     return true;
//   }
//   return false;
// }



// bool isInTimer() {
//   // If the timer is running, return true
//   if (running_timer) {
//     // Decrease the timer by one second
//     if (prev_hour != curr_hour) {
//       prev_hour = curr_hour;
//       timer = addHour(timer);
//       // If the timer finishes, stop lighting
//       if (timer == time_duration) {
//         timer = 0;
//         running_timer = false;
//       }
//     }
//     return true;
//   }

//   // Else, return false
//   return false;
// }



// unsigned int addHour(unsigned int value) {
//   return (++value == 24) ? 0 : value;
// }

// unsigned int addDay(unsigned int value) {
//   if (++value == 7) {

//     weeks_old++;
//     if (weeks_old < 19) {
//       time_duration = 0;
//     }
//     else if (weeks_old == 19) {
//       time_duration = 13;
//     }
//     else if (weeks_old == 20) {
//       time_duration = 14;
//     }
//     else if (weeks_old == 21) {
//       time_duration = 15;
//     }
//     else {
//       time_duration = 16;
//     }

//     return 0;
//   }
//   return value;
// }


//                   ,.
//                  (\(\)
//  ,_              ;  o >
//   {`-.          /  (_) 
//   `={\`-._____/`   |
//    `-{ /    -=`\   |
//  .="`={  -= = _/   /`"-.
// (M==M=M==M=M==M==M==M==M)
//  \=N=N==N=N==N=N==N=NN=/
//   \M==M=M==M=M==M===M=/
//    \N=N==N=N==N=NN=N=/
// jgs \M==M==M=M==M==M/
//      `-------------'


void printStatus() {
  static int q = 1000 / 50 - 2;
    if (--q == 0) {
    //Serial.print("Day: " + String(curr_week_day) + "  ");
    Serial.print("Night: " + String(digitalRead(ldr_pin)) + "  ");
    //Serial.print("Hour: " + String(curr_hour) + "\t");
    //Serial.print("Run: " + String(running_timer) + "\t");
    //Serial.print("Start: " + String(start_hour) + "\t");
    //Serial.println("Timer: " + String(timer) + "/" + String(time_duration));
    q = 1000 / 50 - 2;
  }
}
