#ifndef SDInterface_h
#define SDInterface_h

#include "configs.h"
#include "SD.h"

#ifdef KIT
#define SD_DET 4
#endif

#ifndef SD_CS
#define SD_CS 10
#endif

#ifdef BRD_32CAM
#include "SD_MMC.h"

#define WITHOUT_FLASH_LED
#define SD_CS 2
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#endif

#include "Buffer.h"
#include <Update.h>

extern Buffer buffer_obj;

class SDInterface
{

private:
	bool checkDetectPin();

public:
	uint8_t cardType;
	uint64_t cardSizeMB;

	bool supported = false;
	bool do_save = true;

	String card_sz;

	bool initSD();

	void addPacket(uint8_t *buf, uint32_t len);
	void openCapture(String file_name = "");
	void runUpdate();
	void performUpdate(Stream &updateSource, size_t updateSize);
	void main();
};

#endif
