// MQTT Server use: broker.mqtt-dashboard.com

#include "DHTesp.h"
#include "PubSubClient.h"
#include <WiFi.h>
#include <cstring>

DHTesp dhtSensor;

const char* ssid = "Wokwi-GUEST";
const char* password = "";

String ledOrder = "true";
String buzzerOrder = "true";

// Khai báo các pin cần thiết
const int DHT_PIN = 15;
int echo_pin = 12;
int trig_pin = 13;
int motion_pin = 2;
int buzzer_pin = 14;
int led_pin = 27;

// Hàm lấy khoảng cách cho UltraSonic
long getDistance(){
  digitalWrite(trig_pin,LOW);
  delayMicroseconds(2);
  digitalWrite(trig_pin,HIGH);
  delayMicroseconds(10);
  digitalWrite(trig_pin,LOW);
  
  long duration = pulseIn(echo_pin,HIGH);
  long distanceCm = duration*0.034/2;
  return distanceCm;
  
}

//-----------------------------------------------
//Các hàm thiết lập kết nối
const char* mqttServer = "broker.hivemq.com"; 
int port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

void wifiConnect() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");
}

void mqttReconnect() {
  while(!client.connected()) {
    Serial.println("Attemping MQTT connection...");
    //***Change "123456789" by your student id***
    if(client.connect("20127404_20127442_20127204")) {
      Serial.println("connected");

      //Subcribe button
      client.subscribe("20127404_20127442_20127204/led_out");
      client.subscribe("20127404_20127442_20127204/buzzer_out");
    }
    else {
      Serial.println("try again in 5 seconds");
      delay(5000);
    }
  }
}
//-----------------------------------------------

void setup() {
  Serial.begin(115200);
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  pinMode(trig_pin, OUTPUT);
  pinMode(echo_pin, INPUT);
  pinMode(motion_pin, INPUT);
  pinMode(buzzer_pin, OUTPUT);
  pinMode(led_pin, OUTPUT);

  //Thiết lập các kết nối cần thiết
  wifiConnect();
  client.setServer(mqttServer, port);
  client.setCallback(callback);
}

//MQTT Receiver
void callback(char* topic, byte* message, unsigned int length) {
  Serial.println(topic);
  String strMsg;
  for(int i=0; i<length; i++) {
    strMsg += (char)message[i];
  }
  Serial.println(strMsg);
  String topic_str = String(topic);
  
  if(topic_str == "20127404_20127442_20127204/led_out"){
     ledOrder = strMsg;
  }
  if(topic_str == "20127404_20127442_20127204/buzzer_out"){
     buzzerOrder = strMsg;
  }
}

// Hàm xây dựng JSON để gửi lên server
String jsonBuild(String temperature, String humidity, long distance, String isMotion)
{
  String temp = "";
  temp = "{ \"Temperature\": "+ temperature + ", \"Humidity\": " + humidity + ", \"Distance\": "+ String(distance) +", \"Motion\": " + isMotion + "}";
  return temp;
}



void loop() {
  if(!client.connected()) {
    mqttReconnect();
  }
  client.loop();

  char temp[20];
  char humid[20];
  char dist[20];
  char motio[20];

  // Lấy thông số từ DHT22
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  String temperature_str = String(data.temperature, 2);
  String humidity_str = String(data.humidity, 1);

  // Lấy thông số từ UltraSonic
  long distance = getDistance();
  sprintf(dist,"%u",distance);
  

  // Lấy thông số từ cảm biến chuyển động
  String isMotion ="0";
  if(digitalRead(motion_pin) == HIGH){
    //Nếu phát hiện vật thể chuyển động trong khoảng cách nhỏ hơn 50cm 
    //thì gắn giá trị cho biến cờ isMotion
      if(distance < 50){
      client.setCallback(callback); //Nhận thông tin về lệnh tắt/bật của người dùng
      if(buzzerOrder == "true")
      {
        tone(buzzer_pin, 1, 1000);
      }
      
    }
      isMotion= "1"; //Nếu phát hiện chuyển động thì gắn biến cờ bằng 1 
    
  }
  else{
    isMotion="0";
  }
  
  // Khi phát hiện cháy buzzer sẽ phát ra tiếng kêu kèm với đèn led sẽ được bật để cảnh báo
  float temperature_flt = temperature_str.toFloat();
  float humidity_flt = humidity_str.toFloat();
  
  if (temperature_flt > 40 && humidity_flt > 98)
  {
     client.setCallback(callback);
     if(ledOrder == "false")
     {
       digitalWrite(led_pin,LOW);
     }
     else
     {
       digitalWrite(led_pin, HIGH);
     }

     if(buzzerOrder == "true")
     {
      
       tone(buzzer_pin, 3, 1000);
     }
  }
  else{
    digitalWrite(led_pin, LOW);
  }


  // Chuyển đổi các thông số qua dạng char để tiến hành publish lên MQTT
  isMotion.toCharArray(motio,20);
  temperature_str.toCharArray(temp,20);
  humidity_str.toCharArray(humid,20);

  // Publish các thông số lên MQTT
  String temp_str = "";
  char buffer[100];
  temp_str = jsonBuild(temperature_str, humidity_str, distance, isMotion);
  temp_str.toCharArray(buffer,100);
  Serial.println(temp_str);
  client.publish("20127404_20127442_20127204/input", buffer);
  

  delay(1500); 
}
