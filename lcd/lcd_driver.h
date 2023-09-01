#ifndef _LCD_DRIVER_H
#define _LCD_DRIVER_H

#define LCD_RS(val)     gpio_set_value(gpios[RS].pin, val)
#define LCD_RW(val)     gpio_set_value(gpios[RW].pin, val)
#define LCD_EN(val)     gpio_set_value(gpios[EN].pin, val)

#define LCD_D0(val)     gpio_set_value(gpios[D0].pin, val)
#define LCD_D1(val)     gpio_set_value(gpios[D1].pin, val)
#define LCD_D2(val)     gpio_set_value(gpios[D2].pin, val)
#define LCD_D3(val)     gpio_set_value(gpios[D3].pin, val)

#define LCD_D4(val)     gpio_set_value(gpios[D4].pin, val)
#define LCD_D5(val)     gpio_set_value(gpios[D5].pin, val)
#define LCD_D6(val)     gpio_set_value(gpios[D6].pin, val)
#define LCD_D7(val)     gpio_set_value(gpios[D7].pin, val)

#define CHAR_NUM        32

#define LCD_LINE_1      0x80
#define LCD_LINE_2      0xC0
#define LCD_CLEAR       0x01

#endif
