#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif

#ifdef ESP32
#include <WiFi.h>
#endif

#include <stdio.h>
#include <Arduino.h>
#include <Wire.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "Assets.h"
#include "WiFiScan.h"
#include "SDInterface.h"
#include "Web.h"
#include "Buffer.h"
#include "esp_interface.h"
#include "settings.h"
#include "SerialCommandLine.h"
#include "flipperLED.h"

const String PROGMEM version_number = APP_VERSION;
WiFiScan wifi_scan_obj;
SDInterface sd_obj;
Web web_obj;
Buffer buffer_obj;
EspInterface esp_obj;
Settings settings_obj;
SerialCommandLine serial_cli;
flipperLED flipper_led;
uint32_t current_time = 0;

void setup()
{
	Serial.begin(115200);
	Serial.println("ESP-IDF version is: " + String(esp_get_idf_version()));

	flipper_led.RunSetup();
	settings_obj.begin();
	wifi_scan_obj.RunSetup();

	if (!sd_obj.initSD())
		Serial.println(F("SD Card NOT Supported"));

	Serial.println("CLI Ready");
	serial_cli.RunSetup();
}

void loop()
{
	current_time = millis();

	if (wifi_scan_obj.currentScanMode != ESP_UPDATE)
	{
		serial_cli.main(current_time);
		wifi_scan_obj.main(current_time);
		sd_obj.main();
		settings_obj.main(current_time);

		if (wifi_scan_obj.currentScanMode == OTA_UPDATE)
			web_obj.main();

		delay(50);
	}
	else if (wifi_scan_obj.currentScanMode == ESP_UPDATE)
	{
		delay(1);
	}
}
