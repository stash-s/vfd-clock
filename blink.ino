#include "SimpleTimer.h"

const int output_pins[] = {0,1,2,3,4,5,6,7};
const int output_pins_max = sizeof(output_pins) / sizeof(int);
const int segment_max = 7;
const int dot_pin = 7;

const int mux_pins[] = {8,9};
const int mux_pins_max = sizeof(mux_pins) / sizeof(int);

    

const byte seven_seg_digits[10][7] = { { 1,1,1,1,1,1,0 },  // = 0
                                       { 0,1,1,0,0,0,0 },  // = 1
                                       { 1,1,0,1,1,0,1 },  // = 2
                                       { 1,1,1,1,0,0,1 },  // = 3
                                       { 0,1,1,0,0,1,1 },  // = 4
                                       { 1,0,1,1,0,1,1 },  // = 5
                                       { 1,0,1,1,1,1,1 },  // = 6
                                       { 1,1,1,0,0,0,0 },  // = 7
                                       { 1,1,1,1,1,1,1 },  // = 8
                                       { 1,1,1,0,0,1,1 }   // = 9
};


//RTCTimer timer;

SimpleTimer timer;


int digits[mux_pins_max];
int dots[mux_pins_max];

int mux_digit=0;
int fade_clock=0;
int fade_amount=0;

void displayDigit (uint8_t digit);


void display () 
{
    digitalWrite (mux_pins[mux_digit], LOW);

    ++fade_clock;
    if (fade_clock < fade_amount) {
        return;
    } else {
        fade_clock = 0;
    }
    
    ++mux_digit;
    if (mux_digit >= mux_pins_max) {
        mux_digit = 0;
    }

    displayDigit (digits[mux_digit]);

    digitalWrite (dot_pin, dots[mux_digit] ? HIGH : LOW);
    

    digitalWrite (mux_pins[mux_digit], HIGH);
}


void blink_dot (int dot) 
{
    if (dots[dot] == LOW) {
        dots[dot] = HIGH;
    } else {
        dots[dot] = LOW;
    }
}

void blink_dot_0 () 
{
    blink_dot(0);
};


template <int the_dot>
void blink_dot_generic (void) 
{
    blink_dot (the_dot);
}



void update_digit_1 () 
{
    ++ (digits[1]);
    if (digits[1] >= 10) {
        digits[1] = 0;
    }
    
}

void update_digit_2 () 
{
    ++ (digits[0]);
    if (digits[0] >= 10) {
        digits[0] = 0;
    }
    
}



void setup () 
{
    for (auto pin : output_pins) {
        pinMode (pin, OUTPUT);
    }
    for (auto pin : mux_pins) {
        pinMode (pin, OUTPUT);
        digitalWrite (pin, LOW);
    }

    dots[0] = HIGH;
    
    timer.setInterval (10, display);
    timer.setInterval (1000, update_digit_1);
    timer.setInterval (10000, update_digit_2);
    timer.setInterval (500, blink_dot_generic<1>);
    timer.setInterval (500, blink_dot_0);
    
    //timer.every (5, 
}

static int counter=0;

void displayDigit (uint8_t digit) 
{
    for (int i=0; i < segment_max; ++i) {
        digitalWrite (output_pins[i], seven_seg_digits[digit][i]);
    }
}


void loop () 
{
    /*  for (int i=0; i < output_pins_max; ++i) {
        if (counter == i) {
            digitalWrite (output_pins[i], HIGH);
        } else {
            digitalWrite (output_pins[i], LOW);
        }
        }*/

    timer.run ();
    
}
