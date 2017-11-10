
#define DEV_MODE false


/**
 ** HERE ALL THE WIRING AND INCLUDES 
 **/

#include <EEPROM.h>
#include <PrintEx.h>

#include <OneWire.h>
#include <DallasTemperature.h>

// DALLAS OUTSIDE TEMPERATURE
int cfg_pinONEWIRE = 3;
OneWire oneWire(cfg_pinONEWIRE);
DallasTemperature sensors(&oneWire);

// DHT INTERNAL TEMPERATURE
#include <SimpleDHT.h>
int cfg_pinDHT22 = 2;
SimpleDHT22 dht22;

// DISPLAY
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,16,2);

// VOLTAGE DIVIDER / METER
int cfg_pinVoltage = A0;
float cfg_vRef =5.015;
float cfg_volR1 = 180000;
float cfg_volR2 =  60000;
float cfg_volCorrection = 0.965;
float voltageMultiplier = cfg_volCorrection * (cfg_vRef * (cfg_volR2 + cfg_volR1)) / (1024 * cfg_volR2) ; 

// BUTTONS
#include <ButtonDebounce.h>
int cfg_pinButtonMenu = 8;
int cfg_pinButtonChange = 9;
ButtonDebounce buttonMenu(cfg_pinButtonMenu, 100);
ButtonDebounce buttonChange(cfg_pinButtonChange, 100);

// LIGHT AND SENSOR
int cfg_pinDoor = 10;
int cfg_pinLight = 11;

// RELAYS PELTIER / FAN
int cfg_pinPeltier = 4;
int cfg_pinFan = 5;
bool peltier = false;
bool fan = false;
char lcd_status[] = "OFF";

/**
 * READING FROM SENSORS
 */
float int_temp = 0;
float int_humi = 0;
float ext_temp = 0;
float bat_volt = 0;

void refreshAllSensors() {
  if (dht22.read2(cfg_pinDHT22, &int_temp, &int_humi, NULL) == SimpleDHTErrSuccess) {
    //XXX: read failed, maybe do something clever
    }

  sensors.requestTemperatures();
  // because sensors.setWaitForConversion(false) (in setup) the value read below
  // will be the value sensed a second or so ago, which is good enough an non-blocking
  ext_temp = sensors.getTempCByIndex(0);
  
  float voltageRaw = analogRead(cfg_pinVoltage) ;
  bat_volt = voltageRaw * voltageMultiplier ;
}

/**
 ** MENUES AND GUI 
 **/
#define GM_STATUS   0
#define GM_RUN_MODE 1
#define GM_CUT_OFF  2
#define GM_LAST     3
#define GM_FIRST    GM_STATUS

#define RM_TARGET_5   0
#define RM_TARGET_8   1
#define RM_25C_50F    2
#define RM_50C_75F    3
#define RM_75C_100F   4
#define RM_ALWAYS_ON  5
#define RM_ECO_MODE   6
#define RM_LAST       7
#define RM_FIRST      RM_TARGET_5

#define CO_ALWAYS_ON  0
#define CO_BATT_OK    1  // 12.5v
#define CO_SAFE_START 2  // 13.0v
#define CO_ENGINE_OFF 3  // 13.7v 
#define CO_LAST       4
#define CO_FIRST      CO_ALWAYS_ON

byte gui_menu = 0;
byte gui_run_mode = 0;
byte gui_cut_off = 0;

long lastUIUsed = millis(); // millis overflow after 50 days
long cfg_lcdOffAfter = 1000*30;

void updateLcdBacklight_Loop() {
    if ((lastUIUsed + cfg_lcdOffAfter) < millis()) {
      lcd.noBacklight();
      gui_menu = GM_STATUS;
    }
}
bool updateLcdBacklight_UsedUI() {
    bool backlight_was_off = (lastUIUsed + cfg_lcdOffAfter) < millis();
    lastUIUsed = millis();
    if (backlight_was_off) lcd.backlight();
    return backlight_was_off;
}

void buttonMenuToggle(int state) {
  if (updateLcdBacklight_UsedUI() || state==1) return;
  if (++gui_menu==GM_LAST) gui_menu = GM_FIRST;
}

void buttonChangeToggle(int state) {
  if (updateLcdBacklight_UsedUI() || state==1) return;
  if (gui_menu == GM_RUN_MODE && ++gui_run_mode==RM_LAST) gui_run_mode = RM_FIRST;
  if (gui_menu == GM_CUT_OFF  && ++gui_cut_off==CO_LAST) gui_cut_off = CO_FIRST;
}

void displayStatusPage() {
  char buf[30];
  GString g(buf); // maybe something different
  g.printf("%2.1fC %2.1fRH%%   ", int_temp, int_humi);
  lcd.setCursor(0,0); lcd.print(g);

  g = GString(buf);
  g.printf("%2.1fC %2.2fV %s ", ext_temp, bat_volt, lcd_status);
  lcd.setCursor(0,1); lcd.print(g);
}

void displayRunModePage() {
  lcd.setCursor(0,0);  lcd.print("    Run Mode    ");
  lcd.setCursor(0,1); 
  switch (gui_run_mode) {
    case RM_TARGET_5:  lcd.print("    Target 5C   "); break;
    case RM_TARGET_8:  lcd.print("    Target 8C   "); break;
    case RM_25C_50F:   lcd.print("25% Cou  50% Fan"); break;
    case RM_50C_75F:   lcd.print("50% Cou  75% Fan"); break;
    case RM_75C_100F:  lcd.print("75% Cou 100% Fan"); break;
    case RM_ALWAYS_ON: lcd.print("    Always On   "); break;
    case RM_ECO_MODE:  lcd.print("       Eco      "); break;
  }
}

void displayAutoCutOffPage() {
  lcd.setCursor(0,0);   lcd.print("  Auto Cut Off  ");
  lcd.setCursor(0,1); 
  switch (gui_cut_off) {
    case CO_ALWAYS_ON:  lcd.print("   Always Run   "); break;
    case CO_BATT_OK:    lcd.print("12.5V Battery Ok"); break;
    case CO_SAFE_START: lcd.print("13.0V Safe Start"); break;
    case CO_ENGINE_OFF: lcd.print("13.8V Engine Off"); break;
  }
}

void displayGui() {
  switch(gui_menu) {
    case GM_STATUS: displayStatusPage(); break;
    case GM_RUN_MODE: displayRunModePage(); break;
    case GM_CUT_OFF: displayAutoCutOffPage(); break;
  }
}

/**
 * DOOR/LIGHT
 */

int lightSpeed = 2;
int lightIntensity = 0;
void doDoorLight() {
  bool door = digitalRead(cfg_pinDoor);
  if (door) {
    if (lightIntensity < 250) lightIntensity+=lightSpeed;
    analogWrite(cfg_pinLight, lightIntensity);
  } else {
    lightIntensity = 0;
  }
  analogWrite(cfg_pinLight, lightIntensity);
}

/**
 * PELTIER, FAN & Logic
 */
int workCycle = 0;
void doWorkingLogic() {
  if (++workCycle>100) workCycle = 0; // each tick=5sec (as per main loop), therefore: 5*100/60=8m
  
  fan = false;
  peltier = false;
  
  if (  (gui_cut_off == CO_BATT_OK && bat_volt < 12.5)
     || (gui_cut_off == CO_SAFE_START && bat_volt < 13.0)
     || (gui_cut_off == CO_ENGINE_OFF && bat_volt < 13.7) ) {
      fan = false; peltier = false; strncpy(lcd_status, "BAT", 4); 
      return;
  }

  if (gui_run_mode == RM_ALWAYS_ON) {
      fan = true; peltier = true; 
  }
  if (gui_run_mode == RM_TARGET_5) {
    if (int_temp > 5.5) {
      fan = true; peltier = true; 
    } 
    if (int_temp < 5.0) {
      fan = false; peltier = false; 
    }
    // margin
  }
  if (gui_run_mode == RM_TARGET_8) {
    if (int_temp > 8.5) {
      fan = true; peltier = true; 
    } 
    if (int_temp < 8.0) {
      fan = false; peltier = false; 
    }
    // margin
  }
  if (gui_run_mode == RM_25C_50F) {
    peltier = workCycle > 0 && workCycle < 25;
    fan = workCycle > 0 && workCycle < 50;
  } 
  if (gui_run_mode == RM_50C_75F) {
    peltier = workCycle > 0 && workCycle < 50;
    fan = workCycle > 0 && workCycle < 75;
  } 
  if (gui_run_mode == RM_75C_100F) {
    peltier = workCycle > 0 && workCycle < 75;
    fan = true;
  }

  if (gui_run_mode == RM_ECO_MODE) {
    if (int_temp < 9) {
      peltier = false; fan = false;
    }
    if (int_temp > 10) {
      peltier = workCycle > 0 && workCycle < (25 + int_temp*2);
      fan = workCycle > 0 && workCycle < (40 + int_temp*2);
    }
  }
        
  if (peltier && fan) strncpy(lcd_status, "RUN", 4);
  else if (!peltier && fan) strncpy(lcd_status, "FAN", 4); 
  else strncpy(lcd_status, "SLP", 4); 
}

void doPeltierFan() {
  if (DEV_MODE) {
    digitalWrite(cfg_pinPeltier, false);
    digitalWrite(cfg_pinFan, false);
  } else {
    digitalWrite(cfg_pinPeltier, peltier);
    digitalWrite(cfg_pinFan, fan);
  }
}

/**
 * EPROM
 */
void loadConfigFromEEPROM() {
  gui_run_mode = EEPROM[0];  // bytes should be enough
  if (gui_run_mode >= RM_LAST) gui_run_mode = RM_FIRST;
  gui_cut_off = EEPROM[1];
  if (gui_cut_off >= CO_LAST) gui_cut_off = CO_FIRST;
}

void updateConfigToEEPROM() {
  EEPROM.update(0, gui_run_mode);  //updates does not writes if not necessary
  EEPROM.update(1, gui_cut_off);
}

/**
 ** SETUP AND MAIN LOOP 
 **/
void setup() {
  delay(1000);
  Serial.begin(115200);
  loadConfigFromEEPROM();
  
  pinMode(cfg_pinVoltage, INPUT);

  pinMode(cfg_pinDoor, INPUT_PULLUP);
  pinMode(cfg_pinLight, OUTPUT);

  pinMode(cfg_pinPeltier, OUTPUT);
  pinMode(cfg_pinFan, OUTPUT);
  
  pinMode(cfg_pinButtonMenu, INPUT_PULLUP);
  pinMode(cfg_pinButtonChange, INPUT_PULLUP);
  buttonMenu.setCallback(buttonMenuToggle);
  buttonChange.setCallback(buttonChangeToggle);
  
  sensors.begin();
  sensors.setWaitForConversion(false);

  lcd.init();
  lcd.backlight();
}

long last_slow_loop = millis() - 5000;
void loop() {
  buttonMenu.update();
  buttonChange.update();
  updateLcdBacklight_Loop();

  if (millis() - last_slow_loop > 5000) {
    refreshAllSensors();
    updateConfigToEEPROM();
    
    doWorkingLogic();
    doPeltierFan();

    last_slow_loop = millis();    
  }

  doDoorLight();
  displayGui();

  delay(10);
}
