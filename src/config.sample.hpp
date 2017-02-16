//AP definitions
#define AP_SSID 		"HomeWiFiNetwork"
#define AP_PASSWORD 	"my_secure_wifi_password"

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883 // use 8883 for SSL
#define AIO_USERNAME    "my_username"
#define AIO_KEY         "12345678901234567890"

#define INFLUXDB_HOST 	"influxdb.local"
#define INFLUXDB_PORT	8086;

#define DATABASE		"incubator";
#define DB_USER			"my_influxdb_username";
#define DB_PASSWORD 	"my_influxdb_password";

#define RELAY_PIN 		D1	// Heater relay pin
#define ONE_WIRE_BUS 	D7  // DS18B20 data pin

#define SENSOR_INSIDE_EGG_CUP 	0 // no markings
#define SENSOR_HEATER 			1 // orange + blue
#define SENSOR_BOX 				2 // arrows + pink

#define CHECK_TEMP_LOOP_PERIOD 10
