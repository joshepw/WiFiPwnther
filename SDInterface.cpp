#include "SDInterface.h"

bool SDInterface::initSD()
{
	digitalWrite(SD_CS, HIGH);

	String display_string = "";

#ifdef SD_DET
	pinMode(SD_DET, INPUT);
	if (digitalRead(SD_DET) == LOW)
	{
		Serial.println("SD Card Detect Pin Detected");
	}
	else
	{
		Serial.println("SD Card Detect Pin Not Detected");
		this->supported = false;
		return false;
	}
#endif

#ifdef BRD_32CAM
	if (!SD_MMC.begin())
	{
		Serial.println("Failed to mount SD Card");
		this->supported = false;
		return false;
	}

	this->cardType = SD_MMC.cardType();
	this->supported = this->cardType != CARD_NONE;

	if (!this->supported)
	{
		Serial.println("No MicroSD Card found");
		return false;
	}

	this->cardSizeMB = SD_MMC.cardSize() / (1024 * 1024);
	Serial.printf("SD Card Size: %lluMB\n", this->cardSizeMB);

	if (!SD_MMC.exists("/SCRIPTS"))
	{
		Serial.println("/SCRIPTS does not exist. Creating...");

		SD.mkdir("/SCRIPTS");
		Serial.println("/SCRIPTS created");
	}
#else
	if (!SD.begin(SD_CS))
	{
		Serial.println(F("Failed to mount SD Card"));
		this->supported = false;
		return false;
	}

	this->supported = true;
	this->cardType = SD.cardType();
	this->cardSizeMB = SD.cardSize() / (1024 * 1024);

	Serial.printf("SD Card Size: %lluMB\n", this->cardSizeMB);

	if (this->supported)
	{
		const int NUM_DIGITS = log10(this->cardSizeMB) + 1;

		char sz[NUM_DIGITS + 1];

		sz[NUM_DIGITS] = 0;
		for (size_t i = NUM_DIGITS; i--; this->cardSizeMB /= 10)
		{
			sz[i] = '0' + (this->cardSizeMB % 10);
			display_string.concat((String)sz[i]);
		}

		this->card_sz = sz;
	}

	buffer_obj = Buffer();

	if (!SD.exists("/SCRIPTS"))
	{
		Serial.println("/SCRIPTS does not exist. Creating...");

		SD.mkdir("/SCRIPTS");
		Serial.println("/SCRIPTS created");
	}

#endif

	return true;
}

void SDInterface::addPacket(uint8_t *buf, uint32_t len)
{
	if ((this->supported) && (this->do_save))
	{
		buffer_obj.addPacket(buf, len);
	}
}

void SDInterface::openCapture(String file_name)
{
	if (this->supported)
		buffer_obj.open(&SD, file_name);
}

void SDInterface::runUpdate()
{
#ifdef BRD_32CAM
	File updateBin = SD_MMC.open("/update.bin");
#else
	File updateBin = SD.open("/update.bin");
#endif

	if (updateBin)
	{
		if (updateBin.isDirectory())
		{
			Serial.println(F("Error, update.bin is not a file"));
			updateBin.close();
			return;
		}

		size_t updateSize = updateBin.size();

		if (updateSize > 0)
		{
			Serial.println(F("Try to start update"));
			this->performUpdate(updateBin, updateSize);
		}
		else
		{
			Serial.println(F("Error, file is empty"));
			return;
		}

		updateBin.close();
		Serial.println(F("rebooting..."));

		delay(1000);
		ESP.restart();
	}
	else
	{
		Serial.println(F("Could not load update.bin from sd root"));
	}
}

void SDInterface::performUpdate(Stream &updateSource, size_t updateSize)
{
	if (Update.begin(updateSize))
	{
		size_t written = Update.writeStream(updateSource);
		if (written == updateSize)
		{
			Serial.println("Written : " + String(written) + " successfully");
		}
		else
		{
			Serial.println("Written only : " + String(written) + "/" + String(updateSize) + ". Retry?");
		}
		if (Update.end())
		{
			Serial.println("OTA done!");
			if (Update.isFinished())
			{
				Serial.println(F("Update successfully completed. Rebooting."));
			}
			else
			{
				Serial.println("Update not finished? Something went wrong!");
			}
		}
		else
		{
			Serial.println("Error Occurred. Error #: " + String(Update.getError()));
		}
	}
	else
	{
		Serial.println("Not enough space to begin OTA");
	}
}

bool SDInterface::checkDetectPin()
{
#ifdef SD_DET
	if (digitalRead(SD_DET) == LOW)
		return true;
	else
		return false;
#endif

	return false;
}

void SDInterface::main()
{
	if ((this->supported) && (this->do_save))
	{
#ifdef BRD_32CAM
		buffer_obj.forceSave(&SD_MMC);
#else
		buffer_obj.forceSave(&SD);
#endif
	}
	else if (!this->supported)
	{
		if (checkDetectPin())
		{
			delay(100);
			this->initSD();
		}
	}
}
