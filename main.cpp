#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

#define lcdSDA 6 // LCD SDA
#define lcdSCK 7 // LCD SCK
#define btn1 17  // Button 1
#define btn2 18  // Button 2
#define btn3 16  // Button 3
#define btn4 8   // Home Button

LiquidCrystal_I2C lcd(0x27,  16, 2);


// Function Declarations
void IRAM_ATTR interruptCode();
void oneSecondTimer();
void secondsToHMS(int TimeSeconds);
void home();
void timer_home();
void timer_pause();
void timer_running();
void timer_end();
void stopwatch_home();
void stopwatch_running();
void stopwatch_pause();
void stopwatch_end();
//

// Variables for Interrupts
hw_timer_t *timerCounter = NULL;
volatile bool interruptOccured = false; 
volatile int numInterrupts = 0; // For potential improvement.

// Variables for Timer
// All these values are in seconds
int timerMode = 0; // 0 (not in Timer), 1 (in Timer)
int timeTillEndofTimer = 0; // 
int timerTimeOptions[3] = {1800, 2700, 3599}; // 30 mins (1800s), 45mins (2700s), 1hour (3600s ~ 3599s)
                        
                      

// Variables for Stopwatch
int stopWatchTime = 0; // Initially 0. Counts up. Capped at 9h 59m 59s due to LCD Display and design choice.

// Variables for LCD
char lcdBuffer[17]; // Holds string for time format Xh XXm XXs

// States
enum states {
  HOME,
  TIMER_HOME,
  TIMER_RUNNING,
  TIMER_PAUSE,
  TIMER_END,
  STOPWATCH_HOME,
  STOPWATCH_RUNNING,
  STOPWATCH_PAUSE,
  STOPWATCH_END
};

states curr_state; // Current state for state machine


void setup() {
  Serial.begin(9600); // For serial monitor (debugging).
  oneSecondTimer(); // Runs all the code in 1 second function, ready to generate 
                    // interrupts when required.

  // For LCD
  lcd.init(lcdSDA, lcdSCK);
  lcd.backlight();
  lcd.setCursor(0,0);

  // Buttons ~ Input
  pinMode(btn1, INPUT_PULLUP);
  pinMode(btn2, INPUT_PULLUP);
  pinMode(btn3, INPUT_PULLUP);
  pinMode(btn4, INPUT_PULLUP);

  delay(500);
  curr_state = HOME; // Start at home screen
}

void loop() {
  switch (curr_state) {
    case HOME:
      home();
      break;

    case TIMER_HOME:
      timerMode = 1;
      timer_home();
      break;

    case TIMER_RUNNING:
      timer_running();
      break;

    case TIMER_PAUSE:
      timer_pause();
      break;

    case TIMER_END:
      timerMode = 0;
      timer_end();
      break;      

    case STOPWATCH_HOME:
      timerMode = 0;
      stopwatch_home();
      break;
    
    case STOPWATCH_RUNNING:
      stopwatch_running();
      break;
    
    case STOPWATCH_PAUSE:
      stopwatch_pause();
      break;

    case STOPWATCH_END:
      stopwatch_end();
      break;
    
  }

}


// Below are functions used in rest of the program.

void oneSecondTimer() {
  /* This function is used to keep track of 1 second using ESP32 interrupts */

  // Use 1MHz ~ every 1,000,000 ticks = 1 second
  timerCounter = timerBegin(0, 80, true);     // ESP has 4 hardware timers, chose #1 index 0. 
                                              // 1,000,000 ticks for 1 second. 
                                              // 0 - hardware timer used in ESP32
                                              // 80 - prescaler value
                                              // true - count upwards from 0 till you reach 1,000,000

  timerAttachInterrupt(timerCounter, &interruptCode, true); 
  /*
  timerCounter is timer to use. &interruptCode is address of code to run when
  interrupt occurs. 
  last argument is used for edge triggered (true) or level triggered (false)
  */
  timerAlarmWrite(timerCounter, 1000000, true); // Trigger the alarm at 1,000,000 ticks
                                                // 3rd argument is for reload (true) to make
                                                // the timer periodic. 
  timerAlarmEnable(timerCounter); // Enables the timer
}

void IRAM_ATTR interruptCode() {
  // This does not return anything. 
  interruptOccured = true;
}

void secondsToHMS(int timeSeconds) {
  /* This function converts seconds to MINUTES, and SECONDS 
  Example:
  90 seconds = 1min 30s
  2542 seconds = 42min 22s
  */
  int hrs = timeSeconds/3600; // This returns value for integer division
  int mins = (timeSeconds - 3600*hrs)/60; // Same for this
  int secs = (timeSeconds- 3600*hrs - 60*mins);

  // Max length string on LCD displayable is 16  
  if (timerMode == 1) { // For timer
    snprintf(lcdBuffer, sizeof(lcdBuffer), "%02dm%02ds", mins, secs); 
  }
  else {
    snprintf(lcdBuffer, sizeof(lcdBuffer), "%01dh%02dm%02ds", hrs, mins, secs); // For stopwatch.
  }
}

/////////////////////////////////////////////////////////////////
/* FSM Functions 

home, 
timer_home, timer_running, timer_pause, timer_end,
stopwatch_home, stopwatch_running, stopwatch_pause, stopwatch_end

*/

void home() {
  // HO 
  // ME
  lcd.setCursor(0,0);
  lcd.print("HO");
  lcd.setCursor(0,1);
  lcd.print("ME");

  // Timer
  lcd.setCursor(3, 0);
  lcd.print("[1]TIMER");

  // Stopwatch
  lcd.setCursor(3, 1);
  lcd.print("[2]STOPWATCH");


  // Conditionals for State Switching
  if (digitalRead(btn1) == LOW) {
        curr_state = TIMER_HOME;
        lcd.clear();
        delay(300);
      }
  if (digitalRead(btn2) == LOW) {
        curr_state = STOPWATCH_HOME;
        lcd.clear();
        delay(300);
      }
}

void timer_home() {
  // TIMER
  lcd.setCursor(4,0);
  lcd.print("TIMER");

  // Options ~ 1) 30m 2) 45m 3) 1h
  lcd.setCursor(0,1);
  lcd.print("1)30m 2)45m 3)1h");

  // Conditionals for State Switching
  if (digitalRead(btn4) == LOW) {
    curr_state = HOME;
    lcd.clear();
    delay(300);
  }

  if (digitalRead(btn1) == LOW) { // 30 mins
    timeTillEndofTimer = timerTimeOptions[0]; // 30m 00s
    curr_state = TIMER_RUNNING;
    lcd.clear();
    delay(300); 
  }
  else if (digitalRead(btn2) == LOW) { // 45 mins
    timeTillEndofTimer = timerTimeOptions[1]; // 45m 00s
    curr_state = TIMER_RUNNING;
    lcd.clear();
    delay(300);
  }
  else if (digitalRead(btn3) == LOW) { // 1 hour
    timeTillEndofTimer = timerTimeOptions[2]; // 59m 59s
    curr_state = TIMER_RUNNING;
    lcd.clear();
    delay(300);
  }
}

void timer_running() {
    timerStart(timerCounter); // Explicit start due to potential pausing in other states.

    // TIMER HEADER
    lcd.setCursor(0,0);
    lcd.print("TIMER");

    // Timer Time
    lcd.setCursor(0,1);
    secondsToHMS(timeTillEndofTimer); // Used to fill lcdBuffer array ~ lcdBuffer is the string
    lcd.print(lcdBuffer);

    // Options ~ PAU(PAUSE), END
    lcd.setCursor(11,0);
    lcd.print("1)PAU");
    lcd.setCursor(11,1);
    lcd.print("2)END");

    // Logic for counting time
    if (interruptOccured == true) {
    interruptOccured = !interruptOccured;
    timeTillEndofTimer -= 1;
    }

    // End of Timer
    if (timeTillEndofTimer == 0) {
      curr_state = TIMER_END;
      lcd.clear();
      timerEnd(timerCounter); // End the generation of interrupt signals made by ESP32
    }

    // Conditionals for State Switching
    if (digitalRead(btn1) == LOW) {
        timerStop(timerCounter);
        curr_state = TIMER_PAUSE;
        lcd.clear();
        delay(300);
      }
    if (digitalRead(btn2) == LOW) {
      timerStop(timerCounter);
      curr_state = TIMER_END;
      lcd.clear();
      delay(300);
    }
}

void timer_pause() {
  // PAUSED 
  lcd.setCursor(0,0);
  lcd.print("PAUSED");

  // TIME REMAINING
  lcd.setCursor(7,0);
  lcd.print(lcdBuffer);

  // MENU OPTIONS
  lcd.setCursor(0,1);
  lcd.print("1)RSM 2)RST 4)HM"); // Resume, Reset, Home

  // Conditionals for State Switching
  if (digitalRead(btn1) == LOW) {
    timerStart(timerCounter);
    //delay(100);
    curr_state = TIMER_RUNNING;
    lcd.clear();
    delay(300);
  }
  if (digitalRead(btn2) == LOW) {
    curr_state = TIMER_HOME;
    timerStop(timerCounter);
    lcd.clear();
    delay(300);
  }
  if (digitalRead(btn4) == LOW) {
    curr_state = HOME;
    timerStop(timerCounter);
    lcd.clear();
    delay(300);
  }
}

void timer_end() {
  // END OF TIMER
  lcd.setCursor(2,0);
  lcd.print("END OF TIMER");
  // [PRESS HOME]
  lcd.setCursor(2,1);
  lcd.print("[PRESS HOME]");


  // Conditionals for State Switching
  if (digitalRead(btn4) == LOW) {
    lcd.clear();
    curr_state = HOME;
    delay(300);
  }
}

void stopwatch_home() {
  stopWatchTime = 0;

  // STOPWATCH
  lcd.setCursor(3,0);
  lcd.print("STOPWATCH");

  // [1] to START
  lcd.setCursor(2, 1);
  lcd.print("[1] to START");
  
  // Conditionals for State Switching
  if (digitalRead(btn1) == LOW) {
    timerStart(timerCounter);
    curr_state = STOPWATCH_RUNNING;
    lcd.clear();
    delay(300);
  }
  if (digitalRead(btn4) == LOW) {
    curr_state = HOME;
    lcd.clear();
    delay(300);
  }
}

void stopwatch_running() {
  // STOPWATCH
  lcd.setCursor(0,0);
  lcd.print("STOPWATCH");

  // TIME xh xxm xxs
  secondsToHMS(stopWatchTime);
  lcd.setCursor(0,1);
  lcd.print(lcdBuffer);

  // Options ~ Pause and End
  lcd.setCursor(11,0);
  lcd.print("1)PAU");
  lcd.setCursor(11,1);
  lcd.print("2)END");

  // Timing Logic
  if (interruptOccured == true) {
    interruptOccured = !interruptOccured;
    stopWatchTime += 1;
  }

  // Conditionals for State Switching
  if (digitalRead(btn1) == LOW) {
    timerStop(timerCounter);
    curr_state = STOPWATCH_PAUSE;
    lcd.clear();
    delay(300);
  }
  if (digitalRead(btn2) == LOW) {
    timerStop(timerCounter);
    curr_state = STOPWATCH_END;
    lcd.clear();
    delay(300);
  }
}

void stopwatch_pause() {
  // PAUSED
  lcd.setCursor(0,0);
  lcd.print("PAUSED");

  // PAUSED TIME
  lcd.setCursor(8, 0);
  lcd.print(lcdBuffer);

  // Options ~ Resume, Reset, (Home)
  lcd.setCursor(0,1); 
  lcd.print("1)RSM 2)RST 4)HM");


  // Conditionals for State Switching
  if (digitalRead(btn1) == LOW) {
    timerStart(timerCounter);
    curr_state = STOPWATCH_RUNNING;
    lcd.clear();
    delay(300);
  }
  if (digitalRead(btn2) == LOW) {
    curr_state = STOPWATCH_HOME;
    lcd.clear();
    delay(300);
  }
  if (digitalRead(btn4) == LOW) {
    timerStop(timerCounter);
    curr_state = HOME;
    lcd.clear();
    delay(300);
  }
}

void stopwatch_end() {
  // STOPWATCH ENDED
  lcd.setCursor(0,0);
  lcd.print("STOPWATCH  ENDED");

  // STOPWATCH TIME
  lcd.setCursor(0,1);
  lcd.print(lcdBuffer);

  // [HOME]
  lcd.setCursor(10, 1);
  lcd.print("[HOME]");

  // Conditionals for State Switching
  if (digitalRead(btn4) == LOW) {
    curr_state = HOME;
    lcd.clear();
    delay(300);
  }
}


