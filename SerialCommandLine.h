#ifndef SerialCommandLine_h
#define SerialCommandLine_h

#include <Arduino.h>
#include "configs.h"
#include "WiFiScan.h"
#include "Web.h"
#include "SDInterface.h"
#include "settings.h"

extern WiFiScan wifi_scan_obj;
extern Web web_obj;
extern SDInterface sd_obj;
extern Settings settings_obj;
extern LinkedList<AccessPoint>* access_points;
extern LinkedList<ssid>* ssids;
extern const String PROGMEM version_number;

//// Commands

// Admin
const char PROGMEM CH_CMD[] = "channel";
const char PROGMEM CLEARAP_CMD[] = "clearlist";
const char PROGMEM REBOOT_CMD[] = "reboot";
const char PROGMEM UPDATE_CMD[] = "update";
const char PROGMEM HELP_CMD[] = "help";
const char PROGMEM STATUS_CMD[] = "status";
const char PROGMEM SETTINGS_CMD[] = "settings";

// WiFi sniff/scan
const char PROGMEM SCANAP_CMD[] = "scanap";
const char PROGMEM SNIFF_BEACON_CMD[] = "sniffbeacon";
const char PROGMEM SNIFF_PROBE_CMD[] = "sniffprobe";
const char PROGMEM SNIFF_PWN_CMD[] = "sniffpwn";
const char PROGMEM SNIFF_ESP_CMD[] = "sniffesp";
const char PROGMEM SNIFF_DEAUTH_CMD[] = "sniffdeauth";
const char PROGMEM SNIFF_PMKID_CMD[] = "sniffpmkid";
const char PROGMEM STOPSCAN_CMD[] = "stopscan";

// WiFi attack
const char PROGMEM ATTACK_CMD[] = "attack";
const char PROGMEM ATTACK_TYPE_DEAUTH[] = "deauth";
const char PROGMEM ATTACK_TYPE_BEACON[] = "beacon";
const char PROGMEM ATTACK_TYPE_PROBE[] = "probe";
const char PROGMEM ATTACK_TYPE_RR[] = "rickroll";

// WiFi Aux
const char PROGMEM LIST_AP_CMD[] = "list";
const char PROGMEM SEL_CMD[] = "select";
const char PROGMEM SSID_CMD[] = "ssid";

// Bluetooth sniff/scan
const char PROGMEM BT_SNIFF_CMD[] = "sniffbt";
const char PROGMEM BT_SKIM_CMD[] = "sniffskim";

//// Command help messages
// Admin
const char PROGMEM HELP_MESSAGE[] = 
	"----- Commands -----\n"
	"channel [-s <channel>]\n"
	"clearlist -a\n"
	"clearlist -s\n"
	"reboot\n"
	"update -s\n"
	"update -w\n"
	"settings [-s <setting> enable/disable>]/[-r]\n"
	"scanap\n"
	"scanap -f\n"
	"status\n"
	"sniffbeacon\n"
	"sniffprobe\n"
	"sniffpwn\n"
	"sniffesp\n"
	"sniffdeauth\n"
	"sniffpmkid [-c <channel>]\n"
	"stopscan\n"
	"attack -t <beacon [-l/-r/-a]/deauth/probe/rickroll>\n"
	"list -s\n"
	"list -a\n"
	"select -a <index (comma separated)>\n"
	"select -s <index (comma separated)>\n"
	"ssid -a [-g <count>/-n <name>]\n"
	"ssid -r <index>\n"
	"sniffbt\n"
	"sniffskim\n"
	"--------------------\n";

class SerialCommandLine
{
private:
	String getSerialInput();
	LinkedList<String> parseCommand(String input, char *delim);
	int argSearch(LinkedList<String> *cmd_args, String key);
	bool checkValueExists(LinkedList<String> *cmd_args_list, int index);
	bool inRange(int max, int index);
	void runCommand(String input);
	bool apSelected();
    bool hasSSIDs();

public:
	SerialCommandLine();

	void RunSetup();
	void main(uint32_t currentTime);
};

#endif