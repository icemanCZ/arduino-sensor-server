//#include <SPI.h>
#include "shared.h" 
#include "RF24.h" 
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>



// API
char apiServerHostname[] = "homeiot.aspifyhost.cz";
int apiServerPort = 80;
String apiAddress = "/API/Write?sensorIdentificator=#ID&value=#VAL";
// senzory
const String SELF_INTERNAL_TEMPERATURE_DATA_IDENTIFICATOR = "sensor_server_ambient_temperature";  // identifikator cidla interni teploty
const String KOTELNA_INTERNAL_TEMPERATURE_DATA_IDENTIFICATOR = "kotelna_ambient_temperature";  // identifikator cidla interni teploty v kotelne
const String KOTELNA_SMOKE_TEMPERATURE_DATA_IDENTIFICATOR = "kotelna_smoke_temperature";  // identifikator cidla teploty kourovodu v kotelne
const String KOTELNA_OUTPUT_TEMPERATURE_DATA_IDENTIFICATOR = "kotelna_output_temperature";  // identifikator cidla teploty vystupu z kotle v kotelne
const String KOTELNA_RETURN_TEMPERATURE_DATA_IDENTIFICATOR = "kotelna_return_temperature";  // identifikator cidla teploty zpatecky kotle v kotelne




// WiFi
const char* nazevWifi = "Internet";
const char* hesloWifi = "qawsedrftgzh!";
WiFiClient client;





// Sensor server
// nastaveni propojovacich pinu
RF24 nRF(2, 15);
const byte RF_ADDRESS_KOTELNA[] = "sensorKotelna";
const int RF_PIPE_KOTELNA = 1;





// Interni senzor teploty
#define ONE_WIRE_BUS 5
OneWire ds(ONE_WIRE_BUS);
DallasTemperature senzoryDS(&ds);

unsigned long lastTemperatureSent = 0;  // cas od posledniho nahlaseni interni teploty
const unsigned long TEMPERATURE_INTERVAL = 60000; // interval nahlasovani interni teploty





//typedef struct {
//	int sensorId;
//	float value;
//} payload;
//
//payload data;





void setup()
{
	// inicializace serioveho portu
	Serial.begin(9600);

	Serial.println(F("Startuji"));

	// nastavim port cidla
	Serial.println(F("  IO porty"));
	pinMode(ONE_WIRE_BUS, INPUT);

	// inicalizace site
	Serial.print(F("  Wifi"));
	WiFi.begin(nazevWifi, hesloWifi);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println("");


	// zapnuti komunikace nRF modulu
	Serial.println(F("  RF24"));
	nRF.begin();

	nRF.setPALevel(RF24_PA_HIGH);
	//nRF.disableDynamicPayloads();
	//nRF.setDataRate(RF24_250KBPS);
	//nRF.setAutoAck(false);
	//nRF.setChannel(50);

	// nastaveni zapisovaciho a cteciho kanalu
	//nRF.openWritingPipe(adresaPrijimac);
	nRF.openReadingPipe(RF_PIPE_KOTELNA, RF_ADDRESS_KOTELNA);
	// zacatek prijmu dat
	nRF.startListening();

	// cas na usazeni
	delay(2000);

	Serial.println(F("Nastartovano"));
}


//odesle hodnotu senzoru do API
void sendData(String identificator, float value)
{
	if (value == 0)
	{
		Serial.println(F("Preskoceno odesilani nulove teploty"));
		Serial.println();
		Serial.println();
		return;
	}

	Serial.println("Odesilam teplotu " + String(value) + " ze senzoru " + identificator);

	// pripravim parametry adresy pro API
	String addr = apiAddress;
	addr.replace("#ID", identificator);
	addr.replace("#VAL", String(value));

	Serial.println("Pripojuji k " + String(apiServerHostname));


	if (client.connect(apiServerHostname, apiServerPort))
	{
		// poslu GET
		Serial.println(F("Pripojeno. Odesilam data."));

		client.println("GET " + addr + " HTTP/1.1");
		client.println("Host: " + String(apiServerHostname));
		client.println(F("Accept: */*"));
		client.println(F("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/75.0.3770.100 Safari/537.36"));
		client.println(F("Connection: close"));
		client.println();

		Serial.println(F("Odeslano. Odpoved:"));

		// prectu odpoved
		String line = "x";
		while (client.connected())
		{
			line = client.readStringUntil('\r');

			// cti, dokud neprijde prazdny string
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

void loop()
{
	unsigned long time = millis();

	if (time - lastTemperatureSent > TEMPERATURE_INTERVAL)
	{
		// odeslu interni teplotu
		senzoryDS.requestTemperatures();
		float temp = senzoryDS.getTempCByIndex(0);

		sendData(SELF_INTERNAL_TEMPERATURE_DATA_IDENTIFICATOR, temp);
		lastTemperatureSent = time;
	}


	// prijem dat z RF
	if (nRF.available())
	{
		// cekani na prijem dat
		long sensorId;
		float sensorValue;
		while (nRF.available())
		{
			Serial.println(F("Nova data ze vzdaleneho senzoru"));

			//nRF.read(&data, sizeof(data));
			//Serial.println(data.sensorId);
			//Serial.println(data.value);


			nRF.read(&sensorId, sizeof(sensorId));
			delay(20);
			nRF.read(&sensorValue, sizeof(sensorValue));

			switch (sensorId)
			{
			case SENSOR_KOTELNA_INTERNI_ID:
				Serial.println(KOTELNA_INTERNAL_TEMPERATURE_DATA_IDENTIFICATOR + ": " + String(sensorValue));
				sendData(KOTELNA_INTERNAL_TEMPERATURE_DATA_IDENTIFICATOR, sensorValue);
				break;
			case SENSOR_KOTELNA_KOUROVOD_ID:
				Serial.println(KOTELNA_SMOKE_TEMPERATURE_DATA_IDENTIFICATOR + ": " + String(sensorValue));
				sendData(KOTELNA_SMOKE_TEMPERATURE_DATA_IDENTIFICATOR, sensorValue);
				break;
			case SENSOR_KOTELNA_VYSTUP_ID:
				Serial.println(KOTELNA_OUTPUT_TEMPERATURE_DATA_IDENTIFICATOR + ": " + String(sensorValue));
				sendData(KOTELNA_OUTPUT_TEMPERATURE_DATA_IDENTIFICATOR, sensorValue);
				break;
			case SENSOR_KOTELNA_ZPATECKA_ID:
				Serial.println(KOTELNA_RETURN_TEMPERATURE_DATA_IDENTIFICATOR + ": " + String(sensorValue));
				sendData(KOTELNA_RETURN_TEMPERATURE_DATA_IDENTIFICATOR, sensorValue);
				break;
			default:
				Serial.println("unknown (" + String(sensorId) + "): " + String(sensorValue));
				break;
			}

			if (sensorId > 1000)
			{
				// rozhodilo se nam poradi, zkusim to srovnat
				if (nRF.available()) 
				{
					nRF.read(&sensorId, sizeof(sensorId));
					Serial.println("Syncing by skipping value " + sensorId);
				}
			}

		}
		Serial.println();
	}
}