#include "esp_interface.h"

HardwareSerial MySerial(1);

void EspInterface::begin()
{
	pinMode(ESP_RST, OUTPUT);
	pinMode(ESP_ZERO, OUTPUT);

	delay(100);

	digitalWrite(ESP_ZERO, HIGH);

	Serial.println("Checking for ESP8266...");

	MySerial.begin(BAUD, SERIAL_8N1, 27, 26);

	delay(100);

	this->bootRunMode();

	delay(500);

	while (MySerial.available())
		MySerial.read();

	MySerial.write("PING");

	delay(2000);

	String display_string = "";

	while (MySerial.available())
	{
		display_string.concat((char)MySerial.read());
	}

	display_string.trim();

	Serial.println("\nDisplay string: " + (String)display_string);

	if (display_string == "ESP8266 Pong")
	{
		Serial.println("ESP8266 Found!");
		this->supported = true;
	}

	this->initTime = millis();
}

void EspInterface::RunUpdate()
{
	this->bootProgramMode();
}

void EspInterface::bootProgramMode()
{
	Serial.println("[!] Setting ESP12 in program mode...");
	digitalWrite(ESP_ZERO, LOW);
	delay(100);
	digitalWrite(ESP_RST, LOW);
	delay(100);
	digitalWrite(ESP_RST, HIGH);
	delay(100);
	digitalWrite(ESP_ZERO, HIGH);
	Serial.println("[!] Complete");
	Serial.end();
	Serial.begin(57600);
}

void EspInterface::bootRunMode()
{
	Serial.end();
	Serial.begin(115200);
	Serial.println("[!] Setting ESP12 in run mode...");
	digitalWrite(ESP_ZERO, HIGH);
	delay(100);
	digitalWrite(ESP_RST, LOW);
	delay(100);
	digitalWrite(ESP_RST, HIGH);
	delay(100);
	digitalWrite(ESP_ZERO, HIGH);
	Serial.println("[!] Complete");
}

void EspInterface::program()
{
	if (MySerial.available())
	{
		Serial.write((uint8_t)MySerial.read());
	}

	if (Serial.available())
	{
		while (Serial.available())
		{
			MySerial.write((uint8_t)Serial.read());
		}
	}
}

void EspInterface::main(uint32_t current_time)
{
	if (current_time - this->initTime >= 1000)
	{
		this->initTime = millis();
	}

	while (MySerial.available())
	{
		Serial.print((char)MySerial.read());
	}

	if (Serial.available())
	{
		MySerial.write((uint8_t)Serial.read());
	}
}
