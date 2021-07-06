#include "keyboard.h"

struct keypress
{
    uint8_t _lastkey;
    unsigned long __rpttime;
    unsigned long __dbtime;
    uint8_t __debounced;
} keypress_s;

typedef struct kbdbuf
{
    uint8_t _kbuf[20];
    uint8_t _bfhead;
    uint8_t _bftail;
} kbbuf_t;

typedef struct keyboradHandle
{
    keypress keycontrol[4];
    uint8_t _rowpin[4];
    uint8_t _colpin[4];
    kbbuf_t buffer;
    unsigned int tcnt2;
    uint8_t _scnline;
    unsigned long __repeat_time;
    unsigned long __debounce_time;

} keyboardHandle_t;

void _overflow();
void buf_push(int key);
volatile keyboardHandle_t kb_handle;

/***********************************************************************************************************
 Hardware layer
***********************************************************************************************************/
#if defined(__arm__) && defined(TEENSYDUINO)
static IntervalTimer itimer;
#endif

/*----------------------------------------------------------------------------
 Init kbd control pins and variables
-----------------------------------------------------------------------------*/
keyboard::keyboard(uint8_t kbl1, uint8_t kbl2, uint8_t kbl3, uint8_t kbl4, uint8_t kbr1, uint8_t kbr2, uint8_t kbr3, uint8_t kbr4)
{
    // TODO: inicializr estrutura de controle
    kb_handle._colpin[0] = kbl1;
    kb_handle._colpin[1] = kbl2;
    kb_handle._colpin[2] = kbl3;
    kb_handle._colpin[3] = kbl4;

    kb_handle._rowpin[0] = kbr1;
    kb_handle._rowpin[1] = kbr2;
    kb_handle._rowpin[2] = kbr3;
    kb_handle._rowpin[3] = kbr4;

    timer_set(320, 350);

    /*------------------------------
    Init control pins and variables
    --------------------------------*/
    pinMode(kb_handle._colpin[0], INPUT);
    pinMode(kb_handle._colpin[1], INPUT);
    pinMode(kb_handle._colpin[2], INPUT);
    pinMode(kb_handle._colpin[3], INPUT);

    pinMode(kb_handle._rowpin[0], OUTPUT);
    pinMode(kb_handle._rowpin[1], OUTPUT);
    pinMode(kb_handle._rowpin[2], OUTPUT);
    pinMode(kb_handle._rowpin[3], OUTPUT);

    float prescaler = 0.0;

#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega48__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB1286__)
    TIMSK2 &= ~(1 << TOIE2);
    TCCR2A &= ~((1 << WGM21) | (1 << WGM20));
    TCCR2B &= ~(1 << WGM22);
    ASSR &= ~(1 << AS2);
    TIMSK2 &= ~(1 << OCIE2A);

    if ((F_CPU >= 1000000UL) && (F_CPU <= 16000000UL))
    { // prescaler set to 64
        TCCR2B |= (1 << CS22);
        TCCR2B &= ~((1 << CS21) | (1 << CS20));
        prescaler = 64.0;
    }
    else if (F_CPU < 1000000UL)
    { // prescaler set to 8
        TCCR2B |= (1 << CS21);
        TCCR2B &= ~((1 << CS22) | (1 << CS20));
        prescaler = 8.0;
    }
    else
    { // F_CPU > 16Mhz, prescaler set to 128
        TCCR2B |= ((1 << CS22) | (1 << CS20));
        TCCR2B &= ~(1 << CS21);
        prescaler = 128.0;
    }
#elif defined(__AVR_ATmega8__)
    TIMSK &= ~(1 << TOIE2);
    TCCR2 &= ~((1 << WGM21) | (1 << WGM20));
    TIMSK &= ~(1 << OCIE2);
    ASSR &= ~(1 << AS2);

    if ((F_CPU >= 1000000UL) && (F_CPU <= 16000000UL))
    { // prescaler set to 64
        TCCR2 |= (1 << CS22);
        TCCR2 &= ~((1 << CS21) | (1 << CS20));
        prescaler = 64.0;
    }
    else if (F_CPU < 1000000UL)
    { // prescaler set to 8
        TCCR2 |= (1 << CS21);
        TCCR2 &= ~((1 << CS22) | (1 << CS20));
        prescaler = 8.0;
    }
    else
    { // F_CPU > 16Mhz, prescaler set to 128
        TCCR2 |= ((1 << CS22) && (1 << CS20));
        TCCR2 &= ~(1 << CS21);
        prescaler = 128.0;
    }
#elif defined(__AVR_ATmega128__)
    TIMSK &= ~(1 << TOIE2);
    TCCR2 &= ~((1 << WGM21) | (1 << WGM20));
    TIMSK &= ~(1 << OCIE2);

    if ((F_CPU >= 1000000UL) && (F_CPU <= 16000000UL))
    { // prescaler set to 64
        TCCR2 |= ((1 << CS21) | (1 << CS20));
        TCCR2 &= ~(1 << CS22);
        prescaler = 64.0;
    }
    else if (F_CPU < 1000000UL)
    { // prescaler set to 8
        TCCR2 |= (1 << CS21);
        TCCR2 &= ~((1 << CS22) | (1 << CS20));
        prescaler = 8.0;
    }
    else
    { // F_CPU > 16Mhz, prescaler set to 256
        TCCR2 |= (1 << CS22);
        TCCR2 &= ~((1 << CS21) | (1 << CS20));
        prescaler = 256.0;
    }
#elif defined(__AVR_ATmega32U4__)
    TCCR4B = 0;
    TCCR4A = 0;
    TCCR4C = 0;
    TCCR4D = 0;
    TCCR4E = 0;
    if (F_CPU >= 16000000L)
    {
        TCCR4B = (1 << CS43) | (1 << PSR4);
        prescaler = 128.0;
    }
    else if (F_CPU >= 8000000L)
    {
        TCCR4B = (1 << CS42) | (1 << CS41) | (1 << CS40) | (1 << PSR4);
        prescaler = 64.0;
    }
    else if (F_CPU >= 4000000L)
    {
        TCCR4B = (1 << CS42) | (1 << CS41) | (1 << PSR4);
        prescaler = 32.0;
    }
    else if (F_CPU >= 2000000L)
    {
        TCCR4B = (1 << CS42) | (1 << CS40) | (1 << PSR4);
        prescaler = 16.0;
    }
    else if (F_CPU >= 1000000L)
    {
        TCCR4B = (1 << CS42) | (1 << PSR4);
        prescaler = 8.0;
    }
    else if (F_CPU >= 500000L)
    {
        TCCR4B = (1 << CS41) | (1 << CS40) | (1 << PSR4);
        prescaler = 4.0;
    }
    else
    {
        TCCR4B = (1 << CS41) | (1 << PSR4);
        prescaler = 2.0;
    }
    tcnt2 = (int)((float)F_CPU * 0.001 / prescaler) - 1;
    OCR4C = tcnt2;
    return;
#elif defined(__arm__) && defined(TEENSYDUINO)
    // nothing needed here
#else
#error Unsupported CPU type
#endif

    kb_handle.tcnt2 = 256 - (int)((float)F_CPU * 0.001 / prescaler);
}

/*----------------------------------------------------------------------------
 Configure timer interval to 1ms
-----------------------------------------------------------------------------*/
void keyboard::timer_set(unsigned long debounce_time, unsigned long repeat_time)
{
    kb_handle.__debounce_time = debounce_time;
    kb_handle.__repeat_time = repeat_time;
}

/*----------------------------------------------------------------------------
 Start timer
-----------------------------------------------------------------------------*/
void keyboard::start()
{
    kb_handle._scnline = 0;
    kb_handle.keycontrol[0]._lastkey = 0;
    kb_handle.keycontrol[0].__debounced = 0;
    kb_handle.keycontrol[1]._lastkey = 0;
    kb_handle.keycontrol[1].__debounced = 0;
    kb_handle.keycontrol[2]._lastkey = 0;
    kb_handle.keycontrol[2].__debounced = 0;
    kb_handle.keycontrol[3]._lastkey = 0;
    kb_handle.keycontrol[3].__debounced = 0;

#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega48__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB1286__)
    TCNT2 = kb_handle.tcnt2;
    TIMSK2 |= (1 << TOIE2);
#elif defined(__AVR_ATmega128__)
    TCNT2 = tcnt2;
    TIMSK |= (1 << TOIE2);
#elif defined(__AVR_ATmega8__)
    TCNT2 = tcnt2;
    TIMSK |= (1 << TOIE2);
#elif defined(__AVR_ATmega32U4__)
    TIFR4 = (1 << TOV4);
    TCNT4 = 0;
    TIMSK4 = (1 << TOIE4);
#elif defined(__arm__) && defined(TEENSYDUINO)
    itimer.begin(_overflow, 1000);
#endif
}

/*----------------------------------------------------------------------------
 Stop timer
-----------------------------------------------------------------------------*/
void keyboard::stop()
{
#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega48__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB1286__)
    TIMSK2 &= ~(1 << TOIE2);
#elif defined(__AVR_ATmega128__)
    TIMSK &= ~(1 << TOIE2);
#elif defined(__AVR_ATmega8__)
    TIMSK &= ~(1 << TOIE2);
#elif defined(__AVR_ATmega32U4__)
    TIMSK4 = 0;
#elif defined(__arm__) && defined(TEENSYDUINO)
    itimer.end();
#endif
}

/***********************************************************************************************************
 Functional layer
***********************************************************************************************************/

void buf_push(int key)
{
    uint8_t i = (uint8_t)(kb_handle.buffer._bfhead + 1) % KBD_BUFFER_SIZE;

    // if we should be storing the received character into the location
    // just before the tail (meaning that the head would advance to the
    // current location of the tail), we're about to overflow the buffer
    // and so we don't write the character or advance the head.
    if (i != kb_handle.buffer._bftail)
    {
        kb_handle.buffer._kbuf[kb_handle.buffer._bfhead] = key;
        kb_handle.buffer._bfhead = i;
    }
}

uint8_t keyboard::available(void)
{
    return ((uint8_t)(KBD_BUFFER_SIZE + kb_handle.buffer._bfhead - kb_handle.buffer._bftail)) % KBD_BUFFER_SIZE;
}

uint8_t keyboard::getkey(void)
{
    // if the head isn't ahead of the tail, we don't have any characters
    if (kb_handle.buffer._bfhead == kb_handle.buffer._bftail)
    {
        return 0;
    }
    else
    {
        uint8_t c = kb_handle.buffer._kbuf[kb_handle.buffer._bftail];
        kb_handle.buffer._bftail = (uint8_t)(kb_handle.buffer._bftail + 1) % KBD_BUFFER_SIZE;
        return c;
    }
}

/***********************************************************************************************************
 Interrupt layer
***********************************************************************************************************/

/*----------------------------------------------------------------------------
 Overflow timer
-----------------------------------------------------------------------------*/
void _overflow()
{
    digitalWrite(kb_handle._rowpin[kb_handle._scnline], HIGH);

    uint8_t res = digitalRead(kb_handle._colpin[0]) | (digitalRead(kb_handle._colpin[1]) << 1) | (digitalRead(kb_handle._colpin[2]) << 2) | (digitalRead(kb_handle._colpin[3]) << 3);

    if (res != 0)
    {
        res |= (kb_handle._scnline + 1) << 4;                         // add scan line number
        if (kb_handle.keycontrol[kb_handle._scnline]._lastkey != res) //diferent key from last interrupt
        {
            kb_handle.keycontrol[kb_handle._scnline]._lastkey = res;
            kb_handle.keycontrol[kb_handle._scnline].__dbtime = millis();
            kb_handle.keycontrol[kb_handle._scnline].__rpttime = 0;
            kb_handle.keycontrol[kb_handle._scnline].__debounced = LOW;
        }
        else
        {
            //debounce
            if ((millis() - kb_handle.keycontrol[kb_handle._scnline].__dbtime) >= kb_handle.__debounce_time)
            {
                kb_handle.keycontrol[kb_handle._scnline].__rpttime = millis();
                kb_handle.keycontrol[kb_handle._scnline].__debounced = HIGH;
                buf_push(kb_handle.keycontrol[kb_handle._scnline]._lastkey);
            }
            else if (kb_handle.keycontrol[kb_handle._scnline].__debounced) //debounced
            {
                //repeat time
                if ((millis() - kb_handle.keycontrol[kb_handle._scnline].__rpttime) >= kb_handle.__repeat_time)
                {
                    buf_push(kb_handle.keycontrol[kb_handle._scnline]._lastkey);
                    kb_handle.keycontrol[kb_handle._scnline].__rpttime = millis();
                }
            }
        }
    }
    else
    {
        kb_handle.keycontrol[kb_handle._scnline]._lastkey = 0;
        kb_handle.keycontrol[kb_handle._scnline].__debounced = LOW;
    }
    digitalWrite(kb_handle._rowpin[kb_handle._scnline], LOW);
    kb_handle._scnline = (kb_handle._scnline + 1) % 4;
}

/*----------------------------------------------------------------------------
 ISR timer
-----------------------------------------------------------------------------*/

#if defined(__AVR__)
#if defined(__AVR_ATmega32U4__)
ISR(TIMER4_OVF_vect)
{
#else
ISR(TIMER2_OVF_vect)
{
#endif
#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega48__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB1286__)
    TCNT2 = kb_handle.tcnt2;
#elif defined(__AVR_ATmega128__)
    TCNT2 = tcnt2;
#elif defined(__AVR_ATmega8__)
TCNT2 = tcnt2;
#elif defined(__AVR_ATmega32U4__)
// not necessary on 32u4's high speed timer4
#endif
    _overflow();
}
#endif // AVR
