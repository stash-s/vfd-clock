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


byte    tube_toggle   = 0;

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


// Timer 1 interrupt service routine

ISR(TIMER1_COMPA_vect)
{
  // Drive tubes at 250 Hz
  tube_toggle ^= 1;
  if (tube_toggle) display();

}


void setup () 
{
    for (auto pin : output_pins) {
        pinMode (pin, OUTPUT);
        digitalWrite (pin, LOW);
    }

    // TCCR1A - Timer/Counter Control Register A: (default 00000000b)
    // * Bit 7..6: COM1A[1..0]: Compare Match Output A Mode
    // * Bit 5..4: COM1B[1..0]: Compare Match Output B Mode
    // * Bit 3..2: Reserved
    // * Bit 1..0: WGM1[1..0]: Waveform Generation Mode
    //
    // TCCR1B - Timer/Counter Control Register B: (default 00000000b)
    // * Bit    7: ICNC1: Input Capture Noise Canceler
    // * Bit    6: ICES1: Input Capture Edge Select
    // * Bit    5: Reserved
    // * Bit 4..3: WGM1[3..2]: Waveform Generation Mode
    // * Bit 2..0: CS1[2..0]: Clock Select
    //
    // TCCR1C - Timer/Counter Control Register C: (default 00000000b)
    // * Bit    7: FOC1A: Force Output Compare for Channel A
    // * Bit    6: FOC1B: Force Output Compare for Channel B
    // * Bit 5..0: Reserved
    //
    // TCNT1H and TCNT1L - Timer/Counter Register: (default 0000000000000000b)
    // * Bit 15..0: TCNT1[15..0]: timer/counter value
    //
    // OCR1AH and OCR1AL - Output Compare Register A: (default 0000000000000000b)
    // * Bit 15..0: OCR1A[15..0]: compare value A
    //
    // OCR1BH and OCR1BL - Output Compare Register B: (default 0000000000000000b)
    // * Bit 15..0: OCR1B[15..0]: compare value B
    //
    // ICR1H and ICR1L - Input Capture Register: (default 0000000000000000b)
    // * Bit 15..0: ICR1[15..0]: capture value B
    //
    // TIMSK1 - Timer/Counter Interrupt Mask Register: (default 00000000b)
    // * Bit 7..6: Reserved
    // * Bit    5: ICIE1: Input Capture Interrupt Enable
    // * Bit 4..3: Reserved
    // * Bit    2: OCIE1B: Output Compare B Match Interrupt Enable
    // * Bit    1: OCIE1A: Output Compare A Match Interrupt Enable
    // * Bit    0: TOIE1: Overflow Interrupt Enable
    //
    // TIFR1 - Timer/Counter Interrupt Flag Register: (default 00000000b)
    // * Bit 7..6: Reserved
    // * Bit    5: ICF1: Input Capture Flag
    // * Bit 4..3: Reserved
    // * Bit    2: OCF1B: Output Compare B Match Flag
    // * Bit    1: OCF1A: Output Compare A Match Flag
    // * Bit    0: TOV1: Overflow Flag
    //
    // Settings:
    // - WGM1[3..0]=0100b: CTC (top value OCR1A, overflow on MAX)
    //   Non-PWM mode:
    //   - COM1A[1..0]=00b: Normal port operation, OC1A disconnected
    //   - COM1B[1..0]=00b: Normal port operation, OC1B disconnected
    // - CS1[2..0]=100b: clk/256 speed => 62500 Hz
    //                   -> a non-zero value also enables the timer/counter
    // - OCIE1A=1: Generate interrupt on output compare A match

    cli();

    // We've observed on Arduino IDE 1.5.8 that TCCR1A is non-zero at this point. So let's play safe
    // and write all relevant timer registers.
    TCCR1A = 0b00000000;
    OCR1A  = 125-1;       // 500 Hz (62500/125)
    TCNT1  = 0;
    TIMSK1 = 0b00000010;
    TIFR1  = 0b00000000;
    TCCR1B = 0b00001100;  // Enable timer

    sei();
    
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
    
                       
//    timer.setInterval (1, display);
    timer.setInterval (500, blink_dot_generic<2>);
}

static int counter=0;


void shift_out (byte data)
{
    byte reg = PORTD;
    
}

void displayDigit (uint8_t mux, uint8_t digit, boolean dot) 
{
    byte data = (seven_seg_digits[digit] << 1) | (dot ? 1 : B00000000);

    
    digitalWrite (latch_pin, LOW);
    shiftOut (data_pin, clock_pin, LSBFIRST, (byte) 0);
    shiftOut (data_pin, clock_pin, LSBFIRST, (byte) 0);
    digitalWrite (latch_pin, HIGH);

    // The following function is compiled to one or more nested loops that run for the exact amount
    // of cycles
    __builtin_avr_delay_cycles(16*40);  // 40 us (at 16 MHz)
    
    digitalWrite (latch_pin, LOW);
    shiftOut (data_pin, clock_pin, LSBFIRST, data);
    shiftOut (data_pin, clock_pin, LSBFIRST, mux_code[mux]);
    digitalWrite (latch_pin, HIGH);
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
