#include <Wire.h>
#include "rgb_lcd.h"
rgb_lcd lcd;
#include "DHT.h"
#include "EspMQTTClient.h"
#define DHTPIN D4
#define DHTTYPE DHT11 
#define RELAY_PIN D6
#define COI_PIN D3  // Chân D3
#define BUTTON D2
#define FLAME_SENSOR_PIN D1     // Chân tín hiệu digital của cảm biến lửa
#define GAS_SENSOR_PIN   D0     // Chân tín hiệu digital của cảm biến khí
#define INFLUXDB_URL "http://192.168.1.105:8066"
#define INFLUXDB_TOKEN "Jfkzbvj3pUQzYp5bZ8I38yfBWaabmwubpD2Qp8Dy225AVosk-gNAaKl6HVpZGH2LbECThI80xUbUS0RoqOy0eA=="
#define INFLUXDB_ORG "WGLabz"
#define INFLUXDB_BUCKET "Test"
#define INFLUXDB_MEASUREMENT "esp8266"
bool ledState = 0;     // Trạng thái đèn
bool buttonPressed = 0;
DHT dht(DHTPIN, DHTTYPE);
EspMQTTClient client(
  "Hansa",
  "12345678", 
  "192.168.43.89",  // MQTT Broker server ip
  "MQTTUsername",   // Can be omitted if not needed
  "MQTTPassword",   // Can be omitted if not needed
  "TestClient",     // Client name that uniquely identify your device
  1883              // The MQTT port, default to 1883. this line can be omitted
);

//const int relayPin = 0;  // GPIO0
void setup()
{
 Serial.begin(9600);
  dht.begin();
  //setup RELAY
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  lcd.setRGB(255, 255, 255);
  delay(1000);
  
  pinMode(COI_PIN, OUTPUT);  // Đặt D3 là OUTPUT
  digitalWrite(COI_PIN, HIGH);
  pinMode(BUTTON, INPUT_PULLUP);  // Nút nhấn kéo xuống GND
  Serial.begin(9600);
  pinMode(FLAME_SENSOR_PIN, INPUT);
  pinMode(GAS_SENSOR_PIN, INPUT);

  client.enableDebuggingMessages();
  client.enableHTTPWebUpdater();
  client.enableOTA();
  client.enableLastWillMessage("TestClient/lastwill", "I am going offline");

  // Optional functionalities of EspMQTTClient
  client.enableDebuggingMessages(); // Enable debugging messages sent to serial output
  client.enableHTTPWebUpdater(); // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overridded with enableHTTPWebUpdater("user", "password").
  client.enableOTA(); // Enable OTA (Over The Air) updates. Password defaults to MQTTPassword. Port is the default OTA port. Can be overridden with enableOTA("password", port).
  client.enableLastWillMessage("TestClient/lastwill", "I am going offline");  // You can activate the retain flag by setting the third parameter to true

}
void coi(int n)
{
  digitalWrite(COI_PIN, HIGH); // Bật còi
  delay(n);                  // Đợi 500ms
  digitalWrite(COI_PIN, LOW);  // Tắt còi
  delay(n);                  // Đợi 500ms
}
void onConnectionEstablished()
{
  Serial.println("ESP đã kết nối tới MQTT broker thành công!"); 
  // Subscribe to "mytopic/test" and display received message to Serial
  client.subscribe("mytopic/test", [](const String & payload) {
    Serial.println(payload);
  });
  client.subscribe("mytopic/123test", [](const String & payload) {
    Serial.println(payload);
  });
      client.subscribe("esp8266/relay", [](const String &payload) {
    if (payload == "on") {
      ledState = !ledState;            // Đổi trạng thái
      digitalWrite(RELAY_PIN, ledState); // Bật/tắt RELAY
      //digitalWrite(RELAY_PIN, HIGH);
      Serial.println("ĐỔI");
    }
      /*
     else if (payload == "off") {
      digitalWrite(RELAY_PIN, LOW);
      Serial.println("OFF");
    }
    */
  }); 
}
void loop()
{ 
client.loop();
 
if (client.isConnected()) {
  float h = dht.readHumidity(); // Đọc độ ẩm 
  float t = dht.readTemperature(); // Đọc nhiệt độ
  int flame = digitalRead(FLAME_SENSOR_PIN);  // Đọc cảm biến lửa
  int gas   = digitalRead(GAS_SENSOR_PIN);    // Đọc cảm biến khí gas
  digitalWrite(GAS_SENSOR_PIN,0);
  digitalWrite(FLAME_SENSOR_PIN,1);
  
  if (digitalRead(BUTTON) == LOW && !buttonPressed) {
    buttonPressed = 1;            // Đánh dấu đã nhấn
    ledState = !ledState;            // Đổi trạng thái
    delay(100); // đợi tránh dội nút
  }

  // Nếu đã nhấn và thả tay ra
  if (digitalRead(BUTTON) == HIGH) {
    buttonPressed = 0; // Sẵn sàng nhận lần nhấn tiếp theo
  }

  if (isnan(h) || isnan(t)) {
    Serial.println("Loi doc DHT11");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("DHT loi!");
    delay(200);
    return;
  }
  //Cảm biến khí Gas & Cháy
    if ((ledState==0)&&(flame==0))
  {
    coi(250);
    Serial.print("Flame: ");
    Serial.println("Cháy rồi!");
  } 
  else if ((ledState==0)&&(gas==1))
  {
    coi(700);
    Serial.print("Gas: ");
    Serial.println("Khí Gas vượt ngưỡng");    
  }
  else {
    digitalWrite(COI_PIN, LOW);   // Còi tắt ngay lập tức   
  }
  // Tạo chuỗi JSON hợp lệ
  String jsonData = "{\"temp\":" + String(t, 1) + ",\"humi\":" + String(h, 1) + "}";
  Serial.println("Gửi JSON: " + jsonData);
  //HIEN THI NHIET DO, ĐỘ ẨM LÊN LCD
    lcd.clear();
    lcd.print("temp:      ");
    lcd.setCursor(6, 0);
    lcd.print(h,1);
    lcd.setCursor(0, 1);
    lcd.print("humi:       ");
    lcd.setCursor(6, 1);
    lcd.print(t,1);
    delay(100);
  // Gửi lên MQTT
  client.publish("mytopic/sensor", jsonData.c_str());

} 
else {
  Serial.println("MQTT chưa kết nối. Đợi tiếp...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MQTT connecting...");
  delay(100);
}

delay(200);
}
