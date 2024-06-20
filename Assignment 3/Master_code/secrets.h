// Use this file to store all of the private credentials and connection details

// MQTT access
#define MQTT_BROKERIP "149.132.178.179"         // IP address of the machine running the MQTT broker
#define MQTT_CLIENTID "esp8266_903604"                 // client identifier
#define MQTT_USERNAME "AndreaPascuzzi"         // mqtt user's name
#define MQTT_PASSWORD "iot903604"         // mqtt user's password

// WiFi configuration
#define SECRET_SSID "IoTLabThingsU14"                  // SSID
#define SECRET_PASS "L@b%I0T*Ui4!P@sS**0%Lessons!"              // WiFi password

// ONLY if static configuration is needed
#define IP {149, 132, 182, 153}                    // IP address
#define SUBNET {255, 255, 255, 0}                // Subnet mask
#define DNS {149, 132, 2, 3}                     // DNS
#define GATEWAY {149, 132, 182, 1}               // Gateway


// MySQL access
#define MYSQL_IP {149, 132, 178, 179}            // IP address of the machine running MySQL
#define MYSQL_USER "AndreaPascuzzi"                  // db user
#define MYSQL_PASS "iot903604"              // db user's password

// openweathermap.org configuration
#define WEATHER_API_KEY "927683efa13eae2d3fa3c234e609b1de"           // api key form https://home.openweathermap.org/api_keys
#define WEATHER_CITY "Milan"                     // city
#define WEATHER_COUNTRY "it"                     // ISO3166 country code 
