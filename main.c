/************** ECE2049 DEMO CODE ******************/
/**************  13 March 2019   ******************/
/***************************************************/

#include <msp430.h>
#include <stdlib.h>
#include <math.h>

/* Peripherals.c and .h are where the functions that implement
 * the LEDs and keypad, etc are. It is often useful to organize
 * your code by putting like functions together in files.
 * You include the header associated with that file(s)
 * into the main file of your project. */
#include "peripherals.h"

// Function Prototypes
void initNotes();
__interrupt void Timer_A2_ISR(void);
bool delay(long unsigned int millis);
void resetTimer();
void welcome(char key);
void reset(char key);
void play(char key);
void configButtons();
char buttonStates();
void configLEDs(char inbits);
void drawNotes();
void playNote(int pitch, int time, int octave);

//added git successfully

/////////////////////////////////////////////////////////
void lose();
void win();

#define ARRAYSIZE 12

int notePitch[ARRAYSIZE];
long int noteTime[ARRAYSIZE];
int noteOctave[ARRAYSIZE];

// Declare globals here
enum Notes{A, BFLAT, B, c, CSHARP, D, EFLAT, E, F, FSHARP, G, AFLAT, REST};
enum States{WELCOME, RESET, PLAY, LOSE, WIN};
int currentState = RESET;
long unsigned int currentTime = 0;
long unsigned int startingTime = 0;
long unsigned int delayDuration = 0;
int timeIndex = 0;
bool startTimer = false;

//Utility variables
unsigned char dispSz = 3;
unsigned char dispThree[3];

// Main
void main(void) {
    WDTCTL = WDTPW | WDTHOLD;    // Stop watchdog timer. Always need to stop this!!
                                 // You can then configure it properly, if desired

    //Sets timer A2 frequency

    //32786 Hz is set
    TA2CTL = TASSEL_1 + ID_0 + MC_1;
    //Sets interrupt to occur every (TA2CCR0 + 1)/32786 seconds
    TA2CCR0 = 31;
    //Enables TA2CCR0
    TA2CCTL0 = CCIE;

    //Enables global interrupts
    _BIS_SR(GIE);

    // Useful code starts here
    initLeds();
    configDisplay();
    configKeypad();

    configButtons();
    initNotes();


    //Sets buffer on sides of printed character
    dispThree[0] = ' ';
    dispThree[2] = ' ';

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
            Graphics_clearDisplay(&g_sContext); // Clear the display
            Graphics_flushBuffer(&g_sContext);
            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
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
        case(LOSE):
            lose();
        break;
        case(WIN):
            win();
        break;
        }
    }
}

void initNotes() {
    volatile int i = 0;

    int notes[] = {c, D, EFLAT, c, D, F, EFLAT, c, D, EFLAT, D, c};
    long int time[] = {500, 500, 1000, 500, 500, 250, 1000, 500, 500, 1000, 500, 500};
    int octave[] = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};

    for (i = 0; i < ARRAYSIZE; i++) {
        notePitch[i] = notes[i];
        noteTime[i] = time[i];
        noteOctave[i] = octave[i];
    }
}

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

bool delayEnd() {
    if (startingTime + delayDuration <= currentTime) {
        resetTimer();
        timeIndex++;
        return true;
    }

    return false;
}

void resetTimer() {
    startingTime = 0;
    delayDuration = 0;
    currentTime = 0;
}

#pragma vector = TIMER2_A0_VECTOR
__interrupt void Timer_A2_ISR(void) {
    currentTime++;
}

void nextState(int state) {
    currentState = state;
    timeIndex = 0;
    startTimer = false;
}

void timerStart() {
    resetTimer();
    delayDuration = 1;
    currentTime = 2;
}

void welcome(char key) {

    if (key == '*' && startTimer == false) {
        startTimer = true;
        timerStart();
    }


    if (startTimer) {
        //Plays the 3-2-1 countdown
        if (delayEnd()) {
            Graphics_clearDisplay(&g_sContext); // Clear the display

            if (timeIndex == 1) {
                Graphics_drawStringCentered(&g_sContext, "3", AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);
                configLEDs(BIT0);
                delay(1000);
            }
            else if (timeIndex == 2) {
                Graphics_drawStringCentered(&g_sContext, "2", AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);
                configLEDs(BIT1);
                delay(1000);
            }
            else if (timeIndex == 3) {
                Graphics_drawStringCentered(&g_sContext, "1", AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);
                configLEDs(BIT0);
                delay(1000);
            }
            else if (timeIndex == 4) {
                Graphics_drawStringCentered(&g_sContext, "GO", AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);
                configLEDs(BIT0 | BIT1);
                delay(1000);
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

void play(char key) {

    //A, BFLAT, B, c, CSHARP, D, EFLAT, E, F, FSHARP, G, AFLAT
    if (delayEnd()) {
        if (startTimer) {
            if (timeIndex <= ARRAYSIZE) {
                int index = timeIndex-1;
                //playNote(timeIndex-1);
                playNote(notePitch[index], noteTime[index], noteOctave[index]);
                startTimer = false;
            }
            else {
                timeIndex = 0;
            }
        }
        else {
            BuzzerOff();
            setLeds(0x00);
            startTimer = true;
            delay(50);
            timeIndex--;
        }
    }
    drawNotes();
}

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

void drawNotes() {
    if (currentTime % 500 == 0) {
        Graphics_clearDisplay(&g_sContext); // Clear the display
        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
        Graphics_drawLine(&g_sContext, 19, 96, 19, 96);
        Graphics_drawLine(&g_sContext, 38, 96, 38, 96);
        Graphics_drawLine(&g_sContext, 57, 96, 57, 96);
        Graphics_drawLine(&g_sContext, 76, 96, 76, 96);
        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
        Graphics_drawLine(&g_sContext, 0, 80, 96, 80);
        volatile int i;
        int y = 80;
        for (i = timeIndex-1; i < ARRAYSIZE; i++) {
            int height;
            if (i == timeIndex-1) {
                height = (noteTime[i] - currentTime)/10;
                y -= height;
            }
            else {
                height = noteTime[i]/10;
                y -= height + 10;
            }

            if (notePitch[i] == c || notePitch[i] == CSHARP || notePitch[i] == D) {
                Graphics_drawLine(&g_sContext, 19, y, 19, y + height);
            }
            else if (notePitch[i] == EFLAT || notePitch[i] == E || notePitch[i] == F) {
                Graphics_drawLine(&g_sContext, 38, y, 38, y + height);
            }
            else if (notePitch[i] == FSHARP || notePitch[i] == G || notePitch[i] == AFLAT) {
                Graphics_drawLine(&g_sContext, 57, y, 57, y + height);
            }
            else if (notePitch[i] == A || notePitch[i] == BFLAT || notePitch[i] == B) {
                Graphics_drawLine(&g_sContext, 76, y, 76, y + height);
            }
        }
        Graphics_flushBuffer(&g_sContext);
    }
}


void playNote(int pitch, int time, int octave) {

    double octaveAdjust = pow(2, octave - 5);
    char outLed = 0x00;

    switch (pitch) {
    case(c):
        BuzzerOn((523.0*octaveAdjust));
        outLed |= BIT0;
    break;
    case(CSHARP):
        BuzzerOn((554.0*octaveAdjust));
        outLed |= BIT0;
    break;
    case(D):
        BuzzerOn((587.0*octaveAdjust));
        outLed |= BIT0;
    break;
    case(EFLAT):
        BuzzerOn((622.0*octaveAdjust));
        outLed |= BIT1;
    break;
    case(E):
        BuzzerOn((659.0*octaveAdjust));
        outLed |= BIT1;
    break;
    case(F):
        BuzzerOn((698.0*octaveAdjust));
        outLed |= BIT1;
    break;
    case(FSHARP):
        BuzzerOn((740.0*octaveAdjust));
        outLed |= BIT2;
    break;
    case(G):
        BuzzerOn((784.0*octaveAdjust));
        outLed |= BIT2;
    break;
    case(AFLAT):
        BuzzerOn((831.0*octaveAdjust));
        outLed |= BIT2;
    break;
    case(A):
        BuzzerOn((880.0*octaveAdjust));
        outLed |= BIT3;
    break;
    case(BFLAT):
        BuzzerOn((932.0*octaveAdjust));
        outLed |= BIT3;
    break;
    case(B):
        BuzzerOn((988.0*octaveAdjust));
        outLed |= BIT3;
    break;
    default:
        BuzzerOff();
    break;
    }
    setLeds(outLed);

    delay(time);
}


void lose() {
    if (startTimer == false) {
        startTimer = true;
        timerStart();
        Graphics_clearDisplay(&g_sContext); // Clear the display

        // Write some text to the display
        Graphics_drawStringCentered(&g_sContext, "You Lose!!!", AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);

        //Pushes new screen update
        Graphics_flushBuffer(&g_sContext);
    }

    if (delayEnd()) {
        if (timeIndex == 48) {
            nextState(RESET);
        }
        else if (timeIndex % 2 == 0) {
            setLeds(BIT0 | BIT1 | BIT2 | BIT3);
            BuzzerOn(1100);
            delay(50);
        }
        else{
            setLeds(0x00);
            BuzzerOff();
            delay(50);
        }
    }
}


void win() {
    if (startTimer == false) {
        startTimer = true;
        timerStart();
        Graphics_clearDisplay(&g_sContext); // Clear the display

        // Write some text to the display
        Graphics_drawStringCentered(&g_sContext, "You Win!!!", AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);

        //Pushes new screen update
        Graphics_flushBuffer(&g_sContext);
    }

    if (delayEnd()) {

        if (timeIndex == 10) {
            nextState(RESET);
        }
        else if (timeIndex == 1) {
            setLeds(BIT0);
            BuzzerOn(325);
            delay(100);
        }
        else if (timeIndex == 2) {
            setLeds(BIT1);
            BuzzerOn(400);
            delay(100);
        }
        else if (timeIndex == 3) {
            setLeds(BIT2);
            BuzzerOn(600);
            delay(100);
        }
        else if (timeIndex == 4) {
            setLeds(BIT3);
            BuzzerOn(800);
            delay(100);
        }
        else if (timeIndex == 5 || timeIndex == 7) {
            setLeds(BIT0 | BIT1 | BIT2 | BIT3);
            BuzzerOn(1100);
            delay(100);
        }
        else if (timeIndex == 5 || timeIndex == 8) {
            setLeds(0x00);
            BuzzerOff();
            delay(50);
        }
        else if (timeIndex == 9) {
            delay(500);
        }
    }
}
