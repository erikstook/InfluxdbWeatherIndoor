

#include <Arduino.h>
#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "BME280"
#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#define DEVICE "BME280"
#endif

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>

#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; // I2C
float h, t, p, pin, dp, a;
// WiFi AP SSID
#define WIFI_SSID "ASUS"
// WiFi password
#define WIFI_PASSWORD "guildford"
// InfluxDB v2 server url, e.g. https://eu-central-1-1.aws.cloud2.influxdata.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
#define INFLUXDB_URL "http://192.168.1.113:8086"
// InfluxDB v2 server or cloud API token (Use: InfluxDB UI -> Data -> API Tokens -> <select token>)
#define INFLUXDB_TOKEN "4-FdOgrZMpYZ7LtZ6snmgxw4a3Z9WCFiJam4NRDYKfiHHb6i0Xk0DDmwytomAfUq7BTHd-y1YC4s05G4M0yHQQ=="
// InfluxDB v2 organization id (Use: InfluxDB UI -> User -> About -> Common Ids )
#define INFLUXDB_ORG "ESOX"
// InfluxDB v2 bucket name (Use: InfluxDB UI ->  Data -> Buckets)
#define INFLUXDB_BUCKET "Temperatur"

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
Point sensor("vader");
float tempout;
void setup() {
  Serial.begin(115200);


if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
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

  // Add tags
  sensor.addTag("BME280", DEVICE);
  sensor.addTag("Tempout", "Temperatur");
  sensor.addTag("Pressure", "Lufttryck");
  sensor.addTag("Humid", "Luftfuktighet");
    sensor.addTag("Altitude", "Höjd");
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


  float p = bme.readPressure()/100.0F;
  float a = bme.readAltitude(SEALEVELPRESSURE_HPA);

  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)


  float h = bme.readHumidity();
    // Read temperature as Celsius (the default)
  float t = bme.readTemperature();

  // Store measured value into point
  // Report RSSI of currently connected network
  tempout = random(10,50);
  Serial.print(" temp:");
  Serial.println(t);
  sensor.addField("temp", t);
  Serial.print(" humid:");
  Serial.println(h);
  sensor.addField("humid", h);
  Serial.print(" pressure:");
  Serial.println(p);
  sensor.addField("pressure", p);
  Serial.print(" altitude:");
  Serial.println(a);
  sensor.addField("altitude", a);

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

  Serial.println("Wait 10s");
  delay(10000);
}
