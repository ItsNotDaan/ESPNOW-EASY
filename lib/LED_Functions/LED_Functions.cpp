#include <LED_Functions.h>

// Define the array of leds
CRGB leds[NUM_LEDS];

/// @brief This function will set the color of the LED strip. Red, green, and blue values range from 0 to 255.
/// @param state 
/// @param r 
/// @param g 
/// @param b 
void RGB_LED(bool state, int r, int g, int b) {
  //Hardwired to be GRB.
  if (state) {
    leds[0] = CRGB(g, r, b);
    FastLED.show();
  } else {
    leds[0] = CRGB( 0, 0, 0);
    FastLED.show();
  }
}