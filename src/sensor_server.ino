// libraries
#include "homeiot_shared.h" 
//#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h" 
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>



// API
char apiServerHostname[] = "homeiot.aspifyhost.cz";
int apiServerPort = 80;
String apiAddress = "/API/Write?sensorIdentificator=#ID&value=#VAL";
// API sensor identificators
const String SENSOR_SELF_INTERNAL_TEMPERATURE_DATA_IDENTIFICATOR = "sensor_server_ambient_temperature";  // identifikator cidla interni teploty
const String SENSOR_KOTELNA_INTERNAL_TEMPERATURE_DATA_IDENTIFICATOR = "kotelna_ambient_temperature";  // identifikator cidla interni teploty v kotelne
const String SENSOR_KOTELNA_SMOKE_TEMPERATURE_DATA_IDENTIFICATOR = "kotelna_smoke_temperature";  // identifikator cidla teploty kourovodu v kotelne
const String SENSOR_KOTELNA_OUTPUT_TEMPERATURE_DATA_IDENTIFICATOR = "kotelna_output_temperature";  // identifikator cidla teploty vystupu z kotle v kotelne
const String SENSOR_KOTELNA_RETURN_TEMPERATURE_DATA_IDENTIFICATOR = "kotelna_return_temperature";  // identifikator cidla teploty zpatecky kotle v kotelne

const String SENSOR_OUTSIDE_TEMPERATURE_DATA_IDENTIFICATOR = "outside_temperature";  // identifikator cidla venkovni teploty




// WiFi
const char* nazevWifi = "Internet";
const char* hesloWifi = "qawsedrftgzh!";
WiFiClient client;





// Sensor server
RF24 nRF(2, 15);   // init pins CE, CS
const int RF_KOTELNA_PIPE = 0;
const int RF_OUTSIDE_PIPE = 1;





// OneWire sensor
#define ONE_WIRE_BUS 5
OneWire ds(ONE_WIRE_BUS);
DallasTemperature senzoryDS(&ds);

unsigned long lastSensorValuesSent = 0;  // time from last internal sensor notification
const unsigned long SENSOR_VALUES_SEND_INTERVAL = 5UL * 60UL * 1000UL; // internal sensor notification interval



// send sensor value to API
void sendData(String identificator, float value)
{
	if (value == 0 || value == -127)
	{
		Serial.println(F("Preskoceno odesilani nulove teploty"));
		Serial.println();
		Serial.println();
		return;
	}

	Serial.println("Odesilam teplotu " + String(value) + " ze senzoru " + identificator);

	// API address params
	String addr = apiAddress;
	addr.replace("#ID", identificator);
	addr.replace("#VAL", String(value));

	Serial.println("Pripojuji k " + String(apiServerHostname));


	if (client.connect(apiServerHostname, apiServerPort))
	{
		// send GET
		Serial.println(F("Pripojeno. Odesilam data."));

		client.println("GET " + addr + " HTTP/1.1");
		client.println("Host: " + String(apiServerHostname));
		client.println(F("Accept: */*"));
		client.println(F("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/75.0.3770.100 Safari/537.36"));
		client.println(F("Connection: close"));
		client.println();

		Serial.println(F("Odeslano. Odpoved:"));

		// read response
		String line = "x";
		while (client.connected())
		{
			line = client.readStringUntil('\r');

			// read till empty string
			if (line.length() > 1)
				Serial.print(line);
			else
				break;
		}
		client.stop();

		Serial.println();
		Serial.println(F("Hotovo."));
		Serial.println();
		Serial.println();
	}

	else
	{
		Serial.println(F("Spojeni se nepovedlo."));
	}
}

void setup()
{
	// serial line communication with 9600 baud
	Serial.begin(9600);

	Serial.println(F("Startuji"));

	// set OneWire port
	Serial.println(F("  IO porty"));
	pinMode(ONE_WIRE_BUS, INPUT);

	// init WiFi
	Serial.print(F("  Wifi"));
	WiFi.begin(nazevWifi, hesloWifi);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println("");


	// begin nRF24 communication
	Serial.println(F("  RF24"));
	nRF.begin();

	initRF(nRF);

	//nRF.openWritingPipe(adresaPrijimac);
	nRF.openReadingPipe(RF_KOTELNA_PIPE, RF_KOTELNA_ADDRESS);
	nRF.openReadingPipe(RF_OUTSIDE_PIPE, RF_OUTSIDE_ADDRESS);
	nRF.startListening();

	// settle time
	delay(2000);

	Serial.println(F("Nastartovano"));
}



bool checkCommunication(int sensorId, float value)
{
	if (sensorId > 1000 || sensorId <= 0)
	{
		// data order error, flush
		Serial.println("Chyba pri prijmu dat. Mazu buffer.");
		nRF.flush_rx();
		return false;
	}
	if (value > 1000 || value == 0)
	{
		// data order error, flush
		Serial.println("Chyba pri prijmu dat 2. Mazu buffer.");
		nRF.flush_rx();
		return false;
	}
	return true;
}


void loop()
{
	unsigned long time = millis();

	if (time - lastSensorValuesSent > SENSOR_VALUES_SEND_INTERVAL)
	{
		// send internal temperature
		senzoryDS.requestTemperatures();
		float temp = senzoryDS.getTempCByIndex(0);

		sendData(SENSOR_SELF_INTERNAL_TEMPERATURE_DATA_IDENTIFICATOR, temp);
		lastSensorValuesSent = time;
	}


	// read data from RF
	if (nRF.available())
	{
		// wait for data
		while (nRF.available())
		{
			Serial.println(F("Nova data ze vzdaleneho senzoru"));

			//nRF.read(&data, sizeof(data));
			//Serial.println(data.sensorId);
			//Serial.println(data.value);

			long sensorId;
			float sensorValue;

			if (!readData(nRF, &sensorId, &sensorValue))
				break;

			switch (sensorId)
			{
				case RF_SENSOR_KOTELNA_INTERNAL_TEMPERATURE_ID:
					Serial.println(SENSOR_KOTELNA_INTERNAL_TEMPERATURE_DATA_IDENTIFICATOR + ": " + String(sensorValue));
					sendData(SENSOR_KOTELNA_INTERNAL_TEMPERATURE_DATA_IDENTIFICATOR, sensorValue);
					break;
				case RF_SENSOR_KOTELNA_SMOKE_TEMPERATURE_ID:
					Serial.println(SENSOR_KOTELNA_SMOKE_TEMPERATURE_DATA_IDENTIFICATOR + ": " + String(sensorValue));
					sendData(SENSOR_KOTELNA_SMOKE_TEMPERATURE_DATA_IDENTIFICATOR, sensorValue);
					break;
				case RF_SENSOR_KOTELNA_OUTPUT_TEMPERATURE_ID:
					Serial.println(SENSOR_KOTELNA_OUTPUT_TEMPERATURE_DATA_IDENTIFICATOR + ": " + String(sensorValue));
					sendData(SENSOR_KOTELNA_OUTPUT_TEMPERATURE_DATA_IDENTIFICATOR, sensorValue);
					break;
				case RF_SENSOR_KOTELNA_RETURN_TEMPERATURE_ID:
					Serial.println(SENSOR_KOTELNA_RETURN_TEMPERATURE_DATA_IDENTIFICATOR + ": " + String(sensorValue));
					sendData(SENSOR_KOTELNA_RETURN_TEMPERATURE_DATA_IDENTIFICATOR, sensorValue);
					break;

				case RF_SENSOR_OUTSIDE_TEMPERATURE_ID:
					Serial.println(SENSOR_OUTSIDE_TEMPERATURE_DATA_IDENTIFICATOR + ": " + String(sensorValue));
					sendData(SENSOR_OUTSIDE_TEMPERATURE_DATA_IDENTIFICATOR, sensorValue);
					break;
				default:
					Serial.println("unknown (" + String(sensorId) + "): " + String(sensorValue));
					break;
			}
		}
		Serial.println();
	}
}