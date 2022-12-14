#ifndef SerialCommandLine_h
#define SerialCommandLine_h

#include <Arduino.h>
#include "configs.h"
#include "WiFiScan.h"
#include "SDInterface.h"
#include "settings.h"

extern WiFiScan wifi_scan_obj;
extern SDInterface sd_obj;
extern Settings settings_obj;
extern LinkedList<AccessPoint>* access_points;
extern LinkedList<ssid>* ssids;
extern const String PROGMEM version_number;

//// Commands

// Admin
const char PROGMEM CH_CMD[] = "channel";
const char PROGMEM CLEARAP_CMD[] = "clear";
const char PROGMEM REBOOT_CMD[] = "reboot";
const char PROGMEM HELP_CMD[] = "help";
const char PROGMEM STATUS_CMD[] = "status";
const char PROGMEM SETTINGS_CMD[] = "settings";

// WiFi sniff/scan
const char PROGMEM SCANAP_CMD[] = "scan";
const char PROGMEM SNIFF_CMD[] = "sniff";

const char PROGMEM SNIFF_TYPE_BEACON[] = "-beacon";
const char PROGMEM SNIFF_TYPE_PROBE[] = "-probe";
const char PROGMEM SNIFF_TYPE_PWN[] = "-pwn";
const char PROGMEM SNIFF_TYPE_ESP[] = "-esp";
const char PROGMEM SNIFF_TYPE_DEAUTH[] = "-deauth";
const char PROGMEM SNIFF_TYPE_PMKID[] = "-pmkid";
const char PROGMEM SNIFF_TYPE_BT[] = "-bt";
const char PROGMEM SNIFF_TYPE_BT_SKIM[] = "-skim";

const char PROGMEM SNIFF_BEACON_CMD[] = "sniffbeacon";
const char PROGMEM SNIFF_PROBE_CMD[] = "sniffprobe";
const char PROGMEM SNIFF_PWN_CMD[] = "sniffpwn";
const char PROGMEM SNIFF_ESP_CMD[] = "sniffesp";
const char PROGMEM SNIFF_DEAUTH_CMD[] = "sniffdeauth";
const char PROGMEM SNIFF_PMKID_CMD[] = "sniffpmkid";

const char PROGMEM STOPSCAN_CMD[] = "stop";

// WiFi attack
const char PROGMEM ATTACK_CMD[] = "attack";
const char PROGMEM ATTACK_TYPE_DEAUTH[] = "-deauth";
const char PROGMEM ATTACK_TYPE_BEACON[] = "-beacon";
const char PROGMEM ATTACK_TYPE_PROBE[] = "-probe";
const char PROGMEM ATTACK_TYPE_RR[] = "-rickroll";

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
"--------------- Commands ---------------\n"
"channel [-set <channel>]\n"
"clear [-ap/-ssid]\n"
"reboot\n"
"settings [-set <setting> true/false>]/[-restore]/[-json]\n"
"scan\n"
"status\n"
"sniff [-beacon/-probe/-pwn/-esp/-deauth/-pmkid [-channel <channel>/-frames]/-bt/-skim]\n"
"stop\n"
"attack -t <beacon [-l/-r/-a]/deauth/probe/rickroll>\n"
"list [-ap/-ssid]\n"
"select [-ap <index (comma separated)/all>]/[-ssid <index (comma separated)/all>]\n"
"ssid [-add <name>/-generate <count>/-remove <index>]\n"
"----------------------------------------\n";

const char PROGMEM ASCII_ART[] =
"                                        \n"
"                                        \n"
"         @@@                 @@@        \n"
"       @@   @@             @@   @@      \n"
"       @@     @@         @@     @@      \n"
"       @@       @@@@@@@@@       @@      \n"
"       @@   @@@@         @@@@   @@      \n"
"       @@@@@                 @@@@@      \n"
"       @@                       @@      \n"
"     @@     @@  @@@@@@@@@  @@     @@    \n"
"     @@           @@@@@           @@    \n"
"   @@                               @@  \n"
"     @@@@                       @@@@    \n"
"         @@@@@@@@@@@@@@@@@@@@@@@        \n"
"                                        ";

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