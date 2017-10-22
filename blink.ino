#include <Time.h>

#include "SimpleTimer.h"

const int data_pin  = 3;
const int latch_pin = 4;
const int clock_pin = 5;
const int oe_pin    = 6;

const int output_pins[] = {
    data_pin,
    latch_pin,
    clock_pin,
    oe_pin
};

const int output_pins_max = sizeof(output_pins) / sizeof(int);
const int segment_max = 7;
//const int dot_pin = 7;

//const int mux_pins[] = {8,9,10,11};
const int mux_pins_max = 4; //sizeof(mux_pins) / sizeof(int);

/* const byte seven_seg_digits[10][7] = { { 1,1,1,1,1,1,0 },  // = 0 */
/*                                        { 0,1,1,0,0,0,0 },  // = 1 */
/*                                        { 1,1,0,1,1,0,1 },  // = 2 */
/*                                        { 1,1,1,1,0,0,1 },  // = 3 */
/*                                        { 0,1,1,0,0,1,1 },  // = 4 */
/*                                        { 1,0,1,1,0,1,1 },  // = 5 */
/*                                        { 1,0,1,1,1,1,1 },  // = 6 */
/*                                        { 1,1,1,0,0,0,0 },  // = 7 */
/*                                        { 1,1,1,1,1,1,1 },  // = 8 */
/*                                        { 1,1,1,0,0,1,1 }   // = 9 */
/* }; */

const byte seven_seg_digits[10] = { B1111110,  // = 0
                                    B0110000,  // = 1
                                    B1101101,  // = 2
                                    B1111001,  // = 3
                                    B0110011,  // = 4
                                    B1011011,  // = 5
                                    B1011111,  // = 6
                                    B1110000,  // = 7
                                    B1111111,  // = 8
                                    B1111011   // = 9
};

const byte mux_code[mux_pins_max] = { B0001,
                                      B0010,
                                      B0100,
                                      B1000
};


//RTCTimer timer;

SimpleTimer timer;


int     digits[mux_pins_max];
boolean dots[mux_pins_max];

int mux_digit=0;
int fade_clock=0;
int fade_amount=0;
tmElements_t      tm;

int active_segment=0;
int active_segment_max=8;

tmElements_t * parseTime (tmElements_t *tm, char *str) 
{
    int hour, minute, sec;
    
    if (sscanf (str, "%d:%d:%d", &hour, &minute, &sec)) {
        tm->Hour   = hour;
        tm->Minute = minute;
        tm->Second = sec;

        return tm;
        
    }
    return NULL;
}

int * update_digits (int digits[mux_pins_max], const tmElements_t & tm)
{
    digits[0] = (tm.Minute % 10);
    digits[1] = (tm.Minute / 10);
    digits[2] = (tm.Hour   % 10);
    digits[3] = (tm.Hour   / 10);

    return digits;
}


void displayDigit (uint8_t mux, uint8_t digit, boolean dot);

int run_count;

void display () 
{
    ++ run_count;
    
    ++mux_digit;
    if (mux_digit >= mux_pins_max) {
        mux_digit = 0;
    }

    displayDigit (mux_digit, digits[mux_digit], dots[mux_digit]);
}

void test_display ()
{
    ++mux_digit;
    if (mux_digit >= mux_pins_max) {
        mux_digit = 0;
    }
    
    byte data = 1 << active_segment;

    digitalWrite (latch_pin, LOW);
    shiftOut (data_pin, clock_pin, LSBFIRST, (byte) 0);
    shiftOut (data_pin, clock_pin, LSBFIRST, (byte) 0);
    digitalWrite (latch_pin, HIGH);

    
    digitalWrite (latch_pin, LOW);
    shiftOut (data_pin, clock_pin, LSBFIRST, data);
    shiftOut (data_pin, clock_pin, LSBFIRST, mux_code[mux_digit]);
    digitalWrite (latch_pin, HIGH);    
}


    
void write_run_count () 
{
    Serial.println (run_count);
    run_count = 0;
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
}


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
//    Serial.begin (9600);
    
    
    for (auto pin : output_pins) {
        pinMode (pin, OUTPUT);
    }
    
    for (auto pin : output_pins) {
        pinMode (pin, OUTPUT);
        digitalWrite (pin, LOW);
    }

    parseTime (&tm, __TIME__);
    
    for (auto & dot : dots) {
        dot = LOW;
    }

    timer.setInterval (1000,[]() 
                       {
                           ++ tm.Second;
                           
                           if (tm.Second >= 60) {
                               
                               tm.Second = 0;
                               ++ tm.Minute;
                               
                               if (tm.Minute >= 60) {
                                   
                                   tm.Minute = 0;
                                   ++ tm.Hour;
                                   
                                   if (tm.Hour >= 24) {
                                       
                                       tm.Hour = 0;
                                   }
                               }
                           }
                           update_digits (digits, tm);
                       });
    

    timer.setInterval (1000,[]()
                       {
                           ++active_segment;
                           if (active_segment >= active_segment_max) {
                               active_segment= 0;
                           }
                       });
    
                       
//    dots[0] = HIGH;
//    dots[2] = HIGH;
//    dots[3] = HIGH;
    
    //timer.setInterval (1, test_display);
    
    timer.setInterval (1, display);
//    timer.setInterval (1000, update_digit_1);
//    timer.setInterval (10000, update_digit_2);
    timer.setInterval (500, blink_dot_generic<2>);
//    timer.setInterval (500, blink_dot_0);
//    timer.setInterval (1000, write_run_count);
    
    //timer.every (5, 
}

static int counter=0;


void shift_out (byte data)
{
    byte reg = PORTD;
    
}

void displayDigit (uint8_t mux, uint8_t digit, boolean dot) 
{
    byte data = (seven_seg_digits[digit] << 1) | (dot ? 1 : B00000000);

    //data = 0xff;

    // digitalWrite (latch_pin, LOW);
    // digitalWrite (oe_pin, LOW);
    // shiftOut (data_pin, clock_pin, LSBFIRST, data);
    // shiftOut (data_pin, clock_pin, LSBFIRST, mux_code[mux]);
    
    digitalWrite (latch_pin, LOW);
    shiftOut (data_pin, clock_pin, LSBFIRST, (byte) 0);
    shiftOut (data_pin, clock_pin, LSBFIRST, (byte) 0);
    //shiftOut (data_pin, clock_pin, LSBFIRST, mux_code[mux]);
    digitalWrite (latch_pin, HIGH);

    //delayMicroseconds(160);
    
    digitalWrite (latch_pin, LOW);

    shiftOut (data_pin, clock_pin, LSBFIRST, data);
    shiftOut (data_pin, clock_pin, LSBFIRST, mux_code[mux]);

    digitalWrite (latch_pin, HIGH);

    
    /* for (int i=0; i < segment_max; ++i) { */
    /*     digitalWrite (output_pins[i], seven_seg_digits[digit][i]); */
    /* } */


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

//    while (true)
    timer.run ();
    
}
