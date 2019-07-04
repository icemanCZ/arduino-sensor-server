#include <SPI.h>
#include <Ethernet.h>
#include "RF24.h" 
#include <OneWire.h>

// API
char apiServerHostname[] = "homeiot.aspifyhost.cz";
int apiServerPort = 80;
String apiAddress = "/API/Write?sensorIdentificator=#ID&value=#VAL";

// Ethernet
byte mac[] = { 0xD4, 0xAD, 0xBE, 0xEF, 0xFE, 0x7D };
IPAddress ip(192, 168, 1, 155);
EthernetClient client;


// Sensor server
// nastaveni propojovacich pinu
#define CE 7
#define CS 8
// inicializace nRF s piny CE a CS
RF24 nRF(CE, CS);

char adresaPrijimac[] = "sensorServer";
char adresaVysilac[] = "sensorKotelna";


// senzor teploty
#define P_CIDLO 6
OneWire ds(P_CIDLO);

unsigned long lastTemperatureSent = 0;	// cas od posledniho nahlaseni teploty
const unsigned long TEMPERATURE_INTERVAL = 60000;	// interval nahlasovani teploty
const String TEMPERATURE_DATA_IDENTIFICATOR = "sensor_server_ambient_temperature";	// identifikator cisla teploty



void setup()
{
	// vypnu sd kartu
	pinMode(10, OUTPUT);
	digitalWrite(10, HIGH);

	// nastavim port cidla
	pinMode(P_CIDLO, INPUT);

	// inicializace serioveho portu
	Serial.begin(9600);

	// inicalizace site
	Serial.println(F("Startuji ethernet."));
	Ethernet.begin(mac, ip);
	Serial.println(Ethernet.localIP());



	//// zapnuti komunikace nRF modulu
	//nRF.begin();

	//// nastaveni zapisovaciho a cteciho kanalu
	////nRF.openWritingPipe(adresaPrijimac);
	//nRF.openReadingPipe(1, adresaVysilac);
	//// zacatek prijmu dat
	//nRF.startListening();

	// cas na usazeni
	delay(1000);
	Serial.println(F("Nastartovano."));
}


int i = 1;
void loop()
{
	//Ethernet.maintain();

	unsigned long time = millis();

	if (time - lastTemperatureSent > 10000)
	{
		float temp = getTemp();
		Serial.println("Odesilam teplotu " + String(temp));
		sendData(TEMPERATURE_DATA_IDENTIFICATOR, temp);
		lastTemperatureSent = time;
	}


	//if (i++ % 10000 == 0)
	//	sendData("arduino", 15.15);

	  // promenna pro prijemm a odezvu



	//if (nRF.available()) {
	//	// cekani na prijem dat
	//	int prijem;
	//	while (nRF.available()) {
	//		// v pripade prijmu dat se provede zapis
	//		// do promenne prijem
	//		nRF.read(&prijem, sizeof(prijem));
	//		Serial.println(prijem);
	//	}
	//}
}

//odesle hodnotu senzoru do API
void sendData(String identificator, float value)
{
	// pripravim parametry adresy pro API
	String addr = apiAddress;
	addr.replace("#ID", identificator);
	addr.replace("#VAL", String(value));

	Serial.println("Pripojuji k " + String(apiServerHostname));


	if (client.connect(apiServerHostname, apiServerPort))
	{
		// poslu GET
		Serial.println(F("Pripojeno!"));

		client.println("GET " + addr + " HTTP/1.1");
		client.println("Host: " + String(apiServerHostname));
		client.println(F("Content-Type: application/x-www-form-urlencoded"));
		client.println(F("Connection: close"));
		client.println();
		client.println();
		client.stop();

		Serial.println(F("Odeslano!"));
	}

	else 
	{
		Serial.println(F("Spojeni se nepovedlo."));
	}
}


float getTemp() {
	//returns the temperature from one DS18S20 in DEG Celsius

	byte data[12];
	byte addr[8];

	if (!ds.search(addr)) {
		//no more sensors on chain, reset search
		ds.reset_search();
		return -1000;
	}

	if (OneWire::crc8(addr, 7) != addr[7]) {
		Serial.println(F("CRC is not valid!"));
		return -1000;
	}

	if (addr[0] != 0x10 && addr[0] != 0x28) {
		Serial.print(F("Device is not recognized"));
		return -1000;
	}

	ds.reset();
	ds.select(addr);
	ds.write(0x44, 1); // start conversion, with parasite power on at the end

	byte present = ds.reset();
	ds.select(addr);
	ds.write(0xBE); // Read Scratchpad


	for (int i = 0; i < 9; i++) { // we need 9 bytes
		data[i] = ds.read();
	}

	ds.reset_search();

	byte MSB = data[1];
	byte LSB = data[0];

	float tempRead = ((MSB << 8) | LSB); //using two's compliment
	float TemperatureSum = tempRead / 16;

	return TemperatureSum;

}




