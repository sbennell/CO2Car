/*================================================================================*
   Pinewood Derby Timer                                Version 3.11 - 15 APR 2025
    www.dfgtec.com/pdt

   Flexible and affordable Pinewood Derby timer that interfaces with the 
   following software:
     - PD Test/Tune/Track Utility
     - Grand Prix Race Manager software

   Refer to the website for setup and usage instructions.
   
 Version 3.11 - 15 APR 2025  
 - Updated for ESP32 platform
 - Added support for VL53L0X distance sensors for finish line detection
 - Updated pin definitions for ESP32 hardware
 - Modified solenoid control: inverted logic, auto-off after 100ms
 - Added auto-start functionality when 'S' command is received
 - Improved debug messaging

   Copyright (C) 2011-2020 David Gadberry Modfided by Steweartb for ESP32 and VL53L0X distance sensors
 * This work is licensed under the Creative Commons Attribution-NonCommercial-
 * ShareAlike 3.0 Unported License. To view a copy of this license, visit
 * http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to 
 * Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 
 * 94041, USA.
 *================================================================================*/


/*-----------------------------------------*
  - TIMER CONFIGURATION -
 *-----------------------------------------*/
#define NUM_LANES    2                 // number of lanes
#define GATE_RESET   0                 // Enable closing start gate to reset timer

//#define LED_DISPLAY  1                 // Enable lane place/time displays
//#define DUAL_DISP    1                 // dual displays per lane (4 lanes max)
//#define DUAL_MODE    1                 // dual display mode
//#define LARGE_DISP   1                 // utilize large Adafruit displays (see website)

#define SHOW_PLACE   1                 // Show place mode
#define PLACE_DELAY  3                 // Delay (secs) when displaying place/time
#define MIN_BRIGHT   0                 // minimum display brightness (0-15)
#define MAX_BRIGHT   15                // maximum display brightness (0-15)

// VL53L0X sensor configuration
#define DISTANCE_THRESHOLD 150        // Distance in mm to detect car crossing finish line
#define SENSOR_TIMING_BUDGET 33000    // Timing budget in microseconds

/*-----------------------------------------*
  - END -
 *-----------------------------------------*/

#ifdef LED_DISPLAY                     // LED control libraries
#include "Wire.h"
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"
#endif

// VL53L0X sensor library
#include <Wire.h>
#include <VL53L0X.h>

/*-----------------------------------------*
  - static definitions -
 *-----------------------------------------*/
#define PDT_VERSION  "3.11"            // software version - updated for ESP32
#define MAX_LANE     6                 // maximum number of lanes (ESP32)
#define MAX_DISP     8                 // maximum number of displays (Adafruit)

#define mREADY       0                 // program modes
#define mRACING      1
#define mFINISH      2
#define mTEST        3

#define START_TRIP   LOW               // start switch trip condition
#define NULL_TIME    99.999            // null (non-finish) time
#define NUM_DIGIT    4                 // timer resolution (# of decimals)
#define DISP_DIGIT   4                 // total number of display digits

#define PWM_LED_ON   220
#define PWM_LED_OFF  255
#define char2int(c) (c - '0') 

//
// serial messages                        <- to timer
//                                        -> from timer
//
#define SMSG_ACKNW   '.'               // -> acknowledge message

#define SMSG_POWER   'P'               // -> start-up (power on or hard reset)

#define SMSG_CGATE   'G'               // <- check gate
#define SMSG_GOPEN   'O'               // -> gate open

#define SMSG_RESET   'R'               // <- reset
#define SMSG_READY   'K'               // -> ready

#define SMSG_SOLEN   'S'               // <- start solenoid
#define SMSG_START   'B'               // -> race started
#define SMSG_FORCE   'F'               // <- force end
#define SMSG_RSEND   'Q'               // <- resend race data

#define SMSG_LMASK   'M'               // <- mask lane
#define SMSG_UMASK   'U'               // <- unmask all lanes

#define SMSG_GVERS   'V'               // <- request timer version
#define SMSG_DEBUG   'D'               // <- toggle debug on/off
#define SMSG_GNUML   'N'               // <- request number of lanes
#define SMSG_TINFO   'I'               // <- request timer information

/*-----------------------------------------*
  - pin assignments -
 *-----------------------------------------*/
// Pin definitions for ESP32
#define I2C_SDA             21
#define I2C_SCL             22
#define SENSOR1_XSHUT       16  // Address: 0x30
#define SENSOR2_XSHUT       17  // Address: 0x31
#define START_GATE          4   // With internal pullup
#define START_SOL           14  // Always HIGH, goes LOW when activated
#define RESET_SWITCH        13  // Active LOW
#define STATUS_LED_R        25  // Active HIGH (LED_RED)
#define STATUS_LED_G        26  // Active HIGH (LED_GREEN)
#define STATUS_LED_B        33  // Active HIGH (LED_BLUE)

int  BRIGHT_LEV   = 34;                // brightness level (if connected)

//                Display #    1     2     3     4     5     6     7     8
int  DISP_ADD [MAX_DISP] = {0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77};    // display I2C addresses

/*-----------------------------------------*
  - global variables -
 *-----------------------------------------*/
bool          fDebug = false;          // debug flag
bool          ready_first;             // first pass in ready state flag
bool          finish_first;            // first pass in finish state flag

unsigned long start_time;              // race start time (microseconds)
unsigned long lane_time  [MAX_LANE];   // lane finish time (microseconds)
int           lane_place [MAX_LANE];   // lane finish place
bool          lane_mask  [MAX_LANE];   // lane mask status

int           serial_data;             // serial data
uint8_t       mode;                    // current program mode

float         display_level = -1.0;    // display brightness level

// VL53L0X sensor objects
VL53L0X sensor1;
VL53L0X sensor2;
uint16_t sensor_distance[2];           // Current distances measured by sensors

#ifdef LARGE_DISP
unsigned char msgGateC[] = {0x6D, 0x41, 0x00, 0x0F, 0x07};  // S=CL
unsigned char msgGateO[] = {0x6D, 0x41, 0x00, 0x3F, 0x5E};  // S=OP
unsigned char msgLight[] = {0x41, 0x41, 0x00, 0x00, 0x07};  // == L
unsigned char msgDark [] = {0x41, 0x41, 0x00, 0x00, 0x73};  // == d
#else
unsigned char msgGateC[] = {0x6D, 0x48, 0x00, 0x39, 0x38};  // S=CL
unsigned char msgGateO[] = {0x6D, 0x48, 0x00, 0x3F, 0x73};  // S=OP
unsigned char msgLight[] = {0x48, 0x48, 0x00, 0x00, 0x38};  // == L
unsigned char msgDark [] = {0x48, 0x48, 0x00, 0x00, 0x5e};  // == d
#endif
unsigned char msgDashT[] = {0x40, 0x40, 0x00, 0x40, 0x40};  // ----
unsigned char msgDashL[] = {0x00, 0x00, 0x00, 0x40, 0x00};  //   -
unsigned char msgBlank[] = {0x00, 0x00, 0x00, 0x00, 0x00};  // (blank)

#ifdef LED_DISPLAY                     // LED display control
Adafruit_7segment disp_mat[MAX_DISP];
#endif

#ifdef DUAL_MODE                       // uses 8x8 matrix displays
Adafruit_8x8matrix disp_8x8[MAX_DISP];
#endif

void initialize(bool powerup=false);
void dbg(int, const char * msg, int val=-999);
void smsg(char msg, bool crlf=true);
void smsg_str(const char * msg, bool crlf=true);
void setupSensors();
bool checkFinishLineCrossed(int lane);

/*================================================================================*
  SETUP TIMER
 *================================================================================*/
void setup()
{  
/*-----------------------------------------*
  - hardware setup -
 *-----------------------------------------*/
  Serial.begin(9600);
  smsg_str("ESP32 Pinewood Derby Timer starting...");
  
  // Initialize I2C for VL53L0X sensors
  Wire.begin(I2C_SDA, I2C_SCL);
  
  pinMode(STATUS_LED_R, OUTPUT);
  pinMode(STATUS_LED_B, OUTPUT);
  pinMode(STATUS_LED_G, OUTPUT);
  pinMode(START_SOL,    OUTPUT);
  pinMode(RESET_SWITCH, INPUT_PULLUP);
  pinMode(START_GATE,   INPUT_PULLUP);
  pinMode(BRIGHT_LEV,   INPUT);
  pinMode(SENSOR1_XSHUT, OUTPUT);
  pinMode(SENSOR2_XSHUT, OUTPUT);
  
  digitalWrite(RESET_SWITCH, HIGH);    // enable pull-up resistor
  digitalWrite(START_GATE,   HIGH);    // enable pull-up resistor

  // Initialize the VL53L0X sensors
  setupSensors();

#ifdef LED_DISPLAY
  for (int n=0; n<MAX_DISP; n++)
  {
    disp_mat[n] = Adafruit_7segment();
    disp_mat[n].begin(DISP_ADD[n]);
    disp_mat[n].clear();
    disp_mat[n].drawColon(false);
    disp_mat[n].writeDisplay();

#ifdef DUAL_MODE
    disp_8x8[n] = Adafruit_8x8matrix();
    disp_8x8[n].begin(DISP_ADD[n]);
    disp_8x8[n].clear();
    disp_8x8[n].writeDisplay();
#endif
  }
#endif

  set_display_brightness();

/*-----------------------------------------*
  - software setup -
 *-----------------------------------------*/
  smsg(SMSG_POWER);

/*-----------------------------------------*
  - check for test mode -
 *-----------------------------------------*/
  if (digitalRead(RESET_SWITCH) == LOW)
  {
    mode = mTEST;
    test_pdt_hw();
  }

/*-----------------------------------------*
  - initialize timer -
 *-----------------------------------------*/
  initialize(true);
  unmask_all_lanes();
}

/*================================================================================*
  SETUP VL53L0X SENSORS
 *================================================================================*/
void setupSensors() {
  // Reset both sensors by setting XSHUT pins low
  digitalWrite(SENSOR1_XSHUT, LOW);
  digitalWrite(SENSOR2_XSHUT, LOW);
  delay(10);
  
  // Enable sensor 1
  digitalWrite(SENSOR1_XSHUT, HIGH);
  delay(10);
  sensor1.setTimeout(500);
  if (!sensor1.init()) {
    Serial.println("Failed to initialize sensor 1!");
  }
  
  // Set sensor 1 address to 0x30
  sensor1.setAddress(0x30);
  
  // Enable sensor 2
  digitalWrite(SENSOR2_XSHUT, HIGH);
  delay(10);
  sensor2.setTimeout(500);
  if (!sensor2.init()) {
    Serial.println("Failed to initialize sensor 2!");
  }
  
  // Configure both sensors for better accuracy
  sensor1.setMeasurementTimingBudget(SENSOR_TIMING_BUDGET);
  sensor2.setMeasurementTimingBudget(SENSOR_TIMING_BUDGET);
  
  // Start continuous back-to-back measurement
  sensor1.startContinuous();
  sensor2.startContinuous();
}

/*================================================================================*
  CHECK IF CAR HAS CROSSED FINISH LINE
 *================================================================================*/
bool checkFinishLineCrossed(int lane) {
  // Read the respective sensor based on lane
  uint16_t distance = 0;
  
  if (lane == 0) {
    distance = sensor1.readRangeContinuousMillimeters();
  } else if (lane == 1) {
    distance = sensor2.readRangeContinuousMillimeters();
  }
  
  // Store the current distance
  sensor_distance[lane] = distance;
  
  // Check if distance is below threshold (car is detected)
  return (distance < DISTANCE_THRESHOLD && !sensor1.timeoutOccurred());
}

/*================================================================================*
  MAIN LOOP
 *================================================================================*/
void loop()
{
  process_general_msgs();

  switch (mode)
  {
    case mREADY:
      timer_ready_state();
      break;
    case mRACING:
      timer_racing_state();
      break;
    case mFINISH:
      timer_finished_state();
      break;
  }
}


/*================================================================================*
  TIMER READY STATE
 *================================================================================*/
void timer_ready_state()
{
  if (ready_first)
  {
    set_status_led();
    clear_displays();

    ready_first = false;
  }

  if (serial_data == int(SMSG_SOLEN))    // activate start solenoid and start race immediately
  {
    digitalWrite(START_SOL, LOW);  // Activate solenoid (LOW)
    smsg(SMSG_ACKNW);
    
    // Only keep solenoid on for 100ms
    delay(100);
    digitalWrite(START_SOL, HIGH);  // Deactivate solenoid after brief activation
    
    // Start race immediately
    start_time = micros();
    smsg(SMSG_START);
    
    mode = mRACING;
    return;  // Exit immediately to start racing
  }
  
  // Keep this code for manual starting with the button
  if (digitalRead(START_GATE) == START_TRIP)    // timer start via button
  {
    start_time = micros();
    digitalWrite(START_SOL, HIGH);  // Deactivate solenoid (HIGH)

    smsg(SMSG_START);
    delay(100); 

    mode = mRACING; 
  }

  return;
}
  

/*================================================================================*
  TIMER RACING STATE
 *================================================================================*/
void timer_racing_state()
{
  int lanes_left, finish_order;
  unsigned long current_time, last_finish_time;


  set_status_led();
  clear_displays();

  finish_order = 0;
  last_finish_time = 0;

  lanes_left = NUM_LANES;
  for (int n=0; n<NUM_LANES; n++)
  {
    if (lane_mask[n]) lanes_left--;
  }

  while (lanes_left)
  {
    current_time = micros();

    for (int n=0; n<NUM_LANES; n++)
    {
      // Check if car has crossed finish line using VL53L0X sensors
      if (lane_time[n] == 0 && checkFinishLineCrossed(n) && !lane_mask[n])    // car has crossed finish line
      {
        lanes_left--;

        lane_time[n] = current_time - start_time;

        if (lane_time[n] > last_finish_time)
        {
          finish_order++;
          last_finish_time = lane_time[n];
        }
        lane_place[n] = finish_order;

        update_display(n, lane_place[n], lane_time[n], SHOW_PLACE);
      }
    }
    
    serial_data = get_serial_data();

    if (serial_data == int(SMSG_FORCE) || serial_data == int(SMSG_RESET) || digitalRead(RESET_SWITCH) == LOW)    // force race to end
    {
      lanes_left = 0;
      smsg(SMSG_ACKNW);
    }
  }
    
  send_race_results();

  mode = mFINISH;

  return;
}


/*================================================================================*
  TIMER FINISHED STATE
 *================================================================================*/
void timer_finished_state()
{
  if (finish_first)
  {
    set_status_led();
    finish_first = false;
  }

  if (GATE_RESET && digitalRead(START_GATE) != START_TRIP)    // gate closed
  {
    delay(500);    // ignore any switch bounce

    if (digitalRead(START_GATE) != START_TRIP)    // gate still closed
    {
      initialize();    // reset timer
    } 
  } 

  if (serial_data == int(SMSG_RSEND))    // resend race data
  {
      smsg(SMSG_ACKNW);
      send_race_results();
  } 

  set_display_brightness();
  display_race_results();

  return;
}


/*================================================================================*
  PROCESS GENERAL SERIAL MESSAGES
 *================================================================================*/
void process_general_msgs()
{
  int lane;
  char tmps[50];


  serial_data = get_serial_data();

  if (serial_data == int(SMSG_GVERS))    // get software version
  {
      sprintf(tmps, "vert=%s", PDT_VERSION);
      smsg_str(tmps);
  } 

  else if (serial_data == int(SMSG_GNUML))    // get number of lanes
  {
      sprintf(tmps, "numl=%d", NUM_LANES);
      smsg_str(tmps);
  } 

  else if (serial_data == int(SMSG_TINFO))    // get timer information
  {
      send_timer_info();
  } 

  else if (serial_data == int(SMSG_DEBUG))    // toggle debug
  {
    fDebug = !fDebug;
    dbg(true, "toggle debug = ", fDebug);
  } 

  else if (serial_data == int(SMSG_CGATE))    // check start gate
  {
    if (digitalRead(START_GATE) == START_TRIP)    // gate open
    {
      smsg(SMSG_GOPEN);
    } 
    else
    {
      smsg(SMSG_ACKNW);
    } 
  } 

  else if (serial_data == int(SMSG_RESET) || digitalRead(RESET_SWITCH) == LOW)    // timer reset
  {
    if (digitalRead(START_GATE) != START_TRIP)    // only reset if gate closed
    {
      initialize();
    } 
    else
    {
      smsg(SMSG_GOPEN);
    } 
  } 

  else if (serial_data == int(SMSG_LMASK))    // lane mask
  {
    delay(100);
    serial_data = get_serial_data();

    lane = serial_data - 48;
    if (lane >= 1 && lane <= NUM_LANES)
    {
      lane_mask[lane-1] = true;

      dbg(fDebug, "set mask on lane = ", lane);
    }
    smsg(SMSG_ACKNW);
  }

  else if (serial_data == int(SMSG_UMASK))    // unmask all lanes
  {
    unmask_all_lanes();
    smsg(SMSG_ACKNW);
  }

  return;
}


/*================================================================================*
  TEST PDT FINISH DETECTION HARDWARE
 *================================================================================*/
void test_pdt_hw()
{
  char ctmp[10];


  smsg_str("TEST MODE");
  set_status_led();
  delay(2000); 

/*-----------------------------------------*
   show status of lane detectors
 *-----------------------------------------*/
  while(true)
  {
    for (int n=0; n<NUM_LANES; n++)
    {
      // Test sensors by using distance readings
      bool lane_status = checkFinishLineCrossed(n);
      
      if (lane_status)
      {
        update_display(n, msgLight);
      }
      else
      {
        update_display(n, msgDark);
      }
    }

    if (digitalRead(RESET_SWITCH) == LOW)  // break out of this test
    {
      clear_displays();
      delay(1000); 
      break;
    }
    delay(100);
  }

/*-----------------------------------------*
   show status of start gate switch
 *-----------------------------------------*/
  while(true)
  {
    if (digitalRead(START_GATE) == START_TRIP) 
    {
      update_display(0, msgGateO);
    }
    else
    {
      update_display(0, msgGateC);
    }

    if (digitalRead(RESET_SWITCH) == LOW)  // break out of this test
    {
      clear_displays();
      delay(1000); 
      break;
    }
    delay(100);
  }

/*-----------------------------------------*
   show pattern for brightness adjustment
 *-----------------------------------------*/
  while(true)
  {
#ifdef LED_DISPLAY
    set_display_brightness();

    for (int n=0; n<NUM_LANES; n++)
    {
      sprintf(ctmp,"%d%03d", (n+1), (int)display_level);

      disp_mat[n].clear();

      disp_mat[n].writeDigitNum(0, char2int(ctmp[0]), false);
      disp_mat[n].writeDigitNum(1, char2int(ctmp[1]), false);
      disp_mat[n].writeDigitNum(3, char2int(ctmp[2]), false);
      disp_mat[n].writeDigitNum(4, char2int(ctmp[3]), false);

      disp_mat[n].drawColon(false);
      disp_mat[n].writeDisplay();

#ifdef DUAL_DISP
#ifdef DUAL_MODE
      disp_8x8[n+4].clear();
      disp_8x8[n+4].setTextSize(1);
      disp_8x8[n+4].setRotation(3);
      disp_8x8[n+4].setCursor(2, 0);
      disp_8x8[n+4].print("X");
      disp_8x8[n+4].writeDisplay();
#else
      disp_mat[n+4].clear();

      disp_mat[n+4].writeDigitNum(0, char2int(ctmp[0]), false);
      disp_mat[n+4].writeDigitNum(1, char2int(ctmp[1]), false);
      disp_mat[n+4].writeDigitNum(3, char2int(ctmp[2]), false);
      disp_mat[n+4].writeDigitNum(4, char2int(ctmp[3]), false);

      disp_mat[n+4].drawColon(false);
      disp_mat[n+4].writeDisplay();
#endif
#endif
    }
#endif

    if (digitalRead(RESET_SWITCH) == LOW)  // break out of this test
    {
      clear_displays();
      break;
    }
    delay(1000);
  }
}


/*================================================================================*
  SEND RACE RESULTS TO COMPUTER
 *================================================================================*/
void send_race_results()
{
  float lane_time_sec;


  for (int n=0; n<NUM_LANES; n++)    // send times to computer
  {
    lane_time_sec = (float)(lane_time[n] / 1000000.0);    // elapsed time (seconds)

    if (lane_time_sec == 0)    // did not finish
    {
      lane_time_sec = NULL_TIME;
    }

    Serial.print(n+1);
    Serial.print(" - ");
    Serial.println(lane_time_sec, NUM_DIGIT);  // numbers are rounded to NUM_DIGIT
                                               // digits by println function
  }

  return;
}


/*================================================================================*
  RACE FINISHED - DISPLAY PLACE / TIME FOR ALL LANES
 *================================================================================*/
void display_race_results()
{
  unsigned long now;
  static boolean display_mode;
  static unsigned long last_display_update = 0;


  if (!SHOW_PLACE) return;

  now = millis();

  if (last_display_update == 0)  // first cycle
  {
    last_display_update = now;
    display_mode = false;
  }

  if ((now - last_display_update) > (unsigned long)(PLACE_DELAY * 1000))
  {
    dbg(fDebug, "display_race_results");

    for (int n=0; n<NUM_LANES; n++)
    {
      update_display(n, lane_place[n], lane_time[n], display_mode);
    }

    display_mode = !display_mode;
    last_display_update = now;
  }

  return;
}


/*================================================================================*
  SEND MESSAGE TO DISPLAY
 *================================================================================*/
void update_display(int lane, unsigned char msg[])
{

#ifdef LED_DISPLAY
  disp_mat[lane].clear();
#ifdef DUAL_DISP
#ifdef DUAL_MODE
  disp_8x8[lane+4].clear();
#else
  disp_mat[lane+4].clear();
#endif
#endif

  for (int d = 0; d<=4; d++)
  {
    disp_mat[lane].writeDigitRaw(d, msg[d]);
#ifdef DUAL_DISP
#ifdef DUAL_MODE
    if (d == 3)
    {
      disp_8x8[lane+4].setTextSize(1);
      disp_8x8[lane+4].setRotation(3);
      disp_8x8[lane+4].setCursor(2, 0);
      if (msg == msgBlank)
         disp_8x8[lane+4].print(" ");
      else
         disp_8x8[lane+4].print("-");
    }
#else
    disp_mat[lane+4].writeDigitRaw(d, msg[d]);
#endif
#endif
  }

  disp_mat[lane].writeDisplay();
#ifdef DUAL_DISP
#ifdef DUAL_MODE
  disp_8x8[lane+4].writeDisplay();
#else
  disp_mat[lane+4].writeDisplay();
#endif
#endif
#endif

  return;
}


/*================================================================================*
  UPDATE LANE PLACE/TIME DISPLAY
 *================================================================================*/
void update_display(int lane, int display_place, unsigned long display_time, int display_mode)
{
  int c;
  char ctime[10], cplace[4];
  double display_time_sec;
  boolean showdot;

//  dbg(fDebug, "led: lane = ", lane);
//  dbg(fDebug, "led: plce = ", display_place);
//  dbg(fDebug, "led: time = ", display_time);

#ifdef LED_DISPLAY
  if (display_mode)
  {
    if (display_place > 0)  // show place order
    {
      sprintf(cplace,"%1d", display_place);

      disp_mat[lane].clear();
      disp_mat[lane].drawColon(false);
      disp_mat[lane].writeDigitNum(3, char2int(cplace[0]), false);
      disp_mat[lane].writeDisplay();

#ifdef DUAL_DISP
      disp_mat[lane+4].clear();
      disp_mat[lane+4].drawColon(false);
      disp_mat[lane+4].writeDigitNum(3, char2int(cplace[0]), false);
      disp_mat[lane+4].writeDisplay();
#endif
    }
    else  // did not finish
    {
      update_display(lane, msgDashL);
    }
  }
  else                      // show finish time
  {
    if (display_time > 0)
    {
      disp_mat[lane].clear();
      disp_mat[lane].drawColon(false);

#ifdef DUAL_DISP
#ifdef DUAL_MODE
      disp_8x8[lane+4].clear();
      disp_8x8[lane+4].setTextSize(1);
      disp_8x8[lane+4].setRotation(3);
      disp_8x8[lane+4].setCursor(2, 0);
#else
      disp_mat[lane+4].clear();
      disp_mat[lane+4].drawColon(false);
#endif
#endif

      display_time_sec = (double)(display_time / (double)1000000.0);    // elapsed time (seconds)
      dtostrf(display_time_sec, (DISP_DIGIT+1), DISP_DIGIT, ctime);     // convert to string

//      Serial.print("ctime = ["); Serial.print(ctime); Serial.println("]");
      c = 0;
      for (int d = 0; d<DISP_DIGIT; d++)
      {
#ifdef LARGE_DISP
        showdot = false;
#else
        showdot = (ctime[c + 1] == '.');
#endif
        disp_mat[lane].writeDigitNum(d + int(d / 2), char2int(ctime[c]), showdot);    // time
#ifdef DUAL_DISP
#ifdef DUAL_MODE
        sprintf(cplace,"%1d", display_place);
        disp_8x8[lane+4].print(cplace[0]);
#else
        disp_mat[lane+4].writeDigitNum(d + int(d / 2), char2int(ctime[c]), showdot);    // time
#endif
#endif

        c++; if (ctime[c] == '.') c++;
      }

#ifdef LARGE_DISP
      disp_mat[lane].writeDigitRaw(2, 16);
#ifdef DUAL_DISP
#ifndef DUAL_MODE
      disp_mat[lane+4].writeDigitRaw(2, 16);
#endif
#endif
#endif

      disp_mat[lane].writeDisplay();
#ifdef DUAL_DISP
#ifdef DUAL_MODE
      disp_8x8[lane+4].writeDisplay();
#else
      disp_mat[lane+4].writeDisplay();
#endif
#endif
    }
    else  // did not finish
    {
      update_display(lane, msgDashT);
    }
  }
#endif

  return;
}


/*================================================================================*
  CLEAR LANE PLACE/TIME DISPLAYS
 *================================================================================*/
void clear_displays()
{
  dbg(fDebug, "led: CLEAR");

  for (int n=0; n<NUM_LANES; n++)
  {
    if (mode == mRACING || mode == mTEST)
    {
      update_display(n, msgBlank);     // racing
    }
    else
    {
      update_display(n, msgDashT);     // ready
    }
  }

  return;
}


/*================================================================================*
  SET LANE DISPLAY BRIGHTNESS
 *================================================================================*/
void set_display_brightness()
{
  float new_level;

#ifdef LED_DISPLAY
  new_level = long(1023 - analogRead(BRIGHT_LEV)) / 1023.0F * 15.0F;
  new_level = min(new_level, (float)MAX_BRIGHT);
  new_level = max(new_level, (float)MIN_BRIGHT);

  if (fabs(new_level - display_level) > 0.3F)    // deadband to prevent flickering 
  {                                              // between levels
    dbg(fDebug, "led: BRIGHT");

    display_level = new_level;

    for (int n=0; n<NUM_LANES; n++)
    {
      disp_mat[n].setBrightness((int)display_level);
#ifdef DUAL_DISP
#ifdef DUAL_MODE
      disp_8x8[n+4].setBrightness((int)display_level);
#else
      disp_mat[n+4].setBrightness((int)display_level);
#endif
#endif
    }
  }
#endif

  return;
}


/*================================================================================*
  SET TIMER STATUS LED
 *================================================================================*/
void set_status_led()
{
  int r_lev, b_lev, g_lev;

  dbg(fDebug, "status led = ", mode);

  r_lev = PWM_LED_OFF;
  b_lev = PWM_LED_OFF;
  g_lev = PWM_LED_OFF;

  if (mode == mREADY)         // blue
  {
    b_lev = PWM_LED_ON;
  }
  else if (mode == mRACING)  // green
  {
    g_lev = PWM_LED_ON;
  }
  else if (mode == mFINISH)  // red
  {
    r_lev = PWM_LED_ON;
  }
  else if (mode == mTEST)    // yellow
  {
    r_lev = PWM_LED_ON;
    g_lev = PWM_LED_ON;
  }

  analogWrite(STATUS_LED_R,  r_lev);
  analogWrite(STATUS_LED_B,  b_lev);
  analogWrite(STATUS_LED_G,  g_lev);

  return;
}


/*================================================================================*
  READ SERIAL DATA FROM COMPUTER
 *================================================================================*/
int get_serial_data()
{  
  int data = 0;
  
  if (Serial.available() > 0)
  {
    data = Serial.read();
    dbg(fDebug, "ser rec = ", data);
  }

  return data;
}  


/*================================================================================*
  INITIALIZE TIMER
 *================================================================================*/
void initialize(bool powerup)
{  
  for (int n=0; n<NUM_LANES; n++)
  {
    lane_time[n] = 0;
    lane_place[n] = 0;
  }

  start_time = 0;
  set_status_led();
  digitalWrite(START_SOL, HIGH);  // Deactivate solenoid (HIGH)

  // if power up and gate is open -> goto FINISH state
  if (powerup && digitalRead(START_GATE) == START_TRIP) 
  {
    mode = mFINISH;
  }
  else
  {
    mode = mREADY;

    smsg(SMSG_READY);
    delay(100);
  }
  Serial.flush();

  ready_first  = true;
  finish_first  = true;

  return;
}


/*================================================================================*
  UNMASK ALL LANES
 *================================================================================*/
void unmask_all_lanes()
{  
  dbg(fDebug, "unmask all lanes");

  for (int n=0; n<NUM_LANES; n++)
  {
    lane_mask[n] = false;
  }  

  return;
}  


/*================================================================================*
  SEND DEBUG TO COMPUTER
 *================================================================================*/
void dbg(int flag, const char * msg, int val)
{  
  char tmps[50];


  if (!flag) return;

  smsg_str("dbg: ", false);
  smsg_str(msg, false);

  if (val != -999)
  {
    sprintf(tmps, "%d", val);
    smsg_str(tmps);
  }
  else
  {
    smsg_str("");
  }

  return;
}


/*================================================================================*
  SEND SERIAL MESSAGE (CHAR) TO COMPUTER
 *================================================================================*/
void smsg(char msg, bool crlf)
{  
  if (crlf)
  {
    Serial.println(msg);
  }
  else
  {
    Serial.print(msg);
  }

  return;
}


/*================================================================================*
  SEND SERIAL MESSAGE (STRING) TO COMPUTER
 *================================================================================*/
void smsg_str(const char * msg, bool crlf)
{  
  if (crlf)
  {
    Serial.println(msg);
  }
  else
  {
    Serial.print(msg);
  }

  return;
}


/*================================================================================*
  SEND TIMER INFORMATION TO COMPUTER
 *================================================================================*/
void send_timer_info()
{
  char tmps[50];

  Serial.println("-----------------------------");
  sprintf(tmps, " PDT            Version %s", PDT_VERSION);
  Serial.println(tmps);
  Serial.println("-----------------------------");

  sprintf(tmps, "  NUM_LANES     %d", NUM_LANES);
  Serial.println(tmps);
  sprintf(tmps, "  GATE_RESET    %d", GATE_RESET);
  Serial.println(tmps);
  sprintf(tmps, "  SHOW_PLACE    %d", SHOW_PLACE);
  Serial.println(tmps);
  sprintf(tmps, "  PLACE_DELAY   %d", PLACE_DELAY);
  Serial.println(tmps);
  sprintf(tmps, "  MIN_BRIGHT    %d", MIN_BRIGHT);
  Serial.println(tmps);
  sprintf(tmps, "  MAX_BRIGHT    %d", MAX_BRIGHT);
  Serial.println(tmps);

  Serial.println("");

#ifdef LED_DISPLAY
  Serial.println("  LED_DISPLAY   1");
#else
  Serial.println("  LED_DISPLAY   0");
#endif

#ifdef DUAL_DISP
  Serial.println("  DUAL_DISP     1");
#else
  Serial.println("  DUAL_DISP     0");
#endif
#ifdef DUAL_MODE
  Serial.println("  DUAL_MODE     1");
#else
  Serial.println("  DUAL_MODE     0");
#endif

#ifdef LARGE_DISP
  Serial.println("  LARGE_DISP    1");
#else
  Serial.println("  LARGE_DISP    0");
#endif

  Serial.println("");

  sprintf(tmps, "  ARDUINO VERS  %04d", ARDUINO);
  Serial.println(tmps);
  sprintf(tmps, "  COMPILE DATE  %s", __DATE__);
  Serial.println(tmps);
  sprintf(tmps, "  COMPILE TIME  %s", __TIME__);
  Serial.println(tmps);

  Serial.println("-----------------------------");

  return;
}
