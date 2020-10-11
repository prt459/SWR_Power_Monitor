/*  
Arduino Nano script for homebrew SWR/ power meter.  Intended for a low-band solid state AM transmitter.   
Written by Paul Taylor, VK3HN (https://vk3hn.wordpress.com/)

V1.0, 11 Oct 2020 - first version.

This script was coded to use the directional coupler described in:
'A PIC16F876 based, automatic 1.8 – 60 MHz SWR/WATTmeter' a project from “Il Club Autocostruttori” 
of the Padova ARI club' by IW3EGT and IK3OIL, see:  
http://www.ik3oil.it/_private/Articolo_SWR_eng.pdf  

No attempt was made to write code to accurately determine RF power across a wide range of SWRs and frequencies. 
Instead, a set of power readings from the station transmitter and the correspondng analogRead values (0..1024)
was mapped into an approximation using the curve fitting calculator at https://www.dcode.fr/equation-finder 

No claim to either SWR or power accuracy is made; this is an indicative tool, not an accurate one.   

The main purpose was to provide a digital readout of approximate power and SWR, and to implement 
a 'High SWR' interlock to reduce power in the presence of high SWR.  The time taken to detect and raise this 
interlock is dependent on several factors including the processing speed of the Nano; it is in the 
10s of milliseconds range; this may not suit or even protect your transmitter!  

All constants are #define'd.  
The displays dim after a configurable period of inactivity. 
The High SWR interlock resets when the SWR drops below the threshold. 
Analog reads from the forward and reflected coupler ports are buffered and averaged. 


Comments, feedback, improvements, stories welcome. 

Paul VK3HN.  https://vk3hn.wordpress.com/  

*/

#include <Arduino.h>
#include <TM1637Display.h>

// Module connection pins (Digital Pins)
#define CLK_L 4
#define DIO_L 5
#define CLK_R 2
#define DIO_R 3

const uint8_t SEG_DONE[] = {
	SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
	SEG_C | SEG_E | SEG_G,                           // n
	SEG_A | SEG_D | SEG_E | SEG_F | SEG_G            // E
	};

const uint8_t SEG_S[] = {
  SEG_A | SEG_F | SEG_G | SEG_C | SEG_D,           // S
  0,   
  0,
  0        
  };

const uint8_t SEG_P[] = {
  SEG_A | SEG_B | SEG_G | SEG_F | SEG_E,           // P
  0,   
  0,
  0        
  };

TM1637Display display_left(CLK_L, DIO_L);
TM1637Display display_right(CLK_R, DIO_R);


bool mode_tx = false;
bool user_active = true; 
unsigned long this_mS = 0;         // mS counter  
unsigned long last_mS = millis();  // mS counter  
unsigned int tick = 0; 
unsigned int inactivity_ticks = 1;  // counts the ticks of inactivity
#define INACTIVE_SECS 6

#define DISPLAY_REFRESH_MS 500    // period between display refresheds in mS

#define BRIGHTNESS_MIN 0x01
#define BRIGHTNESS_MAX 0x0f
byte br = BRIGHTNESS_MAX; 

int swr, pwr;
unsigned long swr_last = 0;   // counter for milli seconds since last SWR display refresh
unsigned long pwr_last = 0;   // counter for milli seconds since last power display refresh

#define PWR_REFRESH_MS  600
#define SWR_REFRESH_MS  200

// High SWR interlock settings
#define SWR_HIGH_INTERLOCK_INPUT 2    // digital output to raise when the SWR reaches the unacceptable threshold  
#define SWR_HIGH_INTERLOCK_THRESHOLD 12  // SWR times 10, at which the interlock kicks in 
bool swr_high = false;  
bool display_blink = false; 

// analog input smoothing settings
#define NBR_READINGS 8      // number of analog readings in the smoothing buffer 

// struct and array for smooting of the two analog input readings 
struct smooth_t {
  int r[NBR_READINGS]; 
};

smooth_t smooth[] = {
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
};

int smooth_index[] = {0, 0, 0};     // current indexes into circular buffers



int averaged_analogRead(byte pin_nbr){
  // reads pin_no and adds reading to a buffer; 
  // pin_nbr can be 1 or 2
  // returns the average across the buffer to implement smoothing

  int r=0;
  r = analogRead(pin_nbr);
  smooth[pin_nbr].r[smooth_index[pin_nbr]++] = r;
  
  if(smooth_index[pin_nbr] == NBR_READINGS) smooth_index[pin_nbr] = 0;  // wrap around

  // now, return the average
  int mean=0, ret = 0;
  for (byte n=0; n<NBR_READINGS; n++)
    mean += smooth[pin_nbr].r[n]; 

  ret = mean/NBR_READINGS;
//  Serial.print("
  return ret;
}



void refresh_display()
{
  mode_tx = false;
  if (swr > 0) 
  {
    mode_tx = true;
    user_active = true;
    if(abs(millis() - swr_last) > SWR_REFRESH_MS) {
      // display the SWR reading
      display_left.showNumberDecEx(swr, (0x80 >> 2), false);
      swr_last = millis();
    }
//    swr_last = swr;
  }
  else
  {
     display_left.setSegments(SEG_S); 
  };

  if (pwr > 0) 
  {
    mode_tx = true;
    user_active = true;

    if(swr_high || (abs(millis() - pwr_last) > PWR_REFRESH_MS)) {
      // display the power reading
      display_right.showNumberDec(pwr, false, 4, 0); 
      pwr_last = millis();
    }   
  }
  else
  {
     display_right.setSegments(SEG_P); 
  }
}


void setup()
{
  Serial.begin(9600);
  
  display_left.setBrightness(BRIGHTNESS_MAX);
  display_right.setBrightness(BRIGHTNESS_MAX);

  display_left.clear();
  display_right.clear();

  display_left.setSegments(SEG_S);
  display_right.setSegments(SEG_P);
  delay(1000); 
  
  display_left.clear();
  display_right.clear();
  
//  mode_tx = true; 

  swr_last = millis();
  pwr_last = millis();

  pinMode(SWR_HIGH_INTERLOCK_INPUT, OUTPUT);   // high SWR interlock
  digitalWrite(SWR_HIGH_INTERLOCK_INPUT, 0);   // disable the interlock    
}



void loop()
{
  int fwd, ref, fwd_mV, ref_mV; 

  //--------------------------------------------------------------
  // see if it is a second boundary 
  this_mS = millis(); 
  if(abs(this_mS - last_mS)>1000)
  {
    last_mS = this_mS;
    tick++;             // tick is the second counter
    if(!mode_tx) inactivity_ticks++;
  }
  //  Serial.print("activity_ticks="); Serial.println(activity_ticks); 

  // see if it is time to sleep 
  if(!mode_tx && user_active && (inactivity_ticks % INACTIVE_SECS == 0)) {
    user_active = false; // transition from active to inactive 
    inactivity_ticks = 1;
    //    Serial.println("going to sleep. ");
    // dim displays

    br = BRIGHTNESS_MIN;     
  }

  if(user_active) br = BRIGHTNESS_MAX;  // if transmitting or still active we want full brightness   
  //  Serial.print("br="); Serial.println(br);
  
  display_left.setBrightness(br);    
  display_right.setBrightness(br);
  
  // -------------------------------------------------------------

  fwd = analogRead(A1);
  ref = analogRead(A2);
  
//  fwd = averaged_analogRead(1);
//  ref = averaged_analogRead(2);

 // delay(10);

  fwd_mV = float(fwd)/204.8 * 1000.0;  // scale to milli-volts
  ref_mV = float(ref)/204.8 * 1000.0;

  // apply the diode compensation

  fwd_mV = fwd_mV - 400;  
  if(fwd_mV <0 ) fwd_mV = 0;
  
  ref_mV = ref_mV - 400;  
  if(ref_mV <0 ) ref_mV = 0;
  

  // calculate SWR
  float swr_f = (float(fwd_mV) + float(ref_mV)) / (float(fwd_mV) - float(ref_mV));
  swr = swr_f * 10.0; 
    
  // calculate power (only just before a display refresh to avoid unnecessary floating point calculations!)
  // The following power equation was fitted using one set of data points on the preferred band and frequency 
  // with the calculator at https://www.dcode.fr/equation-finder 
  float x = fwd_mV;
  pwr = ( 
                 ( ((1433.0*(x-2750.0))/264362664930.0) + 31.0/9131304.0) * (x-2100.0) 
                    + 15.0/452.0) * 
                 (x - 1196.0) + 20.0; 

  if(pwr < 0) pwr = 0; // clean up any -ve values at rest  
//  Serial.print("fwd="); Serial.print(fwd);
//  Serial.print("  ref="); Serial.print(ref);
//  Serial.print(" / fwd_mV="); Serial.print(fwd_mV);
//  Serial.print("  ref_mV="); Serial.print(ref_mV);
  Serial.print("    swr="); Serial.print(swr);
  Serial.print(", pwr="); Serial.println(pwr);

//  refresh_display();  

  // test for the HIGH_SWR_INTERLOCK

  if((!swr_high) && (swr >= SWR_HIGH_INTERLOCK_THRESHOLD)){
    // first time we detected an unacceptable SWR, raise the interlock line
    digitalWrite(SWR_HIGH_INTERLOCK_INPUT, 1);
    swr_high = true;    
    Serial.println("!!! HIGH SWR !!!    INTERLOCK ON");
  };

  if((swr_high) && (swr < SWR_HIGH_INTERLOCK_THRESHOLD)){
    // first time we detected the SWR has dropped, so lower the interlock line
    digitalWrite(SWR_HIGH_INTERLOCK_INPUT, 0);
    swr_high = false;
    // un-blink the display! 
    display_left.setBrightness(BRIGHTNESS_MAX, 1);
    Serial.println("!!!      SWR normal interlock OFF");
  };


  if(swr_high){
    // blink the display! 
    if(display_blink) 
      display_left.setBrightness(BRIGHTNESS_MAX, 1);
    else 
      display_left.setBrightness(BRIGHTNESS_MAX, 0);
    display_blink = !display_blink;
    delay(20);
  };
  
  refresh_display();  
}
