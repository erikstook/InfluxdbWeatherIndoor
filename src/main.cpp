

#include <Arduino.h>

#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#define DEVICE "ESP8266"
#endif
#include <Adafruit_BMP280.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
//#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <ArduinoOTA.h>

#include <PubSubClient.h>
#include <Wire.h>


#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BMP280 bme; // I2C
float h, t, p, pin, dp, a;
static char charBuff[30];


// WiFi AP SSID
#define WIFI_SSID "OMDUVISSTE"
// WiFi password
#define WIFI_PASSWORD "grodanboll"
// InfluxDB v2 server url, e.g. https://eu-central-1-1.aws.cloud2.influxdata.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
#define INFLUXDB_URL "http://192.168.3.181:8086"
// InfluxDB v2 server or cloud API token (Use: InfluxDB UI -> Data -> API Tokens -> <select token>)
#define INFLUXDB_TOKEN "DWLHTXbHzv2yHIlwNSY2rFQMQ0Y-GZZpnPZIQAn9Q5_2nc54mmLv_h2ZCII4TmR1nXvEvz2T3Iy6phimUqCyLw=="
// InfluxDB v2 organization id (Use: InfluxDB UI -> User -> About -> Common Ids )
#define INFLUXDB_ORG "bc9ef2b84d88750f"
// InfluxDB v2 bucket name (Use: InfluxDB UI ->  Data -> Buckets)
#define INFLUXDB_BUCKET "Weather_indoor"

// Set timezone string according to https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
// Examples:
//  Pacific Time: "PST8PDT"
//  Eastern: "EST5EDT"
//  Japanesse: "JST-9"
//  Central Europe: "CET-1CEST,M3.5.0,M10.5.0/3"
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"

// InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// Data point
Point sensor("Weather_indoor");

unsigned long lastMs = 0;

unsigned long ms = millis();
float temp_calib = 2.00;


void setup() {
  Serial.begin(115200);


if (!bme.begin(0x76,0x58)) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }

  // Setup wifi
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to wifi");
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();
  Serial.println(WiFi.localIP());

// OTA 

    ArduinoOTA.setPort(8266); // SET to FIXED PORT
    ArduinoOTA.onStart([]() {
       Serial.println("Start");
       });
       ArduinoOTA.onEnd([]() {
         Serial.println("\nEnd");
       });
       ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
         Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
       });
       ArduinoOTA.onError([](ota_error_t error) {
         Serial.printf("Error[%u]: ", error);
         if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
         else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
         else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed Firewall Issue ?");
         else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
         else if (error == OTA_END_ERROR) Serial.println("End Failed");
       });
       ArduinoOTA.begin();
       Serial.println("OTA Ready");


  // Add tags
  sensor.addTag("BMP280_2", DEVICE);
  sensor.addTag("Temp_indoor_2", "Temperatur_2");
  sensor.addTag("Pressure_indoor_2", "Lufttryck_2");
//  sensor.addTag("Humid_outdoor_2", "Luftfuktighet_2");
//  sensor.addTag("Altitude_outdoor_2", "Höjd_2");
  // Accurate time is necessary for certificate validation and writing in batches
  // For the fastest time sync find NTP servers in your area: https://www.pool.ntp.org/zone/
  // Syncing progress and the time will be printed to Serial.
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}
void loop() {
  // Clear fields for reusing the point. Tags will remain untouched
  sensor.clearFields();
//  Serial.println(WiFi.localIP());
  ArduinoOTA.handle();

  // Publishes new temperature and humidity every 30 seconds
    if (millis() - ms > 120000) {
      ms = millis();



 timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  float p = bme.readPressure()/100.0F;
  p = p + 7;  //Calibration +7 hPA
  float a = bme.readAltitude(SEALEVELPRESSURE_HPA);

  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)


  //float h = bme.readHumidity();
    // Read temperature as Celsius (the default)
  float t = bme.readTemperature();
  t = t - temp_calib; // Calibrating temp
     static char temperatureTemp[7];
   //  static char humidityString[6];
     static char altitudeString[8];
     static char pressureString[7];

  // Read temperature as Fahrenheit (isFahrenheit = true)
    //float f = bme.readTemperature(true);
    dtostrf(t, 5, 1, temperatureTemp);
   // dtostrf(h, 5, 1, humidityString);
    dtostrf(p, 6, 1, pressureString);
    dtostrf(a, 6, 1, altitudeString);
  char fullPrint[60];
    strcpy(fullPrint, temperatureTemp);
    strcat(fullPrint, ";");
  //  strcat(fullPrint,humidityString);
  //  strcat(fullPrint, ";");
    strcat(fullPrint,pressureString);
    strcat(fullPrint, ";");
    strcat(fullPrint,charBuff);
    Serial.print(fullPrint);

  // Store measured value into point
  // Report RSSI of currently connected network
  Serial.print(" temp:");
  Serial.println(t);
  sensor.addField("temp_2", t);
 // Serial.print(" humid:");
 // Serial.println(h);
  sensor.addField("humid_2", h);
  Serial.print(" pressure:");
  Serial.println(p);
  sensor.addField("pressure_2", p);
  Serial.print(" altitude:");
  Serial.println(a);
  sensor.addField("altitude_2", a);

  // Print what are we exactly writing
  Serial.print("Writing: ");
  Serial.println(sensor.toLineProtocol());

  // Check WiFi connection and reconnect if needed
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Wifi connection lost");
  }

  // Write point
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }

//  Serial.println("Wait 20s");
 // delay(20000);
}
}
