#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ESP32Servo.h>

// Cấu hình WiFi và MQTT Broker
const char* ssid = "UDA.TEACHING";
const char* password = "uda.aun2025";

const char* mqtt_server = "f72254649f65498686bdf704456645b8.s1.eu.hivemq.cloud";
const int mqtt_port = 8883; // Sử dụng cổng bảo mật
const char* mqtt_username = "truong0907";
const char* mqtt_password = "Truong123";

// Khai báo kết nối và client MQTT
WiFiClientSecure espClient;
PubSubClient client(espClient);

// Cấu hình cảm biến DHT11
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Chân LED
const int ledPin1 = 26;
const int ledPin2 = 27;

// Cấu hình Servo
Servo servo; // Đối tượng servo

// Chân Quạt
const int fanPin = 33; // Chân GPIO 33 để điều khiển quạt

void setup_wifi() {
  delay(10);
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

void callback(char* topic, byte* message, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) {
    msg += (char)message[i];
  }

  // Xử lý tin nhắn nhận từ MQTT Broker
  if (String(topic) == "home/led1") {
    digitalWrite(ledPin1, msg == "on" ? HIGH : LOW);
    Serial.println("LED1: " + msg);
  } else if (String(topic) == "home/led2") {
    digitalWrite(ledPin2, msg == "on" ? HIGH : LOW);
    Serial.println("LED2: " + msg);
  } else if (String(topic) == "home/door") {
    if (msg == "open") {
      servo.write(90); // Mở cửa (servo quay 90 độ)
      Serial.println("Door opened via MQTT");
    } else if (msg == "close") {
      servo.write(0); // Đóng cửa
      Serial.println("Door closed via MQTT");
    }
  } else if (String(topic) == "home/fan") {
    if (msg == "on") {
      digitalWrite(fanPin, LOW); // Bật quạt
      Serial.println("Fan turned on");
    } else if (msg == "off") {
      digitalWrite(fanPin, HIGH); // Tắt quạt
      Serial.println("Fan turned off");
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");

    // Kết nối tới MQTT Broker với thông tin bảo mật
    if (client.connect("ESP32Client", mqtt_username, mqtt_password)) {
      Serial.println("Connected to MQTT Broker");

      // Đăng ký các topic để điều khiển LED, Servo và Quạt
      client.subscribe("home/led1");
      client.subscribe("home/led2");
      client.subscribe("home/door"); // Đăng ký thêm topic điều khiển cửa
      client.subscribe("home/fan");  // Đăng ký thêm topic điều khiển quạt
    } else {
      Serial.print("Failed to connect, rc=");
      Serial.print(client.state());
      Serial.println(" - Retrying in 5 seconds");
      delay(5000); // Thử lại sau 5 giây nếu kết nối thất bại
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Thiết lập chân LED và quạt
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(fanPin, OUTPUT); // Thiết lập chân quạt

  setup_wifi();

  // Thiết lập kết nối bảo mật MQTT, bỏ qua xác thực chứng chỉ
  espClient.setInsecure(); // Sử dụng setInsecure nếu không có chứng chỉ CA
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // Bắt đầu cảm biến DHT
  dht.begin();

  // Khởi tạo Servo
  servo.attach(14); // Kết nối servo với chân D14
  servo.write(0);   // Đặt servo ở góc 0 độ (cửa đóng)
}

void loop() {
  if (!client.connected()) {
    reconnect(); // Kết nối lại nếu MQTT không kết nối
  }
  client.loop();

  // Đọc dữ liệu từ cảm biến DHT11
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Kiểm tra nếu dữ liệu đọc từ cảm biến hợp lệ
  if (!isnan(h) && !isnan(t)) {
    String payload = "{\"temperature\":" + String(t) + ",\"humidity\":" + String(h) + "}";
    Serial.println("Publishing: " + payload);
    client.publish("home/sensor", payload.c_str()); // Gửi dữ liệu lên MQTT Broker
  } else {
    Serial.println("Failed to read from DHT sensor!");
  }

  delay(2000);
}
