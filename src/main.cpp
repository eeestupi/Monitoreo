// Include libraries
#include <HTTPClient.h>
#include <Arduino.h>
#include "MAX30100_PulseOximeter.h"
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include "heartRate.h"
#include <Adafruit_MLX90614.h>
#include <vector>
#include <WiFi.h>
#include "InfluxDB.h"


//Conectar a wifi
const char* ssid     = "NETLIFE-ENRIQUEZ"; 
const char* password = "kendy1998"; 

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
// Function to calculate the average of a vector
float average(std::vector<float>& v) {
  float sum = 0;
  for(int i = 0; i < v.size(); i++)
  {
    sum += v[i];
  }
  return sum / v.size();
}

void loop() {
  // Update oximeter values
  pox.update();
  
  // Gather sensor data
  long irValue = particleSensor.getIR();
  
  // Check if a finger is on the sensor
  String fingerStatus = irValue < 50000 ? "DEDO NO DETECTADO" : "DEDO DETECTADO";
  
  if(fingerStatus == "DEDO DETECTADO"){

    if (tiempo < 30){
      //Get heart rate and SpO2
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
        if(spo2 > 50) {
          listSpO2.push_back(spo2);
        }
        if(heartRate > 40) {
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
    }
    else {
        

      // Calculate and print averages
      if (!listSpO2.empty()) {
        Serial.print("Promedio SpO2 = ");
        Serial.println(average(listSpO2));
      }
      if (!listHeartRate.empty()) {
        Serial.print("Promedio Heart Rate = ");
        Serial.println(average(listHeartRate));
      }
      if (!listTPaciente.empty()) {
        Serial.print("Promedio TPaciente = ");
        Serial.println(average(listTPaciente));
      }
      if (!listTAmbiente.empty()) {
        Serial.print("Promedio TAmbiente = ");
        Serial.println(average(listTAmbiente));
      }

      // Clear the vectors for the next run
      listSpO2.clear();
      listHeartRate.clear();
      listTPaciente.clear();
      listTAmbiente.clear();

      // Wait before looping again
      tiempo=0;
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("ENVIANDO DATOS");
      display.println("AL DOCTOR");
      display.println("ESPERE 10 SEGUNDOS");
      display.println("RETIRE EL DEDO");
      display.display();
      delay(10000);
      tiempo=0;
    }
     
  }
  else{
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("COLOCA TU DEDO ");
      display.println("POR 30 SEGUNDOS ");
      display.println("POR FAVOR ");
      display.println("Status: " + fingerStatus);
      display.display();
  } 
}
