//Streamflow
#define SFSensorPin 13
#define SFCalibrationFactor 90
volatile byte SFPulseCount = 0;
unsigned long SFPrintTimestamp = 0;
float SFRate = 0;
//TDS
#define TDSSensorPin 15
#define TDSVREF 5.0
#define TDSSCOUNT 50
static unsigned long TDSSamplingTimestamp = 0;
static unsigned long TDSPrintTimestamp = 0;
int TDSAnalogBuffer[TDSSCOUNT];  // store the analog value in the array, read from ADC
int TDSAnalogBufferTemp[TDSSCOUNT];
int TDSAnalogBufferIndex = 0;
float TDSAverageVoltage = 0;
float TDSValue = 0;

//pH
#define PHSensorPin 25
#define PHCOUNT 50
#define PHCoeff -1.2162
#define PHConst 7.3649
static unsigned long PHSamplingTimestamp = 0;
static unsigned long PHPrintTimestamp = 0;
float PHAnalogBuffer[PHCOUNT];  // store the analog value in the array, read from ADC
int PHAnalogBufferIndex = 0;
float PHValue = 7;

//MQTT
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

const char* wifissid = "BAV-AMO";
const char* wifipassword = "1234567890a";
const char* mqtt_server = "d8b54cc855d544539015413af5d6f9a1.s2.eu.hivemq.cloud";
const char* mqttusername= "aquaemi";
const char* mqttpassword= "aquaemipass";

WiFiClient espClient;
PubSubClient mqttclient(espClient);

static unsigned long publishTimestamp = 0;
//Global
#define waterTempPin 34
#define airTempPin 19
float temperature = 25;  // current temperature for compensation
float wqi=0;

int getMedianNum(int bArray[], int len) {
  int bTab[len];
  for (byte i = 0; i < len; i++) bTab[i] = bArray[i];
  int i, j, bTemp;

  //Simple sort
  for (j = 0; j < len; j++) {
    for (i = 0; i < len - j; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  return ((len & 1 > 0) ? bTab[len / 2] :              //Odd
            (bTab[len / 2] + bTab[len / 2 - 1]) / 2);  //Even
}

float getMean(float fArray[], int len) {
  float s = 0;
  for (int i=0; i< len; i++) {
    s += fArray[i];
  }
  return s / len;
}
void addTDSBuffer() {
  TDSAnalogBuffer[TDSAnalogBufferIndex] = analogRead(TDSSensorPin);  //read the analog value and store into the buffer
  TDSAnalogBufferIndex++;
  TDSAnalogBufferIndex %= TDSSCOUNT;
}

void printTDS() {
  TDSAverageVoltage = getMedianNum(TDSAnalogBuffer, TDSSCOUNT) * (float)TDSVREF / 1024.0;  // read the analog value more stable by the median filtering algorithm, and convert to voltage value

  float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);  //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));

  float compensationVoltage = TDSAverageVoltage / compensationCoefficient;  //temperature compensation

  TDSValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage - 255.86 * compensationVoltage * compensationVoltage + 857.39 * compensationVoltage) * 0.5;  //convert voltage value to tds value

  //Serial.print("voltage:");
  //Serial.print(TDSAverageVoltage,2);
  //Serial.print("V   ");
  Serial.print("TDS Value:");
  Serial.print(TDSValue, 0);
  Serial.println("ppm");
}

void SFPulseCounter() {
  SFPulseCount++;
}

void printSF() {
  // Disable the interrupt while calculating flow rate and sending the value to the host
  detachInterrupt(SFSensorPin);


  // Because this loop may not complete in exactly 1 second intervals we calculate the number of milliseconds that have passed since the last execution and use that to scale the output. We also apply the SFCalibrationFactor to scale the output based on the number of pulses per second per units of measure (litres/minute in this case) coming from the sensor.
  SFRate = ((1000.0 / (millis() - SFPrintTimestamp)) * SFPulseCount) / SFCalibrationFactor;
  Serial.print("Flow rate: ");
  Serial.print(SFRate/ 60 * 1000, DEC);  // Print the integer part of the variable
  Serial.println("mL/Second");
  SFPulseCount = 0;

  attachInterrupt(SFSensorPin, SFPulseCounter, FALLING);
}
void addPHBuffer() {
  PHAnalogBuffer[PHAnalogBufferIndex] = analogRead(PHSensorPin)*5.0/4065.0;  //read the analog value and store into the buffer
  PHAnalogBufferIndex++;
  PHAnalogBufferIndex %= PHCOUNT;
}

void printPH() {
  Serial.print("pH: ");
  PHValue =6+ PHCoeff*getMean(PHAnalogBuffer, PHCOUNT)+PHConst;
  Serial.println(PHValue);
}

void printWQI() {
  Serial.print("WQI: ");
  wqi = 100 * ((abs((1.0/5.0) * ((PHValue - 7.0) / 1.5)) + (1.0/7.0) * ((TDSValue - min((float)400,TDSValue)) / 400.0))) / 0.12;
  Serial.println(wqi);
  Serial.print("This water is: ");
  Serial.println((wqi <= 10) ? "Clean" : ((wqi <= 30) ? "Acceptable" : ((wqi <= 60) ? "Fair" : "Poor")));
}

void setup() {

  // Initialize a serial connection for reporting values to the host
  Serial.begin(115200);

  //***** Streamflow
  pinMode(SFSensorPin, INPUT);
  digitalWrite(SFSensorPin, HIGH);
  attachInterrupt(SFSensorPin, SFPulseCounter, FALLING);  //you can use Rising or Falling

  //***** TDS
  pinMode(TDSSensorPin, INPUT);
  //pH
  pinMode(PHSensorPin, INPUT);
  pinMode(waterTempPin, INPUT);
  //MQTT
  setup_wifi();
  mqttclient.setServer(mqtt_server, 1883);

  //Global
  pinMode(airTempPin, INPUT);
}

void setup_wifi(){
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifissid);

  WiFi.begin(wifissid, wifipassword);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttclient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttclient.connect("esp32", mqttusername, mqttpassword)) {
      Serial.println("Connected successfully");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(mqttclient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void publishMQTT(){
  mqttclient.publish("aquaemiIOT/pH",String(PHValue,3).c_str());
  mqttclient.publish("aquaemiIOT/wqi", String(wqi,3).c_str());
  mqttclient.publish("aquaemiIOT/streamflow", String(SFRate,3).c_str());
  mqttclient.publish("aquaemiIOT/TDS", String(TDSValue,3).c_str());
  Serial.println("Message sent");
}

void loop() {
  if (!mqttclient.connected()) {
    reconnect();
  }
  if(millis() - TDSPrintTimestamp > 1000U){
    printTDS();
    TDSPrintTimestamp = millis();
  }
  if(millis() - TDSSamplingTimestamp > 40U){     //(1) every 40 milliseconds,read the analog value from the ADC so every 800ms we get the last 30 values instead of 31
    addTDSBuffer();
    TDSSamplingTimestamp = millis();
  }   
  if(millis() - SFPrintTimestamp > 1000U){ 
    printSF();  // Only process counters once per second
    SFPrintTimestamp=millis();
  }
  if (millis() - PHPrintTimestamp > 1000U){
    printPH();
    PHPrintTimestamp=millis();
    printWQI();
  }
  if(millis() - PHSamplingTimestamp > 40U){     //(1) every 40 milliseconds,read the analog value from the ADC so every 800ms we get the last 30 values instead of 31
    addPHBuffer();
    PHSamplingTimestamp = millis();
  }   
  if (millis() - publishTimestamp > 10000U){
    publishTimestamp=millis();
    publishMQTT();
  }
  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):

  //delay(100);
}