
#ifndef KEYBOARD_h
#define KEYBOARD_h

#include <avr/interrupt.h>
#include <Arduino.h>

#define KBD_BUFFER_SIZE 0x14

class keyboard
{
public:
    keyboard(uint8_t kbl1, uint8_t kbl2, uint8_t kbl3, uint8_t kbl4, uint8_t kbr1, uint8_t kbr2, uint8_t kbr3, uint8_t kbr4);
    void timer_set(unsigned long debounce_time, unsigned long repeat_time);
    void start();
    void stop();
    uint8_t available(void);
    uint8_t getkey(void);

private:
    unsigned long _interval;
};

#endif