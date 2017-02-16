/*
* Relay Shield - Blink
* Turns on the relay for two seconds, then off for two seconds, repeatedly.
*
* Relay Shield transistor closes relay when D1 is HIGH
*/
#include "includes.hpp"
#include "config.hpp"

unsigned long last_loop_time = 0;

float temp_egg_cup;
float temp_heater;
float temp_box;
bool heater_status = false;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Publish incubator_temp_egg_cup = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/incubator_temp_egg_cup");
Adafruit_MQTT_Publish incubator_temp_box = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/incubator_temp_box");
Adafruit_MQTT_Publish incubator_temp_heater = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/incubator_temp_heater");
Adafruit_MQTT_Publish incubator_status_heater = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/incubator_status_heater");

Influxdb influxdb(INFLUXDB_HOST, INFLUXDB_PORT);

void wifi_connect();
void mqtt_connect();
void turn_on_heater();
void turn_off_heater();


void setup() {
	pinMode(RELAY_PIN, OUTPUT);

	// Force turn off the heater, since the pin is held HIGH when the MCU initializes.
	turn_off_heater();

	Serial.begin(115200);
	Serial.setDebugOutput(true);

	wifi_connect();
	influxdb.opendb(DATABASE, DB_USER, DB_PASSWORD);

	ArduinoOTA.onStart([]() {});
	ArduinoOTA.onEnd([]() {
	    Serial.println("\nFlash successful");

	    // Force restart the MCU. Otherwise it hangs when the flash is complete.
		ESP.restart();
	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
	    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
	});
	ArduinoOTA.onError([](ota_error_t error) {
	    Serial.printf("Error[%u]: ", error);
	    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
	    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
	    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
	    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
	    else if (error == OTA_END_ERROR) Serial.println("End Failed");
	});
	ArduinoOTA.begin();
	Serial.println("Ready");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
}

void loop() {
	DB_RESPONSE dbresponse;

	ArduinoOTA.handle();
	wifi_connect();

	if (millis() > ( last_loop_time + CHECK_TEMP_LOOP_PERIOD*1000 ) ) {
	    mqtt_connect();

	    DS18B20.requestTemperatures();
	    temp_egg_cup = DS18B20.getTempCByIndex(SENSOR_INSIDE_EGG_CUP);
	    temp_heater = DS18B20.getTempCByIndex(SENSOR_HEATER);
	    temp_box = DS18B20.getTempCByIndex(SENSOR_BOX);

	    Serial.print("Temperature in egg_cup: ");
	    Serial.println(temp_egg_cup);

	    if (temp_egg_cup < 30.0) {
            turn_on_heater();
	    }
	    else if (temp_egg_cup >= 30.2) {
            turn_off_heater();
	    }


	    // Publish to Adafruit
	    Serial.print("Publishing temp egg_cup ");
	    if (!incubator_temp_egg_cup.publish(temp_egg_cup)) {
            Serial.println(F("Failed"));
	    } else {
            Serial.println(F("OK!"));
	    }

	    Serial.print("Publishing temp box ");
	    if (!incubator_temp_box.publish(temp_box)) {
            Serial.println(F("Failed"));
	    } else {
            Serial.println(F("OK!"));
	    }

	    Serial.print("Publishing temp heater ");
	    if (!incubator_temp_heater.publish(temp_heater)) {
            Serial.println(F("Failed"));
	    } else {
            Serial.println(F("OK!"));
	    }

	    Serial.print("Publishing heater status ");
	    if (!incubator_status_heater.publish(heater_status ? 1 : 0)) {
            Serial.println(F("Failed"));
	    } else {
            Serial.println(F("OK!"));
	    }

		// Writing data using FIELD object
		FIELD dataObj("incubator");
		dataObj.addTag("type", "gecko_incubator");
		dataObj.addField("temp_egg_cup", temp_egg_cup);
		dataObj.addField("temp_box", temp_box);
		dataObj.addField("temp_heater", temp_heater);
		dataObj.addField("heater_status", heater_status);

		Serial.println(influxdb.write(dataObj) == DB_SUCCESS ? "Writing sucess" : "Writing failed");

		// Empty field object.
		dataObj.empty();


	    last_loop_time = millis();
	}
}

void wifi_connect()
{
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

	int retries_counter = 0;

	do {
	    Serial.print("Connecting to AP");
	    WiFi.mode(WIFI_STA);
	    WiFi.begin(AP_SSID, AP_PASSWORD);
	    while (WiFi.status() != WL_CONNECTED) {
	        delay(1000);
	        Serial.print(".");
			retries_counter++;

			if (retries_counter > 10) {
				WiFi.disconnect();
				WiFi.mode(WIFI_OFF);
				delay(1000);
				Serial.println("Retrying to connect ... ");
				retries_counter = 0;
				break;
			}
	    }
	} while (WiFi.status() != WL_CONNECTED);

    Serial.println("");
    Serial.println("WiFi connected");
}

void mqtt_connect() {
    int8_t ret;

    // Stop if already connected.
    if (mqtt.connected()) {
        return;
    }

    Serial.print("Connecting to MQTT... ");

    uint8_t retries = 3;
    while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
        Serial.println(mqtt.connectErrorString(ret));
        Serial.println("Retrying MQTT connection in 5 seconds...");
        mqtt.disconnect();
        delay(5000); // wait 5 seconds
        retries--;
        if (retries == 0) {
            // reset retries and wait for next iteration
            retries = 3;
            break;
        }
		Serial.println("MQTT Connected!");
    }
}

void turn_on_heater() {
    digitalWrite(RELAY_PIN, HIGH);
    Serial.println("Turn ON heater");
    heater_status = true;
}


void turn_off_heater() {
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("Turn OFF heater");
    heater_status = false;
}
