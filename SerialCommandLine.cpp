#include "SerialCommandLine.h"

SerialCommandLine::SerialCommandLine()
{
}

void SerialCommandLine::RunSetup()
{
	Serial.println("--------------------");
	Serial.println("  Pwnther  " + version_number);
	Serial.println("--------------------");
	Serial.println("  Type 'help' for   ");
	Serial.println("  more information  ");
	Serial.println("--------------------");

	Serial.print("> ");
}

String SerialCommandLine::getSerialInput()
{
	String input = "";

	if (Serial.available() > 0)
	{
		input = Serial.readStringUntil('\n');
	}

	input.trim();
	return input;
}

void SerialCommandLine::main(uint32_t currentTime)
{
	String input = this->getSerialInput();

	this->runCommand(input);

	if (input != "")
	{
		Serial.print("> ");
	}
}

LinkedList<String> SerialCommandLine::parseCommand(String input, char *delim)
{
	LinkedList<String> cmd_args;

	if (input != "")
	{

		char fancy[input.length() + 1] = {};
		input.toCharArray(fancy, input.length() + 1);

		char *ptr = strtok(fancy, delim);

		while (ptr != NULL)
		{
			cmd_args.add(String(ptr));
			ptr = strtok(NULL, delim);
		}
	}

	return cmd_args;
}

int SerialCommandLine::argSearch(LinkedList<String> *cmd_args_list, String key)
{
	for (int i = 0; i < cmd_args_list->size(); i++)
	{
		if (cmd_args_list->get(i) == key)
		{
			return i;
		}
	}

	return -1;
}

bool SerialCommandLine::checkValueExists(LinkedList<String> *cmd_args_list, int index)
{
	if (index < cmd_args_list->size() - 1)
		return true;

	return false;
}

bool SerialCommandLine::inRange(int max, int index)
{
	if ((index >= 0) && (index < max))
		return true;

	return false;
}

void SerialCommandLine::runCommand(String input)
{
	if (input != "")
		Serial.println("#" + input);
	else
		return;

	LinkedList<String> cmd_args = this->parseCommand(input, " ");

	//// Admin commands

	// Help
	if (cmd_args.get(0) == HELP_CMD)
	{
		Serial.println(HELP_MESSAGE);
	}

	if (cmd_args.get(0) == STATUS_CMD)
	{
		Serial.println((String)wifi_scan_obj.currentScanMode);
	}

	// Stop Scan
	if (cmd_args.get(0) == STOPSCAN_CMD)
	{
		if (wifi_scan_obj.currentScanMode == OTA_UPDATE)
		{
			wifi_scan_obj.currentScanMode = WIFI_SCAN_OFF;
			WiFi.softAPdisconnect(true);
			web_obj.shutdownServer();
			return;
		}

		wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
		Serial.println("Stopping WiFi tran/recv");
	}
	// Channel command
	else if (cmd_args.get(0) == CH_CMD)
	{
		// Search for channel set arg
		int ch_set = this->argSearch(&cmd_args, "-s");

		if (cmd_args.size() == 1)
		{
			Serial.println("Current channel: " + (String)wifi_scan_obj.set_channel);
		}
		else if (ch_set != -1)
		{
			wifi_scan_obj.set_channel = cmd_args.get(ch_set + 1).toInt();
			wifi_scan_obj.changeChannel();
			Serial.println("Set channel: " + (String)wifi_scan_obj.set_channel);
		}
	}
	// Clear APs
	else if (cmd_args.get(0) == CLEARAP_CMD)
	{
		int ap_sw = this->argSearch(&cmd_args, "-a"); // APs
		int ss_sw = this->argSearch(&cmd_args, "-s"); // SSIDs

		if (ap_sw != -1)
			wifi_scan_obj.RunClearAPs();

		if (ss_sw != -1)
			wifi_scan_obj.RunClearSSIDs();
	}

	else if (cmd_args.get(0) == SETTINGS_CMD)
	{
		int ss_sw = this->argSearch(&cmd_args, "-s");	   // Set setting
		int re_sw = this->argSearch(&cmd_args, "-r");	   // Reset setting
		int en_sw = this->argSearch(&cmd_args, "enable");  // enable setting
		int da_sw = this->argSearch(&cmd_args, "disable"); // disable setting

		if (re_sw != -1)
		{
			settings_obj.createDefaultSettings(SPIFFS);
			return;
		}

		if (ss_sw == -1)
		{
			settings_obj.printJsonSettings(settings_obj.getSettingsString());
		}
		else
		{
			bool result = false;
			String setting_name = cmd_args.get(ss_sw + 1);
			if (en_sw != -1)
				result = settings_obj.saveSetting<bool>(setting_name, true);
			else if (da_sw != -1)
				result = settings_obj.saveSetting<bool>(setting_name, false);
			else
			{
				Serial.println("You did not properly enable/disable this setting.");
				return;
			}

			if (!result)
			{
				Serial.println("Could not successfully update setting \"" + setting_name + "\"");
				return;
			}
		}
	}

	else if (cmd_args.get(0) == REBOOT_CMD)
	{
		Serial.println("Rebooting...");
		ESP.restart();
	}

	//// WiFi/Bluetooth Scan/Attack commands
	if (!wifi_scan_obj.scanning())
	{

		// AP Scan
		if (cmd_args.get(0) == SCANAP_CMD)
		{
			int full_sw = this->argSearch(&cmd_args, "-c");

			if (full_sw == -1)
			{
				Serial.println("Starting AP scan. Stop with " + (String)STOPSCAN_CMD);
				wifi_scan_obj.StartScan(WIFI_SCAN_TARGET_AP);
			}
			else
			{
				Serial.println("Starting Full AP scan. Stop with " + (String)STOPSCAN_CMD);
				wifi_scan_obj.StartScan(WIFI_SCAN_TARGET_AP_FULL);
			}
		}
		// Beacon sniff
		else if (cmd_args.get(0) == SNIFF_BEACON_CMD)
		{
			Serial.println("Starting Beacon sniff. Stop with " + (String)STOPSCAN_CMD);
			wifi_scan_obj.StartScan(WIFI_SCAN_AP);
		}
		// Probe sniff
		else if (cmd_args.get(0) == SNIFF_PROBE_CMD)
		{
			Serial.println("Starting Probe sniff. Stop with " + (String)STOPSCAN_CMD);
			wifi_scan_obj.StartScan(WIFI_SCAN_PROBE);
		}
		// Deauth sniff
		else if (cmd_args.get(0) == SNIFF_DEAUTH_CMD)
		{
			Serial.println("Starting Deauth sniff. Stop with " + (String)STOPSCAN_CMD);
			wifi_scan_obj.StartScan(WIFI_SCAN_DEAUTH);
		}
		// Pwn sniff
		else if (cmd_args.get(0) == SNIFF_PWN_CMD)
		{
			Serial.println("Starting Pwnagotchi sniff. Stop with " + (String)STOPSCAN_CMD);
			wifi_scan_obj.StartScan(WIFI_SCAN_PWN);
		}
		// Espressif sniff
		else if (cmd_args.get(0) == SNIFF_ESP_CMD)
		{
			Serial.println("Starting Espressif device sniff. Stop with " + (String)STOPSCAN_CMD);
			wifi_scan_obj.StartScan(WIFI_SCAN_ESPRESSIF);
		}
		// PMKID sniff
		else if (cmd_args.get(0) == SNIFF_PMKID_CMD)
		{
			int ch_sw = this->argSearch(&cmd_args, "-c");
			int d_sw = this->argSearch(&cmd_args, "-d"); // Deauth for pmkid

			if (ch_sw != -1)
			{
				wifi_scan_obj.set_channel = cmd_args.get(ch_sw + 1).toInt();
				wifi_scan_obj.changeChannel();
				Serial.println("Set channel: " + (String)wifi_scan_obj.set_channel);
			}

			if (d_sw == -1)
			{
				Serial.println("Starting PMKID sniff on channel " + (String)wifi_scan_obj.set_channel + ". Stop with " + (String)STOPSCAN_CMD);
				wifi_scan_obj.StartScan(WIFI_SCAN_EAPOL);
			}
			else
			{
				Serial.println("Starting PMKID sniff with deauthentication on channel " + (String)wifi_scan_obj.set_channel + ". Stop with " + (String)STOPSCAN_CMD);
				wifi_scan_obj.StartScan(WIFI_SCAN_ACTIVE_EAPOL);
			}
		}

		//// WiFi attack commands
		// attack
		if (cmd_args.get(0) == ATTACK_CMD)
		{
			int attack_type_switch = this->argSearch(&cmd_args, "-t"); // Required
			int list_beacon_sw = this->argSearch(&cmd_args, "-l");
			int rand_beacon_sw = this->argSearch(&cmd_args, "-r");
			int ap_beacon_sw = this->argSearch(&cmd_args, "-a");

			if (attack_type_switch == -1)
			{
				Serial.println("You must specify an attack type");
				return;
			}
			else
			{
				String attack_type = cmd_args.get(attack_type_switch + 1);

				// Branch on attack type
				// Deauth
				if (attack_type == ATTACK_TYPE_DEAUTH)
				{
					if (!this->apSelected())
					{
						Serial.println("You don't have any targets selected. Use " + (String)SEL_CMD);
						return;
					}

					Serial.println("Starting Deauthentication attack. Stop with " + (String)STOPSCAN_CMD);
					wifi_scan_obj.StartScan(WIFI_ATTACK_DEAUTH);
				}
				// Beacon
				else if (attack_type == ATTACK_TYPE_BEACON)
				{
					// spam by list
					if (list_beacon_sw != -1)
					{
						if (!this->hasSSIDs())
						{
							Serial.println("You don't have any SSIDs in your list. Use " + (String)SSID_CMD);
							return;
						}

						Serial.println("Starting Beacon list spam. Stop with " + (String)STOPSCAN_CMD);
						wifi_scan_obj.StartScan(WIFI_ATTACK_BEACON_LIST);
					}
					// spam with random
					else if (rand_beacon_sw != -1)
					{
						Serial.println("Starting random Beacon spam. Stop with " + (String)STOPSCAN_CMD);
						wifi_scan_obj.StartScan(WIFI_ATTACK_BEACON_SPAM);
					}
					// Spam from AP list
					else if (ap_beacon_sw != -1)
					{
						if (!this->apSelected())
						{
							Serial.println("You don't have any targets selected. Use " + (String)SEL_CMD);
							return;
						}

						Serial.println("Starting Targeted AP Beacon spam. Stop with " + (String)STOPSCAN_CMD);
						wifi_scan_obj.StartScan(WIFI_ATTACK_AP_SPAM);
					}
					else
					{
						Serial.println("You did not specify a beacon attack type");
					}
				}
				else if (attack_type == ATTACK_TYPE_PROBE)
				{
					if (!this->apSelected())
					{
						Serial.println("You don't have any targets selected. Use " + (String)SEL_CMD);
						return;
					}
					Serial.println("Starting Probe spam. Stop with " + (String)STOPSCAN_CMD);

					wifi_scan_obj.StartScan(WIFI_ATTACK_AUTH);
				}
				else if (attack_type == ATTACK_TYPE_RR)
				{
					Serial.println("Starting Rick Roll Beacon spam. Stop with " + (String)STOPSCAN_CMD);
					wifi_scan_obj.StartScan(WIFI_ATTACK_RICK_ROLL);
				}
				else
				{
					Serial.println("Attack type not properly defined");
					return;
				}
			}
		}

		//// Bluetooth scan/attack commands
		// Bluetooth scan
		if (cmd_args.get(0) == BT_SNIFF_CMD)
		{
			Serial.println("Starting Bluetooth scan. Stop with " + (String)STOPSCAN_CMD);
			wifi_scan_obj.StartScan(BT_SCAN_ALL);
		}
		// Bluetooth CC Skimmer scan
		else if (cmd_args.get(0) == BT_SKIM_CMD)
		{
			Serial.println("Starting Bluetooth CC Skimmer scan. Stop with " + (String)STOPSCAN_CMD);
			wifi_scan_obj.StartScan(BT_SCAN_SKIMMERS);
		}

		// Update command
		if (cmd_args.get(0) == UPDATE_CMD)
		{
			int w_sw = this->argSearch(&cmd_args, "-w");  // Web update
			int sd_sw = this->argSearch(&cmd_args, "-s"); // SD Update

			// Update via OTA
			if (w_sw != -1)
			{
				Serial.println("Starting Marauder OTA Update. Stop with " + (String)STOPSCAN_CMD);
				wifi_scan_obj.currentScanMode = OTA_UPDATE;
				web_obj.setupOTAupdate();
			}
			// Update via SD
			else if (sd_sw != -1)
			{
				if (!sd_obj.supported)
				{
					Serial.println("SD card is not connected. Cannot perform SD Update");
					return;
				}
			}
		}
	}

	//// WiFi aux commands
	// List access points
	if (cmd_args.get(0) == LIST_AP_CMD)
	{
		int ap_sw = this->argSearch(&cmd_args, "-a");
		int ss_sw = this->argSearch(&cmd_args, "-s");

		// List APs
		if (ap_sw != -1)
		{
			for (int i = 0; i < access_points->size(); i++)
			{
				AccessPoint ap = access_points->get(i);
				Serial.printf("[%d] %s%s\n %02x:%02x:%02x:%02x:%02x:%02x %s %d Ch:%d\n", i, ap.essid.c_str(), access_points->get(i).selected ? " (+)" : "", ap.bssid[0], ap.bssid[1], ap.bssid[2], ap.bssid[3], ap.bssid[4], ap.bssid[5], wifi_scan_obj.getSignalStrength(ap.rssi), ap.rssi, ap.channel);
			}
		}
		// List SSIDs
		else if (ss_sw != -1)
		{
			for (int i = 0; i < ssids->size(); i++)
			{
				ssid station = ssids->get(i);
				Serial.printf("[%d] %s%s\n %02x:%02x:%02x:%02x:%02x:%02x\n", i, station.essid.c_str(), ssids->get(i).selected ? " (+)" : "", station.bssid[0], station.bssid[1], station.bssid[2], station.bssid[3], station.bssid[4], station.bssid[5]);
			}
		}
		else
		{
			if (access_points->size() > 0)
				Serial.println("--- APs ------------");

			for (int i = 0; i < access_points->size(); i++)
			{
				AccessPoint ap = access_points->get(i);
				Serial.printf("[%d] %s%s\n%02x:%02x:%02x:%02x:%02x:%02x %s %d Ch:%d\n", i, ap.essid.c_str(), access_points->get(i).selected ? " (+)" : "", ap.bssid[0], ap.bssid[1], ap.bssid[2], ap.bssid[3], ap.bssid[4], ap.bssid[5], wifi_scan_obj.getSignalStrength(ap.rssi), ap.rssi, ap.channel);
			}

			if (ssids->size() > 0)
				Serial.println("--- SSIDs ----------");

			for (int i = 0; i < ssids->size(); i++)
			{
				ssid station = ssids->get(i);
				Serial.printf("[%d] %s%s\n%02x:%02x:%02x:%02x:%02x:%02x\n", i, station.essid.c_str(), ssids->get(i).selected ? " (+)" : "", station.bssid[0], station.bssid[1], station.bssid[2], station.bssid[3], station.bssid[4], station.bssid[5]);
			}

			return;
		}
	}
	// Select access points or stations
	else if (cmd_args.get(0) == SEL_CMD)
	{
		// Get switches
		int ap_sw = this->argSearch(&cmd_args, "-a");
		int ss_sw = this->argSearch(&cmd_args, "-s");

		// select Access points
		if (ap_sw != -1)
		{
			// Get list of indices
			LinkedList<String> ap_index = this->parseCommand(cmd_args.get(ap_sw + 1), ",");

			// Select ALL APs
			if (cmd_args.get(ap_sw + 1) == "all")
			{
				for (int i = 0; i < access_points->size(); i++)
				{
					if (access_points->get(i).selected)
					{
						// Unselect "selected" ap
						AccessPoint new_ap = access_points->get(i);
						new_ap.selected = false;
						access_points->set(i, new_ap);
					}
					else
					{
						// Select "unselected" ap
						AccessPoint new_ap = access_points->get(i);
						new_ap.selected = true;
						access_points->set(i, new_ap);
					}
				}
			}
			// Select specific APs
			else
			{
				// Mark APs as selected
				for (int i = 0; i < ap_index.size(); i++)
				{
					int index = ap_index.get(i).toInt();
					if (!this->inRange(access_points->size(), index))
					{
						Serial.println("Index not in range: " + (String)index);
						continue;
					}
					if (access_points->get(index).selected)
					{
						// Unselect "selected" ap
						AccessPoint new_ap = access_points->get(index);
						new_ap.selected = false;
						access_points->set(index, new_ap);
					}
					else
					{
						// Select "unselected" ap
						AccessPoint new_ap = access_points->get(index);
						new_ap.selected = true;
						access_points->set(index, new_ap);
					}
				}
			}
		}
		// select ssids
		else if (ss_sw != -1)
		{
			// Get list of indices
			LinkedList<String> ss_index = this->parseCommand(cmd_args.get(ss_sw + 1), ",");

			// Mark APs as selected
			for (int i = 0; i < ss_index.size(); i++)
			{
				int index = ss_index.get(i).toInt();
				if (!this->inRange(ssids->size(), index))
				{
					Serial.println("Index not in range: " + (String)index);
					continue;
				}
				if (ssids->get(index).selected)
				{
					// Unselect "selected" ap
					ssid new_ssid = ssids->get(index);
					new_ssid.selected = false;
					ssids->set(index, new_ssid);
				}
				else
				{
					// Select "unselected" ap
					ssid new_ssid = ssids->get(index);
					new_ssid.selected = true;
					ssids->set(index, new_ssid);
				}
			}
		}
		else
		{
			Serial.println("You did not specify which list to select from");
			return;
		}
	}
	// SSID stuff
	else if (cmd_args.get(0) == SSID_CMD)
	{
		int add_sw = this->argSearch(&cmd_args, "-a");
		int gen_sw = this->argSearch(&cmd_args, "-g");
		int spc_sw = this->argSearch(&cmd_args, "-n");
		int rem_sw = this->argSearch(&cmd_args, "-r");

		// Add ssid
		if (add_sw != -1)
		{
			// Generate random
			if (gen_sw != -1)
			{
				int gen_count = cmd_args.get(gen_sw + 1).toInt();
				wifi_scan_obj.generateSSIDs(gen_count);
			}
			// Add specific
			else if (spc_sw != -1)
			{
				String essid = cmd_args.get(spc_sw + 1);
				wifi_scan_obj.addSSID(essid);
			}
			else
			{
				Serial.println("You did not specify how to add SSIDs");
			}
		}
		// Remove SSID
		else if (rem_sw != -1)
		{
			int index = cmd_args.get(rem_sw + 1).toInt();
			if (!this->inRange(ssids->size(), index))
			{
				Serial.println("Index not in range: " + (String)index);
				return;
			}
			ssids->remove(index);
		}
		else
		{
			Serial.println("You did not specify whether to add or remove SSIDs");
			return;
		}
	}
}

bool SerialCommandLine::apSelected()
{
	for (int i = 0; i < access_points->size(); i++)
	{
		if (access_points->get(i).selected)
			return true;
	}

	return false;
}

bool SerialCommandLine::hasSSIDs()
{
	if (ssids->size() == 0)
		return false;

	return true;
}