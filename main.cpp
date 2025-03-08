/*
Filename: main.cpp
Description: Reports alarm state on the LCD when buttons '2' and '3' are pressed on the keypad, 
Author: A Redfern
Date: 06/03/2025
Input/Output: 
Version: 01.00
Change log: 01.00 - Initial issue
*/
 
#include "mbed.h"
#include "display.h"

DigitalOut Alarm(LED3);
DigitalOut CodeWarning(LED2);
DigitalOut TooManyAttempts(LED1);
AnalogIn GasSensor(A2);
AnalogIn TempSensor(A1);
float TempReading;
float GasReading;
int GasAlarm;
int TempAlarm;
int Tick =0;

#define NUMBER_OF_KEYS                           4
#define DEBOUNCE_KEY_TIME_MS                    40
#define TIME_INCREMENT_MS                       10
#define KEYPAD_NUMBER_OF_ROWS                    4
#define KEYPAD_NUMBER_OF_COLS                    4

bool alarmState            = false;
int accumulatedTimeAlarm = 0;

DigitalOut keypadRowPins[KEYPAD_NUMBER_OF_ROWS] = {PB_3, PB_5, PC_7, PA_15};
DigitalIn keypadColPins[KEYPAD_NUMBER_OF_COLS]  = {PB_12, PB_13, PB_15, PC_6};

typedef enum {
    MATRIX_KEYPAD_SCANNING,
    MATRIX_KEYPAD_DEBOUNCE,
    MATRIX_KEYPAD_KEY_HOLD_PRESSED
} matrixKeypadState_t;

int numberOfIncorrectCodes = 0;
int numberOfHashKeyReleasedEvents = 0;
int keyBeingCompared    = 0;
char codeSequence[NUMBER_OF_KEYS]   = { '1', '8', '0', '5' };
char keyPressed[NUMBER_OF_KEYS] = { '0', '0', '0', '0' };
int accumulatedDebounceMatrixKeypadTime = 0;
int matrixKeypadCodeIndex = 0;
char matrixKeypadLastKeyPressed = '\0';
char matrixKeypadIndexToCharArray[] = {
    '1', '2', '3', 'A',
    '4', '5', '6', 'B',
    '7', '8', '9', 'C',
    '*', '0', '#', 'D',
};
matrixKeypadState_t matrixKeypadState;

void Init();
void alarmDeactivationUpdate();
void alarmActivationUpdate();
void matrixKeypadInit();
char matrixKeypadScan();
char matrixKeypadUpdate();
bool areEqual();
void LCDUpdate();

 
int main()
{
    Init();

    while( true ) {
        alarmDeactivationUpdate();
        alarmActivationUpdate();
        LCDUpdate();
        thread_sleep_for(TIME_INCREMENT_MS);
    }
}

void Init(){
matrixKeypadInit();
}

void alarmDeactivationUpdate()
{
    char keyReleased = matrixKeypadUpdate();

    if ( numberOfIncorrectCodes < 5 ) {
        if( keyReleased != '\0' && keyReleased != '#') {
            keyPressed[matrixKeypadCodeIndex] = keyReleased;
            if( matrixKeypadCodeIndex >= NUMBER_OF_KEYS ) {
                matrixKeypadCodeIndex = 0;
            } else {
                matrixKeypadCodeIndex++;
            }
        }
        if( keyReleased == '#' ) {
            if( CodeWarning ) {
                numberOfHashKeyReleasedEvents++;
                if( numberOfHashKeyReleasedEvents >= 2 ) {
                    CodeWarning = false;
                    numberOfHashKeyReleasedEvents = 0;
                    matrixKeypadCodeIndex = 0;
                }
        } else {
                if ( alarmState ) {
                    if (areEqual() ) {
                        alarmState = false;
                        Alarm = false;
                        numberOfIncorrectCodes = 0;
                        matrixKeypadCodeIndex = 0;
                    } else {
                        CodeWarning = true;
                        numberOfIncorrectCodes++;
                    }
                }
            }
        }
    } else {
        TooManyAttempts = true;
    }

}

bool areEqual()
{
    int i;
    for (i = 0; i < NUMBER_OF_KEYS; i++) {
        if (codeSequence[i] != keyPressed[i]) {
            return false;
        }
    }
    return true;
}

void alarmActivationUpdate()
{
    if( !alarmState && (TempAlarm == true || GasAlarm == true)) {             
        alarmState = true;
        Alarm = true;

    }

    TempReading = TempSensor.read()*330;
    if(TempReading >= 28){
        TempAlarm = true;
    }else{
        TempAlarm = false;
    }

    GasReading = GasSensor.read()*3.3;
    if(GasReading >= 1.2){
        GasAlarm = true;
    }else{
        GasAlarm = false;
    }
}




void matrixKeypadInit()
{
    matrixKeypadState = MATRIX_KEYPAD_SCANNING;
    int pinIndex = 0;
    for( pinIndex=0; pinIndex<KEYPAD_NUMBER_OF_COLS; pinIndex++ ) {
        (keypadColPins[pinIndex]).mode(PullUp);
    }
}

char matrixKeypadScan()
{
    int row = 0;
    int col = 0;
    int i = 0;
    for( row=0; row<KEYPAD_NUMBER_OF_ROWS; row++ ) {
        for( i=0; i<KEYPAD_NUMBER_OF_ROWS; i++ ) {
            keypadRowPins[i] = true;
        }
        keypadRowPins[row] = false;
        for( col=0; col<KEYPAD_NUMBER_OF_COLS; col++ ) {
            if( keypadColPins[col] == false ) {
                return matrixKeypadIndexToCharArray[row*KEYPAD_NUMBER_OF_ROWS + col];
            }
        }
    }
    return '\0';
}

char matrixKeypadUpdate()
{
    char keyDetected = '\0';
    char keyReleased = '\0';
    switch( matrixKeypadState ) {
    case MATRIX_KEYPAD_SCANNING:
        keyDetected = matrixKeypadScan();
        if( keyDetected != '\0' ) {
            matrixKeypadLastKeyPressed = keyDetected;
            accumulatedDebounceMatrixKeypadTime = 0;
            matrixKeypadState = MATRIX_KEYPAD_DEBOUNCE;
        }
        break;

    case MATRIX_KEYPAD_DEBOUNCE:
        if( accumulatedDebounceMatrixKeypadTime >=
            DEBOUNCE_KEY_TIME_MS ) {
            keyDetected = matrixKeypadScan();
            if( keyDetected == matrixKeypadLastKeyPressed ) {
                matrixKeypadState = MATRIX_KEYPAD_KEY_HOLD_PRESSED;
            } else {
                matrixKeypadState = MATRIX_KEYPAD_SCANNING;
            }
        }
        accumulatedDebounceMatrixKeypadTime = accumulatedDebounceMatrixKeypadTime + TIME_INCREMENT_MS;
        break;

    case MATRIX_KEYPAD_KEY_HOLD_PRESSED:
        keyDetected = matrixKeypadScan();
        if( keyDetected != matrixKeypadLastKeyPressed ) {
            if( keyDetected == '\0' ) {
                keyReleased = matrixKeypadLastKeyPressed;
            }
            matrixKeypadState = MATRIX_KEYPAD_SCANNING;
        }
        break;

    default:
        matrixKeypadInit();
        break;

    }

    return keyReleased;
}

void LCDUpdate()
{ 

    if (Tick % 500 == 0){ // update LCD every 5 seconds
        if(GasAlarm == true && TempAlarm == true){
            displayInit( DISPLAY_CONNECTION_I2C_PCF8574_IO_EXPANDER );
            displayCharPositionWrite ( 0,0 );
            displayStringWrite( "Gas Alarm Active    " );
            displayCharPositionWrite ( 0,1 );
            displayStringWrite( "Temp Alarm Active   " );
            displayCharPositionWrite ( 0,2 );
            displayStringWrite( "Enter code on keypad" );
            displayCharPositionWrite ( 0,3 );
            displayStringWrite( "to disable alarm    " );
        }

        if(GasAlarm == true && TempAlarm == false){
            displayInit( DISPLAY_CONNECTION_I2C_PCF8574_IO_EXPANDER );
            displayCharPositionWrite ( 0,0 );
            displayStringWrite( "Gas Alarm Active    " );
            displayCharPositionWrite ( 0,2 );
            displayStringWrite( "Enter code on keypad" );
            displayCharPositionWrite ( 0,3 );
            displayStringWrite( "to disable alarm    " );
        }
        
        if(TempAlarm == true && GasAlarm == false){
            displayInit( DISPLAY_CONNECTION_I2C_PCF8574_IO_EXPANDER );
            displayCharPositionWrite ( 0,0 );
            displayStringWrite( "Temp Alarm Active   " );
            displayCharPositionWrite ( 0,2 );
            displayStringWrite( "Enter code on keypad" );
            displayCharPositionWrite ( 0,3 );
            displayStringWrite( "to disable alarm    " );
        }

        if(TempAlarm == false && GasAlarm == false && alarmState == false){
            displayInit( DISPLAY_CONNECTION_I2C_PCF8574_IO_EXPANDER );
            displayCharPositionWrite ( 0,0 );
            displayStringWrite( "No Active Alarms    " );
        }
  
    }
    Tick = Tick+1;

}

