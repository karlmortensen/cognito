/*
 * Note the names for these pins are based on the motion of a clock. 
 * If the RPi´s are stacked around a circle, the early light is the 
 * first pin the LED the clock hand will hit, followed by middle, then
 * by late. Also note the B leg has the resistor on it. The Excel sheet
 * shows how to map to physical pins.
 */
static const int EARLY_LED_A  = 28;
static const int EARLY_LED_B  = 25;
static const int MIDDLE_LED_A = 4;
static const int MIDDLE_LED_B = 3;
static const int LATE_LED_A   = 15;
static const int LATE_LED_B   = 7;

// static const int MOTION_SENSOR_GROUND = 19; PHYSICAL!
// static const int MOTION_SENSOR_VCC    = 18; PHYSICAL!
static const int MOTION_SENSOR_DATA = 5;

static const int PUSHBUTTON_A = 11;
// static const int PUSHBUTTON_B = 26; PHYSICAL! 

// static const int LIGHT_SENSOR_A = 29; PHYSICAL!
static const int LIGHT_SENSOR_B = 21;

static const int WIFI_A = 29;
// static const int WIFI_B = 40; PHYSICAL!

// KDM these might change direction, depending upon how I fabricate the LEDs
static const int RED_A = 0;
static const int RED_B = 1;

static const int GREEN_A = 1;
static const int GREEN_B = 0;
