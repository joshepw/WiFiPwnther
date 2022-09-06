#include "flipperLED.h"
#include "configs.h"

void flipperLED::RunSetup()
{
#ifdef SIMPLE_LED

	pinMode(SIMPLE_LED, OUTPUT);

	delay(50);
	digitalWrite(SIMPLE_LED, LOW);
	delay(500);
	digitalWrite(SIMPLE_LED, HIGH);
	delay(500);
	digitalWrite(SIMPLE_LED, LOW);
	delay(500);
	digitalWrite(SIMPLE_LED, HIGH);

#else

	pinMode(B_PIN, OUTPUT);
	pinMode(G_PIN, OUTPUT);
	pinMode(R_PIN, OUTPUT);

	delay(50);

	digitalWrite(B_PIN, LOW);
	delay(500);
	digitalWrite(B_PIN, HIGH);
	digitalWrite(G_PIN, LOW);
	delay(500);
	digitalWrite(G_PIN, HIGH);
	digitalWrite(R_PIN, LOW);
	delay(500);
	digitalWrite(R_PIN, HIGH);

#endif
}

void flipperLED::attackLED()
{
#ifdef SIMPLE_LED
	digitalWrite(SIMPLE_LED, LOW);
#else

	digitalWrite(B_PIN, HIGH);
	digitalWrite(G_PIN, HIGH);
	digitalWrite(R_PIN, HIGH);
	delay(10);
	digitalWrite(R_PIN, LOW);

#endif
}

void flipperLED::sniffLED()
{
#ifdef SIMPLE_LED
	digitalWrite(SIMPLE_LED, LOW);
#else

	digitalWrite(B_PIN, HIGH);
	digitalWrite(G_PIN, HIGH);
	digitalWrite(R_PIN, HIGH);
	delay(10);
	digitalWrite(B_PIN, LOW);

#endif
}

void flipperLED::offLED()
{
#ifdef SIMPLE_LED
	digitalWrite(SIMPLE_LED, HIGH);
#else

	digitalWrite(B_PIN, HIGH);
	digitalWrite(G_PIN, HIGH);
	digitalWrite(R_PIN, HIGH);

#endif
}
