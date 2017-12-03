#include "StandardCplusplus.h"
#include <vector>
#include <functional>

#include <Time.h>

#include "SimpleTimer.h"

#include <SPI.h>
#include <Wire.h>
#include "RTClib.h"

RTC_DS1307 rtc;

const int data_pin  = 11;
const int latch_pin = 10;
const int clock_pin = 13;
const int oe_pin    = 6;
const int filament_pwm_pin = 7;

const int output_pins[] = {
    data_pin,
    latch_pin,
    clock_pin,
    oe_pin,
    filament_pwm_pin
};

const int output_pins_max = sizeof(output_pins) / sizeof(int);

/*
       bit 1
        ---
 bit 6 |   | bit 2
       |   |
 bit 7  ---
       |   | bit 3
 bit 5 |   |
        --- . bit 8
       bit 4
*/

const int digits_max   = 4; // number of digits
const int mux_pins_max = digits_max; //sizeof(mux_pins) / sizeof(int);

uint8_t display_matrix[digits_max];  // static table of digits

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

const byte seven_seg_digits[10] = { B11111100,  // = 0
                                    B01100000,  // = 1
                                    B11011010,  // = 2
                                    B11110010,  // = 3
                                    B01100110,  // = 4
                                    B10110110,  // = 5
                                    B10111110,  // = 6
                                    B11100000,  // = 7
                                    B11111110,  // = 8
                                    B11110110   // = 9
};

const byte mux_code[mux_pins_max] = { B10001,
                                      B10010,
                                      B10100,
                                      B11000
};


SimpleTimer      timer;
std::vector<int> timer_numbers;


tmElements_t      tm;

// the model
uint8_t digits[mux_pins_max];
boolean dots  [mux_pins_max];

// timer status
int current_timer;
std::vector<int> current_timers;

/**
 * utility function to cycle thru range <0, max)
 *
template <int max>
class rotary_counter
{
    int counter=0;
    
  public:
    
    inline int operator()()
    {        
        const int v = counter;

        ++ counter;
        
        if (counter >=max) {
            counter = 0;
        }
        
        return v;
    }

    inline operator int () const 
    {
        return counter;
    }
};
*/

void display_digits();
void display_circle();

timer_callback display_callback = display_digits;

/**
 * Display digits callback function.
 */
void display_digits ()
{
    for (auto i = 0; i < mux_pins_max; ++i) {
        display_matrix[i] = seven_seg_digits[digits[i]] | dots[i];
    }        
}

/**
 * Display circle animation
 */
int circle_counter;
int circle_timer;

void display_circle () 
{
    byte frame = circle_counter & 7;
    
    for (auto i = 0; i < mux_pins_max; ++i) {
        display_matrix[i] = (1 << (frame + 1)) | dots[i];
    }        
}

void start_circle () 
{
    display_callback = display_circle;
    circle_counter = 7;

    circle_timer = timer.setInterval (100, []()
                                      {
                                          if (circle_counter <= 0) {
                                              // end loop
                                              timer.deleteTimer (circle_timer);
                                              display_callback = display_digits;
                                          }
                                          -- circle_counter;
                                          display_callback();
                                      });
}

/**
 * error display
 */
void display_error ()
{
    display_matrix[3] = 0b00000000;
    display_matrix[2] = 0b10011110;
    display_matrix[1] = 0b00001010;
    display_matrix[0] = 0b00001010;
}



void update_time (const tmElements_t & tm)
{
    digits[0] = (tm.Minute % 10);
    digits[1] = (tm.Minute / 10);
    digits[2] = (tm.Hour   % 10);
    digits[3] = (tm.Hour   / 10); 

    display_callback ();
}

void blink_dot (int dot) 
{
    if (dots[dot] == LOW) {
        dots[dot] = HIGH;
    } else {
        dots[dot] = LOW;
    }
    display_callback ();
}


template <int the_dot>
void blink_dot_generic (void) 
{
    blink_dot (the_dot);
}

tmElements_t * parseTime (tmElements_t *tm, const char *str) 
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

void get_rtc_time () 
{
    DateTime now = rtc.now();

    tm.Hour   = now.hour();
    tm.Minute = now.minute();
    tm.Second = now.second();

    return tm;
}

/**
 * Utility class. Rotate through available numbers and trigger actions when ticking and reseting
 */
template <typename reset_callback_type, typename tick_callback_type>
class rotary_counter 
{
    const int max;
    int counter=0;
    const reset_callback_type reset_callback;
    const tick_callback_type tick_callback;

public:

    rotary_counter(int max, reset_callback_type reset_callback, tick_callback_type tick_callback)
    :max (max),
     reset_callback (reset_callback),
     tick_callback (tick_callback)
        {}
    
    
    inline int next ()
        {
            
            if (counter == max) {
                reset_callback (counter);
                counter = 0;
            } else {
                tick_callback (counter);
                ++counter;
            }
            
            return counter;
        }            
};

template <typename t, typename q>
inline rotary_counter<t,q> make_rotary_counter (int max, t callback, q tick) 
{
    return rotary_counter<t,q>(max, callback, tick);
}


inline void latch_low()
{
    PORTB &= ~ (1 << PB2);
}

inline void latch_high()
{
    PORTB |= (1 << PB2);
}


const SPISettings spi_settings (20000000, LSBFIRST, SPI_MODE0);

/**
 * Continuousle sweep display_matrix table
 * and display active digit
 */
inline void display_sweep () 
{
    static auto mux = make_rotary_counter (mux_pins_max, [](int){},
                                                      [](int mux_digit) 
                                                      {
                                                          latch_low();
                                                          SPI.beginTransaction(spi_settings);
                                                          SPI.transfer (display_matrix[mux_digit]);
                                                          SPI.transfer (mux_code      [mux_digit]);
                                                          SPI.endTransaction();
                                                          latch_high();
                                                      });
    
    

    mux.next();
}

inline void blank_display ()
{
    latch_low();
    SPI.beginTransaction(spi_settings);
    SPI.transfer (0);
    SPI.transfer (0);    
    SPI.endTransaction();
    latch_high();
}

const int pwm_value=15;

auto filament_counter = make_rotary_counter (1,[](int)
                                             {
                                                 PORTD |= (1 << filament_pwm_pin);
                                             },
                                             [](int) {
                                                 PORTD &= ~ (1 << filament_pwm_pin);
                                             });
                                             
auto sweep_counter = make_rotary_counter(16, [](int) 
                                         {
                                             display_sweep();
                                         },
                                         [](int slot)
                                         {
                                             if (slot == pwm_value) {
                                                 blank_display();
                                             }
                                         });

                                         


// Timer 1 interrupt service routine
ISR(TIMER1_COMPA_vect)
{
    sweep_counter.next();
    filament_counter.next();
}

void setup () 
{

    for (auto pin : output_pins) {
        pinMode (pin, OUTPUT);
    }
    digitalWrite(filament_pwm_pin, LOW);
    digitalWrite(oe_pin, LOW);

    for (auto & digit : digits) digit=0;
    for (auto & digit : display_matrix) digit=0;
    for (auto & dot   : dots  ) dot  =0;
 
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
    OCR1A  = 10-1;       // 6250 Hz (62500/10)
    TCNT1  = 0;
    TIMSK1 = 0b00000010;
    TIFR1  = 0b00000000;
    TCCR1B = 0b00001100;  // Enable timer

    sei();

    SPI.begin();
    
    //parseTime (&tm, __TIME__);
    
    //update_time (tm);

    timer_numbers.push_back (timer.setInterval (1000,  []() {
                
                ++ tm.Second;
                           
                if (tm.Second >= 60) {
                    
                    tm.Second = 0;
                    ++ tm.Minute;

                    start_circle();
                    
                    if (tm.Minute >= 60) {
                        
                        tm.Minute = 0;
                        ++ tm.Hour;
                        
                        if (tm.Hour >= 24) {
                            
                            tm.Hour = 0;
                        }
                    }
                }
                update_time (tm);
            }));
    
    timer_numbers.push_back (timer.setInterval (500, blink_dot_generic<2>));


    //timer.setInterval (1000, []() 
    //                   {
    //                       static rotary_counter<3> counter;
    //                       pwm_value = counter () + 1;
    //                   }                   );
    
    
    if (! rtc.begin()) {
        display_callback = display_error;
    }

    if (! rtc.isrunning()) {
        display_callback = display_error;
        //Serial.println("RTC is NOT running!");
    }

    // following line sets the RTC to the date & time this sketch was compiled
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    
    timer_numbers.push_back (timer.setInterval (5000, get_rtc_time));
    
    get_rtc_time ();
    update_time (tm);
    
    display_callback();
}

void loop () 
{
    timer.run ();    
}


// Local Variables:
// mode: arduino
// End:
