#include <Arduino.h>
#include <FastLED.h>

/*
In the LED.h file, we define the following variables and functions:

Variables:
DATA_PIN: This is the pin that the LED (strip) is connected to.
NUM_LEDS: The number of LEDs in the strip.
BRIGHTNESS: The brightness of the LEDs. 0 is the lowest (off), 255 is the highest.
LED_TYPE: The type of LED strip that you are using. The most common ones are WS2812B, WS2811, and WS2812.
UPDATES_PER_SECOND: The number of times the loop function will run per second. This is used to control the speed of the animations. The higher the number, the faster the animations will be.
CRGB leds[NUM_LEDS]: This is an array that will store the color values of each LED in the strip. You can access each LED by its index in the array. For example, leds[0] will give you the first LED in the strip.

Functions:
RGB_LED(bool state, int r, int g, int b): This is a function that will set the color of the LED strip. It takes three arguments: state, r, g, and b. The state argument is a boolean that will turn the LED strip on or off. The r, g, and b arguments are integers that represent the red, green, and blue values of the color that you want to set the LED strip to. The values range from 0 to 255. For example, RGB_LED(true, 255, 0, 0) will turn the LED strip on and set it to red.
*/

#define DATA_PIN    7 //This is the pin that the LED (strip) is connected to.
#define NUM_LEDS    1
#define BRIGHTNESS  30
#define LED_TYPE    WS2812B

// Define the array of leds
#define UPDATES_PER_SECOND 100

extern CRGB leds[NUM_LEDS];

void RGB_LED(bool state, int r, int g, int b);  	 