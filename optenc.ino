// include the library code:
#include <Encoder.h>
#include <LiquidCrystal.h>
#include "WearLevelling.h"

//pin connections
#define ENC_A 2
#define ENC_B 3
#define LCD_RS A1
#define LCD_EN A0
#define LCD_D4 10
#define LCD_D5 11
#define LCD_D6 12
#define LCD_D7 13
#define BTN_RESET 9

//how may millimeters of filament per encoder revolution
#define MM_PER_REV 66.0f

//how many encoder ticks per revolution
//depends on your encoer
#define TICKS_PER_REV 80.0f

//eeprom saving ticks interval
#define TICKS_BETWEEN_EEPROM_SAVE 80
//save anyway to eeprom after inactivity
#define IDLE_TIME_EEPROM_SAVE_MS 60000

#define MM_PER_TICK			MM_PER_REV/TICKS_PER_REV
#define METERS_PER_TICK		MM_PER_TICK/1000.0f

struct MyEEPROMData
{
	long a = 0;
	long b = 0;
};

long ticks_at_last_save = 0;
long curr_enc_count = 0;
long initial_tot_len_ticks = 0;
long initial_part_len_ticks = 0;
long tot_len_ticks = 0;
long part_len_ticks = 0;
unsigned long timer_display = 0;
unsigned long last_save_millis = 0;

LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
Encoder myEnc(ENC_A, ENC_B);
E2WearLevelling WL;
MyEEPROMData myData;

void setup() 
{
	lcd.begin(16, 2);
	Serial.begin(115200);
	pinMode(BTN_RESET, INPUT_PULLUP);
	delay(100);

	lcd.print(F("Filameter 1.0"));
	WL.init();
	delay(1000);

	if (!digitalRead(BTN_RESET))
	{
		lcd.print(F("TOTAL RESET?"));
		lcd.setCursor(0, 1);
		lcd.print(F("Confirm?"));
		
		delay(500);
		while (!digitalRead(BTN_RESET)) { ; }
		delay(500);
		while (digitalRead(BTN_RESET)) { ; }
		delay(500);
		
		lcd.setCursor(0, 1);
		lcd.print(F("Erasing..."));

		tot_len_ticks = 0;
		part_len_ticks = 0;
		myEnc.write(0);
		WL.format();

		myData.a = 0;
		myData.b = 0;
		WL.writeToEEPROM(&myData, sizeof(MyEEPROMData));
				
		lcd.setCursor(0, 1);
		lcd.print(F("Done.     "));

		delay(1000);
		lcd.clear();
	}

	lcd.setCursor(0, 2);
	lcd.print(F("Resuming..."));
	delay(1000);
	if (WL.readFromEEPROM(&myData, sizeof(MyEEPROMData)))
	{
		initial_tot_len_ticks = myData.a;
		initial_part_len_ticks = myData.b;
	}
	else
	{
		lcd.setCursor(0, 1);
		lcd.print(F("EEPROM read error"));
		lcd.setCursor(0, 2);
		lcd.print(F("please reset!"));
		while (1) {}
	}

	lcd.clear();
}

char lcd_buf[17];
char float_buf[17];
char spaces[17];

void loop()
{
	curr_enc_count = myEnc.read();

	tot_len_ticks = initial_tot_len_ticks + curr_enc_count;
	part_len_ticks = initial_part_len_ticks + curr_enc_count;

	if ((millis() - timer_display)>500)
	{
		timer_display = millis();

		lcd.setCursor(0, 0);
		dtostrf((float)(tot_len_ticks)*METERS_PER_TICK, 5, 3, float_buf);

		int i;
		for (i = 0; i < 16 - 4 - strlen(float_buf);i++) spaces[i] = ' ';
		spaces[i+1] = 0;

		sprintf(lcd_buf, "T[m]%s%s", spaces, float_buf);
		lcd.print(lcd_buf);

		lcd.setCursor(0, 1);
		dtostrf((float)(part_len_ticks)*METERS_PER_TICK, 5, 3, float_buf);

		for (i = 0; i < 16 - 4 - strlen(float_buf); i++) spaces[i] = ' ';
		spaces[i + 1] = 0;

		sprintf(lcd_buf, "P[m]%s%s", spaces, float_buf);
		lcd.print(lcd_buf);

		Serial.println(tot_len_ticks);

		if (!digitalRead(BTN_RESET))
		{
			initial_part_len_ticks = 0;
			initial_tot_len_ticks += curr_enc_count;
			part_len_ticks = 0;
			myEnc.write(0);

			myData.a = tot_len_ticks;
			myData.b = part_len_ticks;
			if (!WL.writeToEEPROM(&myData, sizeof(MyEEPROMData)))
			{
				lcd.clear();
				lcd.setCursor(0, 0);
				lcd.write(F("EEPROM write error!"));
			}
		}
	}

	long ticks_left_to_save = abs(curr_enc_count - ticks_at_last_save);
	int save_req = (ticks_left_to_save > TICKS_BETWEEN_EEPROM_SAVE) || ((ticks_left_to_save > 0) && (millis() - last_save_millis) > IDLE_TIME_EEPROM_SAVE_MS);
	if (save_req)
	{
		last_save_millis = millis();
		ticks_at_last_save = curr_enc_count;
	
		myData.a = tot_len_ticks;
		myData.b = part_len_ticks;
		if (!WL.writeToEEPROM(&myData, sizeof(MyEEPROMData)))
		{
			lcd.clear();
			lcd.setCursor(0, 0);
			lcd.write(F("EEPROM write error!"));
		}
	}
}
