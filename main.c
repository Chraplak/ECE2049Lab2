// INCLUDES
#include <msp430.h>
#include <stdlib.h>
#include <math.h>
#include "peripherals.h"



// PROTOTYPES
void swDelay(long unsigned int numLoops);
__interrupt void Timer_A2_ISR(void);
bool delay(long unsigned int millis);
void resetTimer();
void welcome(char key);
void reset(char key);
void play(char key);
void win(void);
void lose(void);
void configButtons();
char buttonStates();
void configLEDs(char inbits);
void playNote(int index);
double getPitch();



// NOTE ARRAYS
// stores a pitch, ledValue, and LCD xValue for each term in arrays; denotations in parentheses are for rests
// pitch -> Hz
double pitch[36] = {0,0,0,0,
                    784,784,784,784,
                    000,784,784,1175,
                    1047,1047,1047,0,
                    932,932,880,880,
                    784,784,784,784,
                    000,784,784,1175,
                    1318,1318,1318,0,
                    1047,0,880,0};
// ledValue -> (0x00) 0x08 0x04 0x02 0x01
char ledValue[36] = {0x00,0x00,0x00,0x00,
                     0x08,0x08,0x08,0x08,
                     0x00,0x08,0x08,0x01,
                     0x02,0x02,0x02,0x00,
                     0x02,0x02,0x04,0x04,
                     0x08,0x08,0x08,0x08,
                     0x00,0x08,0x08,0x02,
                     0x01,0x01,0x01,0x01,
                     0x02,0x00,0x04,0x00};
// xValue -> (-10) 20 40 60 80
int xValue[36] = {-10,-10,-10,-10,
                  20,20,20,20,
                  -10,20,20,80,
                  60,60,60,-10,
                  60,60,40,40,
                  20,20,20,20,
                  -10,20,20,60,
                  80,80,80,-10,
                  60,-10,40,-10};
// (0x00) 0x01 0x02 0x04 0x08
char buttonValue[36] = {0x00,0x00,0x00,0x00,
                        0x01,0x01,0x01,0x01,
                        0x00,0x01,0x01,0x08,
                        0x04,0x04,0x04,0x00,
                        0x04,0x04,0x02,0x02,
                        0x01,0x01,0x01,0x01,
                        0x00,0x01,0x01,0x04,
                        0x08,0x08,0x08,0x00,
                        0x04,0x00,0x02,0x00};



// GLOBALS
enum Notes{A, BFLAT, B, c, CSHARP, D, EFLAT, E, F, FSHARP, G, AFLAT};
enum States{WELCOME, RESET, PLAY, WIN, LOSE};
int currentState = RESET;
long unsigned int currentTime = 0;
long unsigned int startingTime = 0;
long unsigned int delayDuration = 0;
int timeIndex = 0;
bool startTimer = false;
int missCounter = 0;
volatile int i = 0;



// UTILITY VARIABLES
unsigned char dispSz = 3;
unsigned char dispThree[3];



// MAIN
void main(void) {
    WDTCTL = WDTPW | WDTHOLD;    // Stop watchdog timer. Always need to stop this!!
                                 // You can then configure it properly, if desired

    // timer A2 management
    TA2CTL = TASSEL_1 + ID_0 + MC_1; // 32786 Hz is set
    TA2CCR0 = 327; // sets interrupt to occur every (TA2CCR0 + 1)/32786 seconds
    TA2CCTL0 = CCIE; // enables TA2CCR0 interrupt

    // enables global interrupts
    _BIS_SR(GIE);

    // setup for LEDs, LCD, Keypad, Buttons
    initLeds();
    configDisplay();
    configKeypad();
    configButtons();

    // sets buffer on sides of printed character
    dispThree[0] = ' ';
    dispThree[2] = ' ';

    // state machine
    while (1) {
        char key = getKey();
        if (key == '#') {
            reset(key);
            resetTimer();
            timeIndex = 0;
            startTimer = false;
            configLEDs(0x00);
            BuzzerOff();
            setLeds(0x00);
        }
        switch (currentState) {
        case(WELCOME):
            welcome(key);
        break;
        case(RESET):
            reset(key);
        break;
        case(PLAY):
            play(key);
        break;
        case(WIN):
            win();
        break;
        case(LOSE):
            lose();
        break;
        }
    }
}



// SOFTWARE DELAY
void swDelay(long unsigned int millis) {
    volatile unsigned int i,j;  // volatile to prevent removal in optimization
                                // by compiler. Functionally this is useless code

    for (j=0; j<millis; j++) {
        i = 100 ;                 // SW Delay
        while (i > 0) i--;
    }
}



// HARDWARE DELAY
bool delay(long unsigned int millis) {
    if (delayDuration == 0) {
        startingTime = currentTime;
        delayDuration = millis;
    }
    else if (startingTime + delayDuration <= currentTime) {
        resetTimer();
        timeIndex++;
        return true;
    }
    return false;
}



// END HARDWARE DELAY
bool delayEnd() {
    if (startingTime + delayDuration <= currentTime) {
        resetTimer();
        timeIndex++;
        return true;
    }

    return false;
}



//RESET TIMER
void resetTimer() {
    startingTime = 0;
    delayDuration = 0;
    currentTime = 0;
}



// TIMER INTERRUPT
#pragma vector = TIMER2_A0_VECTOR
__interrupt void Timer_A2_ISR(void) {
    currentTime++;
}



// idk what this does lol
void nextState(int state) {
    currentState = state;
    timeIndex = 0;
    startTimer = false;
}



// RESTART HARDWARE TIMER
void timerStart() {
    resetTimer();
    delayDuration = 1;
    currentTime = 2;
}



// WELCOME STATE HANDLER
void welcome(char key) {

    if (key == '*' && startTimer == false) {
        startTimer = true;
        timerStart();
    }

    i = 0;
    missCounter = 0;

    if (startTimer) {
        //Plays the 3-2-1 countdown
        if (delayEnd()) {
            Graphics_clearDisplay(&g_sContext); // Clear the display

            if (timeIndex == 1) {
                Graphics_drawStringCentered(&g_sContext, "3", AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);
                configLEDs(BIT0);
                delay(100);
            }
            else if (timeIndex == 2) {
                Graphics_drawStringCentered(&g_sContext, "2", AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);
                configLEDs(BIT1);
                delay(100);
            }
            else if (timeIndex == 3) {
                Graphics_drawStringCentered(&g_sContext, "1", AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);
                configLEDs(BIT0);
                delay(100);
            }
            else if (timeIndex == 4) {
                Graphics_drawStringCentered(&g_sContext, "GO", AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);
                configLEDs(BIT0 | BIT1);
                delay(100);
            }
            else if (timeIndex == 5) {
                configLEDs(0x00);
                nextState(PLAY);
                timerStart();
            }

            Graphics_flushBuffer(&g_sContext);
        }
    }
}



// RESET HANDLER
void reset(char key) {
    // *** Intro Screen ***
    Graphics_clearDisplay(&g_sContext); // Clear the display

    // Write some text to the display
    Graphics_drawStringCentered(&g_sContext, "MSP40 Hero", AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "Welcome", AUTO_STRING_LENGTH, 48, 25, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "Press *", AUTO_STRING_LENGTH, 48, 45, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "To Begin", AUTO_STRING_LENGTH, 48, 55, TRANSPARENT_TEXT);

    //Pushes new screen update
    Graphics_flushBuffer(&g_sContext);

    currentState = WELCOME;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// PLAY STATE HANDLER
void play(char key) {
    if (startTimer == false) {
            startTimer = true;
            timerStart();
    }

    // while loop cycles through pitch, xValue, and ledValue arrays
    while(delayEnd()) {

        // displays five sequential 'notes' at a time as zeroes on screen (rests are zeroes stores offscreen for organization)
        // displays five notes in advance so player can see notes coming
        Graphics_clearDisplay(&g_sContext); //clear screen
        Graphics_drawStringCentered(&g_sContext, "0", AUTO_STRING_LENGTH, xValue[i+4], 20, TRANSPARENT_TEXT);
        Graphics_drawStringCentered(&g_sContext, "0", AUTO_STRING_LENGTH, xValue[i+3], 30, TRANSPARENT_TEXT);
        Graphics_drawStringCentered(&g_sContext, "0", AUTO_STRING_LENGTH, xValue[i+2], 40, TRANSPARENT_TEXT);
        Graphics_drawStringCentered(&g_sContext, "0", AUTO_STRING_LENGTH, xValue[i+1], 50, TRANSPARENT_TEXT);
        Graphics_drawStringCentered(&g_sContext, "0", AUTO_STRING_LENGTH, xValue[i], 60, TRANSPARENT_TEXT);
        Graphics_drawLine(&g_sContext,0,60,96,60);

        // sets LED value of current note in array
        setLeds(ledValue[i]);

        if(buttonValue[i] == buttonStates()) { // if button pressed corresponds to i'th button value, play i'th pitch value on buzzer
              BuzzerOn(pitch[i]);
        }
        else {
              BuzzerOff();
              if(xValue[i] > 10) { // if i'th note is not a rest and was not pressed, add to missCounter and display "Miss!" at bottom
                   missCounter++;
                   Graphics_drawStringCentered(&g_sContext, "Miss!", AUTO_STRING_LENGTH, 48, 80, TRANSPARENT_TEXT);
              }
        }

        Graphics_flushBuffer(&g_sContext);

        // lose condition: if missCounter reaches 5, send to LOSE state
        if(missCounter > 4) {
            nextState(LOSE);
            //break;
            //break;
        }

        i++;
        delay(25);
    }


    if(i > 39 && missCounter < 4) currentState = WIN;

}



// WIN STATE HANDLER
void win(void) {
    Graphics_clearDisplay(&g_sContext); // Clear the display

        // Write win text to the display
        Graphics_drawStringCentered(&g_sContext, "You Win!", AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);
        Graphics_drawStringCentered(&g_sContext, ":D", AUTO_STRING_LENGTH, 48, 25, TRANSPARENT_TEXT);

        //Pushes new screen update
        Graphics_flushBuffer(&g_sContext);

        swDelay(2000);

        // reset everything
        nextState(RESET);
}



// LOSE STATE HANDLER
void lose(void) {
    Graphics_clearDisplay(&g_sContext); // Clear the display

        // Write lose text to the display
        Graphics_drawStringCentered(&g_sContext, "You Lose...", AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);
        Graphics_drawStringCentered(&g_sContext, ":(", AUTO_STRING_LENGTH, 48, 25, TRANSPARENT_TEXT);

        //Pushes new screen update
        Graphics_flushBuffer(&g_sContext);

        swDelay(2000);

        // reset everything
        nextState(RESET);
}

// BUTTON CONFIGURATION HELPER
void configButtons() {
    //Sets P2.2, P3.6, P7.0, and P7.4 to IO
    P2SEL &= ~BIT2;
    P3SEL &= ~BIT6;
    P7SEL &= ~(BIT0 | BIT4);

    //Sets P2.2, P3.6, P7.0, and P7.4 to input
    P2DIR &= ~BIT2;
    P3DIR &= ~BIT6;
    P7DIR &= ~(BIT0 | BIT4);

    //Sets pins to use pull up/down
    P2REN |= BIT2;
    P3REN |= BIT6;
    P7REN |= (BIT0 | BIT4);

    //Sets pins to pull up
    P2OUT |= BIT2;
    P3OUT |= BIT6;
    P7OUT |= (BIT0 | BIT4);
}

// BUTTON PRESS CHECKING HANDLER
char buttonStates() {
    char returnState = 0x00;
    if ((P2IN & BIT2) == 0) {
        returnState |= BIT2;
    }
    if ((P3IN & BIT6) == 0) {
        returnState |= BIT1;
    }
    if ((P7IN & BIT0) == 0) {
        returnState |= BIT0;
    }
    if ((P7IN & BIT4) == 0) {
        returnState |= BIT3;
    }
    return returnState;
}

// LED CONFIGURATION HELPER
void configLEDs(char inbits) {
    //Sets pins to IO
    P1SEL &= ~(BIT0);
    P4SEL &= ~(BIT7);

    //Sets pins to output
    P1DIR |= (BIT0);
    P4DIR |= (BIT7);

    //Sets LEDs to off
    P1OUT &= ~(BIT0);
    P4OUT &= ~(BIT7);

    //Sets Leds according to inbits
    if (inbits & BIT0) {
        P1OUT |= (BIT0);
    }
    if (inbits & BIT1) {
        P4OUT |= (BIT7);
    }
}
