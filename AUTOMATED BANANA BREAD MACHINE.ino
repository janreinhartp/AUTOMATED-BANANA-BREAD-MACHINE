// Declaration of Libraries
// LCD
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROMex.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);
#include <Debounce.h>
// Motor
#include "control.h"
// Encoder
#include <ClickEncoder.h>
// Timer 1 for encoder
#include <TimerOne.h>
// Save Function
#include <EEPROMex.h>
#include <AccelStepper.h>

byte enterChar[] = {
    B10000,
    B10000,
    B10100,
    B10110,
    B11111,
    B00110,
    B00100,
    B00000};

byte fastChar[] = {
    B00110,
    B01110,
    B00110,
    B00110,
    B01111,
    B00000,
    B00100,
    B01110};
byte slowChar[] = {
    B00011,
    B00111,
    B00011,
    B11011,
    B11011,
    B00000,
    B00100,
    B01110};

// Declaration of LCD Variables
const int numOfMainScreens = 3;
const int numOfSettingScreens = 7;
const int numOfTimers = 7;
const int numOfTestMenu = 8;
int currentScreen = 0;
int currentSettingScreen = 0;
int currentTestMenuScreen = 0;

String screens[numOfMainScreens][2] = {
    {"SETTINGS", "ENTER TO EDIT"},
    {"RUN AUTO", "ENTER TO RUN AUTO"},
    {"TEST MACHINE", "ENTER TO TEST"}};

String settings[numOfSettingScreens][2] = {
    {"MIXING LIQUID", "MIN"},
    {"GRINDING", "MIN"},
    {"DISPENSE FLOUR", "MIN"},
    {"SCREW DISPENSING", "SEC"},
    {"MIXING", "MIN"},
    {"TRANSFER TO EXTRUDE", "MIN"},
    {"SAVE SETTINGS", "ENTER TO SAVE"}};

String TestMenuScreen[numOfTestMenu] = {
    "GRINDER",
    "DISPENSER",
    "MIXER",
    "LINEAR ACTUATOR",
    "DISPENSE",
    "DENESTER",
    "CONVEYOR",
    "Back to Main Menu"};

double parametersTimer[numOfTimers] = {1, 1, 15, 1, 1, 20};
double parametersTimerMaxValue[numOfTimers] = {1200, 1200, 1200, 1200, 1200, 1200};

bool settingsFlag = false;
bool settingEditTimerFlag = false;
bool runAutoFlag = false;
bool testMenuFlag = false;
bool refreshScreen = false;

int GrindTimeAdd = 10;
int ScrewDispenseTimeAdd = 20;
int MixingTimeAdd = 30;
int MoveToExtrudeTimeAdd = 40;
int ValveOpenTimeAdd = 50;
int MixingTimeLiquidAdd = 60;
void saveSettings()
{
    EEPROM.writeDouble(MixingTimeLiquidAdd, parametersTimer[0]);
    EEPROM.writeDouble(GrindTimeAdd, parametersTimer[1]);
    EEPROM.writeDouble(ScrewDispenseTimeAdd, parametersTimer[2]);
    EEPROM.writeDouble(MixingTimeAdd, parametersTimer[3]);
    EEPROM.writeDouble(MoveToExtrudeTimeAdd, parametersTimer[4]);
    EEPROM.writeDouble(ValveOpenTimeAdd, parametersTimer[5]);
}

void loadSettings()
{
    parametersTimer[0] = EEPROM.readDouble(MixingTimeLiquidAdd);
    parametersTimer[1] = EEPROM.readDouble(GrindTimeAdd);
    parametersTimer[2] = EEPROM.readDouble(ScrewDispenseTimeAdd);
    parametersTimer[3] = EEPROM.readDouble(MixingTimeAdd);
    parametersTimer[4] = EEPROM.readDouble(MoveToExtrudeTimeAdd);
    parametersTimer[5] = EEPROM.readDouble(ValveOpenTimeAdd);
}

// Declaration of Variables
// Rotary Encoder Variables
boolean up = false;
boolean down = false;
boolean middle = false;
ClickEncoder *encoder;
int16_t last, value;
// Fast Scroll
bool fastScroll = false;
unsigned long previousMillis = 0;
const long interval = 1000;

// RELAY DECLARATION
Control Grinder(47, 100, 101);
Control Dispenser(33, 100, 101);
Control Mixer(45, 100, 101);
Control Linear(39, 100, 101);
Control Extruder(41, 100, 101);
Control Valve(37, 100, 101);
Control Denester(35, 100, 101);
Control Conveyor(43, 100, 101);
Control CloseTimer(100, 100, 101);
Control trayDetectTimer(100, 100, 101);
Control TimerMixingLiquid(100,100,100);

const int pinSen1 = A0;
const int pinSen2 = A1;
Debounce NozzleSen(pinSen1, 100, true);
Debounce TraySen(pinSen2, 100, true);

unsigned long previousMillis2 = 0;
const long interval2 = 200;
unsigned long currentMillis2 = 0;

bool SenStat1, SenStat2 = false;

void initSensors()
{
    pinMode(pinSen1, INPUT_PULLUP);
    pinMode(pinSen2, INPUT_PULLUP);
}

void ReadSensor()
{
    SenStat1 = NozzleSen.read();
    SenStat2 = TraySen.read();
}

char *secondsToHHMMSS(int total_seconds)
{
    int hours, minutes, seconds;

    hours = total_seconds / 3600;         // Divide by number of seconds in an hour
    total_seconds = total_seconds % 3600; // Get the remaining seconds
    minutes = total_seconds / 60;         // Divide by number of seconds in a minute
    seconds = total_seconds % 60;         // Get the remaining seconds

    // Format the output string
    static char hhmmss_str[7]; // 6 characters for HHMMSS + 1 for null terminator
    sprintf(hhmmss_str, "%02d%02d%02d", hours, minutes, seconds);
    return hhmmss_str;
}

void setTime()
{
    TimerMixingLiquid.setTimer(secondsToHHMMSS(parametersTimer[0] * 60));
    Grinder.setTimer(secondsToHHMMSS(parametersTimer[1] * 60));
    Dispenser.setTimer(secondsToHHMMSS(parametersTimer[2]*60));
    Valve.setTimer(secondsToHHMMSS(parametersTimer[3]));
    Mixer.setTimer(secondsToHHMMSS(parametersTimer[4] * 60));
    Linear.setTimer(secondsToHHMMSS(parametersTimer[5] * 60));
     //Extruder.setTimer(secondsToHHMMSS(parametersTimer[6]));
    // Denester.setTimer(secondsToHHMMSS(parametersTimer[8]));
    // Conveyor.setTimer(secondsToHHMMSS(parametersTimer[11]));
    CloseTimer.setTimer(secondsToHHMMSS(15));
    trayDetectTimer.setTimer(secondsToHHMMSS(5));
}
int runAutoStat = 0;
int extrudeStatus = 0;
bool DispenseTestRunFlag = false;

void StopAll()
{
    Grinder.stop();
    Grinder.relayOff();

    Dispenser.stop();
    Dispenser.relayOff();

    Mixer.stop();
    Mixer.relayOff();

    Linear.stop();
    Linear.relayOff();

    Extruder.stop();
    Extruder.relayOff();

    Valve.stop();
    Valve.relayOff();

    Denester.stop();
    Denester.relayOff();

    Conveyor.stop();
    Conveyor.relayOff();

    runAutoStat = 0;
    extrudeStatus = 0;
}

void stopVfd()
{
    Grinder.stop();
    Grinder.relayOff();
    Mixer.stop();
    Mixer.relayOff();
    Conveyor.stop();
    Conveyor.relayOff();
}

void runPour()
{
    Valve.run();
    if (Valve.isTimerCompleted() == true)
    {
        Extruder.relayOff();
    }
    else
    {
        Extruder.relayOn();
    }
}

void TestRun()
{
    if (DispenseTestRunFlag == true)
    {
        runPour();
        if (Valve.isTimerCompleted() == true)
        {
            DispenseTestRunFlag = false;
        }
    }
}

void RunAuto()
{
    ReadSensor();
    switch (runAutoStat)
    {
    case 1:
        RunMixLiquid();
        break;
    case 2:
        runGrinderandDispenser();
        break;
    case 3:
        RunMixer();
        break;
    case 4:
        MoveToExtruder();
        break;
    case 5:
        RunExtrude();
        break;
    default:
        break;
    }
}

void RunMixLiquid()
{
    TimerMixingLiquid.run();
    Mixer.relayOn();
    if (TimerMixingLiquid.isTimerCompleted() == true)
    {
        runAutoStat = 2;
        Grinder.start();
        Dispenser.start();
    }
}

void runGrinderandDispenser(){
    Grinder.run();
    Dispenser.run();
    Mixer.relayOn();
    if (Grinder.isTimerCompleted() == true && Dispenser.isTimerCompleted() == true)
    {
        runAutoStat = 3;
        Mixer.start();
    }
    
}

void RunMixer()
{
    Mixer.run();
    if (Mixer.isTimerCompleted() == true)
    {
        runAutoStat = 4;
        Linear.start();
    }
}

void MoveToExtruder()
{
    Linear.run();
    Mixer.relayOn();
    if (Linear.isTimerCompleted() == true)
    {
        Mixer.relayOff();
        runAutoStat = 5;
        extrudeStatus = 1;
        currentMillis2 = millis();
        previousMillis2 = currentMillis2;
    }
}

bool initialMoveExtruder = false;

void RunExtrude()
{
    switch (extrudeStatus)
    {
    case 1:
        dispenseTray();
        break;
    case 2:
        runTrayDetectTimer();
        break;
    case 3:
        if (initialMoveExtruder == true)
        {
            if (SenStat1 == false)
            {
                Conveyor.relayOn();
            }
            else
            {
                initialMoveExtruder = false;
            }
        }
        else
        {
            if (SenStat1 == false)
            {
                Conveyor.relayOff();
                Valve.start();

                extrudeStatus = 4;
            }
            else
            {
                Conveyor.relayOn();
            }
        }

        break;
    case 4:
        if (SenStat1 == false)
        {
            RunDispenseBatter();
        }
        else
        {
            Extruder.relayOff();
            extrudeStatus = 5;
            CloseTimer.start();
            initialMoveExtruder = true;
        }
        break;
    case 5:
        CloseValveTimer();
        break;
    default:
        break;
    }
}

void dispenseTray()
{
    currentMillis2 = millis();
    if (currentMillis2 - previousMillis2 >= interval2)
    {
      Denester.relayOff();
        delay(500);
        previousMillis2 = currentMillis2;
        extrudeStatus = 2;
        trayDetectTimer.start();
    }
    else
    {
        Denester.relayOn();
    }
}

void runTrayDetectTimer()
{
    trayDetectTimer.run();
    if (trayDetectTimer.isTimerCompleted() == true)
    {
        if (SenStat2 == true)
        {
            extrudeStatus = 3;
        }
        else
        {
            extrudeStatus = 1;
            currentMillis2 = millis();
            previousMillis2 = currentMillis2;
        }
    }
}

void RunDispenseBatter()
{
    Valve.run();
    Extruder.relayOn();
    if (Valve.isTimerCompleted() == true)
    {
        Extruder.relayOff();
        extrudeStatus = 5;
        CloseTimer.start();
        initialMoveExtruder = true;
    }
}

void CloseValveTimer()
{
    CloseTimer.run();
    if (CloseTimer.isTimerCompleted() == true)
    {
        extrudeStatus = 1;
        currentMillis2 = millis();
        previousMillis2 = currentMillis2;
    }
}

// Functions for Rotary Encoder
void timerIsr()
{
    encoder->service();
}

void readRotaryEncoder()
{
    value += encoder->getValue();

    if (value / 2 > last)
    {
        last = value / 2;
        down = true;
        delay(100);
    }
    else if (value / 2 < last)
    {
        last = value / 2;
        up = true;
        delay(100);
    }
}

void readButtonEncoder()
{
    ClickEncoder::Button b = encoder->getButton();
    if (b != ClickEncoder::Open)
    { // Open Bracket for Click
        switch (b)
        { // Open Bracket for Double Click
        case ClickEncoder::Clicked:

            middle = true;
            break;

        case ClickEncoder::DoubleClicked:
            refreshScreen = 1;
            if (settingsFlag)
            {
                if (fastScroll == false)
                {
                    fastScroll = true;
                }
                else
                {
                    fastScroll = false;
                }
            }
            break;
        }
    }
}

void inputCommands()
{
    // LCD Change Function and Values
    //  To Right Rotary
    if (up == 1)
    {
        up = false;
        refreshScreen = true;
        if (settingsFlag == true)
        {
            if (settingEditTimerFlag == true)
            {
                if (parametersTimer[currentSettingScreen] >= parametersTimerMaxValue[currentSettingScreen] - 1)
                {
                    parametersTimer[currentSettingScreen] = parametersTimerMaxValue[currentSettingScreen];
                }
                else
                {
                    if (fastScroll == true)
                    {
                        parametersTimer[currentSettingScreen] += 1;
                    }
                    else
                    {
                        parametersTimer[currentSettingScreen] += 0.1;
                    }
                }
            }
            else
            {
                if (currentSettingScreen == numOfSettingScreens - 1)
                {
                    currentSettingScreen = 0;
                }
                else
                {
                    currentSettingScreen++;
                }
            }
        }
        else if (testMenuFlag == true)
        {
            if (currentTestMenuScreen == numOfTestMenu - 1)
            {
                currentTestMenuScreen = 0;
            }
            else
            {
                currentTestMenuScreen++;
            }
        }
        else
        {
            if (currentScreen == numOfMainScreens - 1)
            {
                currentScreen = 0;
            }
            else
            {
                currentScreen++;
            }
        }
    }

    // To Left Rotary
    if (down == 1)
    {
        down = false;
        refreshScreen = true;
        if (settingsFlag == true)
        {
            if (settingEditTimerFlag == true)
            {
                if (parametersTimer[currentSettingScreen] <= 0)
                {
                    parametersTimer[currentSettingScreen] = 0;
                }
                else
                {
                    if (fastScroll == true)
                    {
                        parametersTimer[currentSettingScreen] -= 1;
                    }
                    else
                    {
                        parametersTimer[currentSettingScreen] -= 0.1;
                    }
                }
            }
            else
            {
                if (currentSettingScreen <= 0)
                {
                    currentSettingScreen = numOfSettingScreens - 1;
                }
                else
                {
                    currentSettingScreen--;
                }
            }
        }
        else if (testMenuFlag == true)
        {
            if (currentTestMenuScreen <= 0)
            {
                currentTestMenuScreen = numOfTestMenu - 1;
            }
            else
            {
                currentTestMenuScreen--;
            }
        }
        else
        {
            if (currentScreen == 0)
            {
                currentScreen = numOfMainScreens - 1;
            }
            else
            {
                currentScreen--;
            }
        }
    }

    // Rotary Button Press
    if (middle == 1)
    {
        middle = false;
        refreshScreen = 1;
        if (currentScreen == 0 && settingsFlag == true)
        {
            if (currentSettingScreen == numOfSettingScreens - 1)
            {
                settingsFlag = false;
                currentSettingScreen = 0;
                saveSettings();
                loadSettings();
                setTime();
            }
            else
            {
                if (settingEditTimerFlag == true)
                {
                    settingEditTimerFlag = false;
                }
                else
                {
                    settingEditTimerFlag = true;
                }
            }
        }
        else if (runAutoFlag == true)
        {
            runAutoFlag = false;
            StopAll();
        }
        else if (testMenuFlag == true)
        {
            if (currentTestMenuScreen == numOfTestMenu - 1)
            {
                currentTestMenuScreen = 0;
                testMenuFlag = false;
                // runTestFlag = false;
            }
            else if (currentTestMenuScreen == 0)
            {
                if (Grinder.getMotorState() == false)
                {
                    stopVfd();
                    Grinder.relayOn();
                }
                else
                {
                    Grinder.relayOff();
                }
            }
            else if (currentTestMenuScreen == 1)
            {
                if (Dispenser.getMotorState() == false)
                {
                    Dispenser.relayOn();
                }
                else
                {
                    Dispenser.relayOff();
                }
            }
            else if (currentTestMenuScreen == 2)
            {
                if (Mixer.getMotorState() == false)
                {
                    stopVfd();
                    Mixer.relayOn();
                }
                else
                {
                    Mixer.relayOff();
                }
            }
            else if (currentTestMenuScreen == 3)
            {
                if (Linear.getMotorState() == false)
                {
                    Linear.relayOn();
                }
                else
                {
                    Linear.relayOff();
                }
            }
            else if (currentTestMenuScreen == 4)
            {
                if (DispenseTestRunFlag == false)
                {
                    DispenseTestRunFlag = true;
                    Valve.start();
                }
                else
                {
                    DispenseTestRunFlag = false;
                    Valve.stop();
                    Valve.relayOff();
                    Extruder.relayOff();
                }
            }
            else if (currentTestMenuScreen == 5)
            {
                if (Denester.getMotorState() == false)
                {
                    Denester.relayOn();
                }
                else
                {
                    Denester.relayOff();
                }
            }
            else if (currentTestMenuScreen == 6)
            {
                if (Conveyor.getMotorState() == false)
                {
                    stopVfd();
                    Conveyor.relayOn();
                }
                else
                {
                    Conveyor.relayOff();
                }
            }
        }
        else
        {
            if (currentScreen == 0)
            {
                settingsFlag = true;
            }
            else if (currentScreen == 1)
            {
                runAutoFlag = true;
                runAutoStat = 1;
                TimerMixingLiquid.start();
                refreshScreen = 1;
            }
            else if (currentScreen == 2)
            {
                testMenuFlag = true;
            }
        }
    }
}

void PrintRunAuto(String job, char *time)
{
    lcd.clear();
    lcd.print("Running Auto");
    lcd.setCursor(0, 1);
    lcd.print("Status: ");
    lcd.setCursor(0, 2);
    lcd.print(job);
    lcd.setCursor(0, 3);
    lcd.print("Timer: ");
    lcd.setCursor(7, 3);
    lcd.print(time);
}

void printScreen()
{

    if (settingsFlag == true)
    {
        lcd.clear();
        lcd.print(settings[currentSettingScreen][0]);
        lcd.setCursor(0, 1);
        if (currentSettingScreen == numOfSettingScreens - 1)
        {
            lcd.setCursor(0, 3);
            lcd.write(0);
            lcd.setCursor(2, 3);
            lcd.print("Click to Save All");
        }
        else
        {
            lcd.setCursor(0, 1);
            lcd.print(parametersTimer[currentSettingScreen]);
            lcd.print(" ");
            lcd.print(settings[currentSettingScreen][1]);
            lcd.setCursor(0, 3);
            lcd.write(0);
            lcd.setCursor(2, 3);
            if (settingEditTimerFlag == false)
            {
                lcd.print("ENTER TO EDIT");
            }
            else
            {
                lcd.print("ENTER TO SAVE");
            }
            lcd.setCursor(19, 3);
            if (fastScroll == true)
            {
                lcd.write(1);
            }
            else
            {
                lcd.write(2);
            }
        }
    }
    else if (runAutoFlag == true)
    {
        switch (runAutoStat)
        {
        case 1:
            PrintRunAuto("MIXING LIQUID", TimerMixingLiquid.getTimeRemaining());
            break;
        case 2:
            PrintRunAuto("DISPENSING/GRINDING", Grinder.getTimeRemaining());
            break;
        case 3:
            PrintRunAuto("MIXING", Mixer.getTimeRemaining());
            break;
        case 4:
            PrintRunAuto("TRANSFER TO EXTRUDE", Linear.getTimeRemaining());
            break;
        case 5:
            switch (extrudeStatus)
            {
            case 1:
                PrintRunAuto("DROPPING TRAY", "N/A");
                break;
            case 2:
                PrintRunAuto("DETECTING TRAY", trayDetectTimer.getTimeRemaining());
                break;
            case 3:
                PrintRunAuto("MOVING TO NOZZLE", "N/A");
                break;
            case 4:
                PrintRunAuto("DISPENSING", Valve.getTimeRemaining());
                break;
            case 5:
                PrintRunAuto("CLOSING VALVE", CloseTimer.getTimeRemaining());
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
    }
    else if (testMenuFlag == true)
    {
        lcd.clear();
        lcd.print(TestMenuScreen[currentTestMenuScreen]);

        if (currentTestMenuScreen == numOfTestMenu - 1)
        {
            lcd.setCursor(0, 3);
            lcd.print("Click to Exit Test");
        }
        else
        {
            lcd.setCursor(0, 3);
            lcd.print("Click to Run Test");
        }
    }
    else
    {
        lcd.clear();
        lcd.print(screens[currentScreen][0]);
        lcd.setCursor(0, 3);
        lcd.write(0);
        lcd.setCursor(2, 3);
        lcd.print(screens[currentScreen][1]);
        refreshScreen = false;
    }
}

void setupJumper()
{
}

void setup()
{
    // Encoder Setup
    encoder = new ClickEncoder(4, 2, 3); // Actual
    // encoder = new ClickEncoder(3, 2, A0); // TestBench
    encoder->setAccelerationEnabled(false);
    Timer1.initialize(1000);
    Timer1.attachInterrupt(timerIsr);
    last = encoder->getValue();

    // LCD Setup
    lcd.init();
    lcd.createChar(0, enterChar);
    lcd.createChar(1, fastChar);
    lcd.createChar(2, slowChar);
    lcd.clear();
    lcd.backlight();
    refreshScreen = true;
    Serial.begin(9600);

    initSensors();
    // saveSettings();
    loadSettings();
    setTime();
    StopAll();
}

void loop()
{
    readRotaryEncoder();
    readButtonEncoder();
    inputCommands();

    if (refreshScreen == true)
    {
        printScreen();
        refreshScreen = false;
    }

    if (runAutoFlag == true)
    {

        RunAuto();
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval)
        {
            // save the last time you blinked the LED
            previousMillis = currentMillis;
            refreshScreen = true;
        }
    }

    if (testMenuFlag == true)
    {
        TestRun();
    }
}