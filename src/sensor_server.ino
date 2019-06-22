#include <SPI.h>
#include <Ethernet.h>
#include "RF24.h" 


// API
IPAddress apiServerIP = IPAddress(192,168,1,24);
String apiServerHostname = "192.168.1.24";
int apiServerPort = 54000;
String apiAddress = "/API/Write?sensorIdentificator=#ID&value=#VAL";

// Ethernet
byte mac[] = { 0xD4, 0xAD, 0xBE, 0xEF, 0xFE, 0x7D };
IPAddress ip(192, 168, 1, 150);
EthernetClient client;


// Sensor server
// nastavení propojovacích pinù
#define CE 7
#define CS 8
// inicializace nRF s piny CE a CS
RF24 nRF(CE, CS);

byte adresaPrijimac[] = "sensorServer";
byte adresaVysilac[] = "sensorKotelna";

void setup()
{
	Serial.begin(57600);
	// inicalizace sítì
	Ethernet.begin(mac, ip);

	// zapnutí komunikace nRF modulu
	nRF.begin();

	// nastavení zapisovacího a ètecího kanálu
	//nRF.openWritingPipe(adresaPrijimac);
	nRF.openReadingPipe(1, adresaVysilac);
	// zaèátek pøíjmu dat
	nRF.startListening();
}

int i = 1;
void loop()
{
	//if (i++ % 10000 == 0)
	//	sendData("arduino", 15.15);

	  // promìnné pro pøíjem a odezvu
	int prijem;

	if (nRF.available()) {
		// èekání na pøíjem dat
		while (nRF.available()) {
			// v pøípadì pøíjmu dat se provede zápis
			// do promìnné prijem
			nRF.read(&prijem, sizeof(prijem));
			Serial.println(prijem);
		}
	}
}

void sendData(String identificator, float value)
{

	String addr = apiAddress;
	addr.replace("#ID", identificator);
	addr.replace("#VAL", String(value));

	Serial.println("Pripojuji k " + apiServerHostname);


	if (client.connect(apiServerIP, apiServerPort))
	{
		Serial.println("Pripojeno!");

		client.println("GET " + addr + " HTTP/1.1");
		client.println("Host: " + apiServerHostname);
		client.println("Content-Type: application/x-www-form-urlencoded");
		client.println("Connection: close");
		client.println();
		client.println();
		client.stop();
	}        

	else {
		Serial.println("Spojeni se nepovedlo.");
	}
}
