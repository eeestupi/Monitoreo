#include <Arduino.h>
#include <InfluxDbClient.h>
#include "MAX30100_PulseOximeter.h"
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "MAX30105.h"
#include <Adafruit_MLX90614.h>
#include <vector>
#include <WiFi.h>

//Conectar a wifi
const char* ssid = "Microcontroladores"; 
const char* password = "raspy123"; 

// Crear vectores
std::vector<float> listSpO2;
std::vector<float> listHeartRate;
std::vector<float> listTPaciente;
std::vector<float> listTAmbiente;

// OLED display configuracion
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Crear sensores y objetos
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);
PulseOximeter pox;
MAX30105 particleSensor;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

// Variable de tiempo 
int tiempo = 0;

// InfluxDB server URL. Don't use localhost, always use server name or IP address.
#define INFLUXDB_URL "http://200.126.14.234:8086/"
// InfluxDB 2 server or cloud API authentication token
#define INFLUXDB_TOKEN "5KycwxL5zMvN7b4fzQpawwYz7fHeMTMW"
// InfluxDB 2 organization ID
#define INFLUXDB_ORG "detect"
// InfluxDB 2 bucket name
#define INFLUXDB_BUCKET "IndicadoresSalud"

// InfluxDB client instance
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);

// Data point
Point paciente("Paciente");

// Function to calculate the average of a vector
float average(std::vector<float>& v) {
  float sum = 0;
  for (int i = 0; i < v.size(); i++) {
    sum += v[i];
  }
  return sum / v.size();
}

void setup() {
  // Initialize serial communication
  Serial.begin(9600);

  // Initialize WiFi
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  
  // Show the IP address of ESP32 when it's successfully connected
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  // Initialize temperature sensor
  mlx.begin();

  // Initialize the OLED display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);

  // Initialize the MAX30105 sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }

  // Additional sensor configuration
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);
}

void loop() {
  // Update oximeter values
  pox.update();
  
  // Gather sensor data
  long irValue = particleSensor.getIR();
  
  // Check if a finger is on the sensor
  String fingerStatus = irValue < 50000 ? "DEDO NO DETECTADO" : "DEDO DETECTADO";
  
  if (fingerStatus == "DEDO DETECTADO") {
    if (tiempo < 30) {
      // Get heart rate and SpO2
      float heartRate = pox.getHeartRate();
      Serial.print("Heart Rate = ");
      Serial.print(heartRate);

      float spo2 = pox.getSpO2();
      Serial.print("  SpO2 = ");
      Serial.print(spo2); 
      
      // Print temperature readings to serial monitor
      float TAmbiente = mlx.readAmbientTempC();
      Serial.print("  TAmbiente = ");
      Serial.print(TAmbiente); 
      float TPaciente = mlx.readObjectTempC();
      Serial.print("  ºC\tTPaciente = "); 
      Serial.print(TPaciente); 
      Serial.println("ºC");
      
      // Store values in lists if they meet the condition
      if (spo2 > 0 & spo2 < 100) {
        listSpO2.push_back(spo2);
      }
      if (heartRate > 30 & heartRate < 250) {
        listHeartRate.push_back(heartRate);
      } 
      listTPaciente.push_back(TPaciente);
      listTAmbiente.push_back(TAmbiente);

      // Display values on the OLED screen
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("SPO2: " + String(spo2));
      display.println("Heart Rate: " + String(heartRate));
      display.println("TAmbiente: " + String(TAmbiente));
      display.println("TPaciente: " + String(TPaciente)+ "C");
      display.println("Tiempo: " + String(tiempo));
      display.display();
      
      // Wait before looping again
      delay(1000);
      tiempo += 1;
    } else {
      // Calculate and print averages
      float avgSpO2 = average(listSpO2);
      float avgHeartRate = average(listHeartRate);
      float avgTPaciente = average(listTPaciente);
      float avgTAmbiente = average(listTAmbiente);

      Serial.print("Promedio SpO2 = ");
      Serial.println(avgSpO2);
      Serial.print("Promedio Heart Rate = ");
      Serial.println(avgHeartRate);
      Serial.print("Promedio TPaciente = ");
      Serial.println(avgTPaciente);
      Serial.print("Promedio TAmbiente = ");
      Serial.println(avgTAmbiente);

      // Clear the vectors for the next run
      listSpO2.clear();
      listHeartRate.clear();
      listTPaciente.clear();
      listTAmbiente.clear();

      // Create a point with the average values
      paciente.clearFields();
      paciente.addField("spo2", avgSpO2);
      paciente.addField("heart_rate", avgHeartRate);
      paciente.addField("tem_paciente", avgTPaciente);
      paciente.addField("tem_ambiente", avgTAmbiente);
      paciente.addField("id", "0932065253"); // Replace "your_unique_id" with your unique identifier

      // Print the line protocol of the point (for debugging purposes)
      Serial.print("Writing: ");
      Serial.println(client.pointToLineProtocol(paciente));

      // Send data to InfluxDB
      if (client.writePoint(paciente)) {
        Serial.println("Data sent to InfluxDB");
      } else {
        Serial.println("Failed to send data to InfluxDB.");
      }

      // Reset the time counter
      tiempo = 0;
    }
  }
}
