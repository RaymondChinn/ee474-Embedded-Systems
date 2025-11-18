// Filename: Lab3_Part2
// Author: Raymond Chinn, Xander Geurkink
// Date: 11/5/2025
// Description: Priority-based task scheduler using TCB (Task Control Block)
//              structure. Executes 4 tasks based on dynamic priority levels,
//              cycling priorities after each complete round.
//
// Version: 1.0
// ============================================================================
// INCLUDES
// ============================================================================
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ============================================================================
// HARDWARE PIN DEFINITIONS
// ============================================================================
#define LED1 5  // ledBlinkerTask led pin
#define LED2 6  // musicPlayerTask led pin
#define BUZZER_PIN 19 // Buzzer for musicPlayerTask

// ============================================================================
// LCD CONFIGURATION
// ============================================================================
#define LCD_ADDR 0x27                    // I2C address for LCD
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);  // 16 columns x 2 rows

// ============================================================================
// PWM CONFIGURATION
// ============================================================================
#define PWM_FREQ 500           // Base PWM frequency in Hz
#define PWM_RESOLUTION 13      // 13-bit resolution (0-8191 range)


// TCB structure
// Structure that defines each task's properties including function pointer, 
// priority level, completion status, and name.
struct TCB {
  void (*task_function)();  // Pointer to the task's function
  int priority;             // Priority level (1-4)
  bool isComplete;          // Task completion status
  const char *taskName;     // Name of task
};

// Forward declarations
void ledBlinkerTask();
void counterTask();
void musicPlayerTask();
void alphabetPrinterTask();

// ============================================================================
// TASK CONTROL BLOCK ARRAY
// ============================================================================
// Initialize array of TCBs with initial priorities 1-4
TCB tasks[] = {
  { ledBlinkerTask, 1, false, "LED Blinker" },
  { counterTask, 2, false, "Counter" },
  { musicPlayerTask, 3, false, "Music Player" },
  { alphabetPrinterTask, 4, false, "Alphabet Printer" }
};

#define NUM_TASKS 4

// ============================================================================
// TASK IMPLEMENTATIONS
// ============================================================================

// Name: ledBlinkerTask
// Description: Blinks LED1 eight times with 1-second intervals (500ms ON, 
//              500ms OFF). Priority 1 initially.
void ledBlinkerTask() {
  for (int i = 0; i < 8; i++) {
    digitalWrite(LED1, HIGH); // LED on
    delay(500);
    digitalWrite(LED1, LOW); // LED off
    delay(500);
  }
}

// Name: counterTask
// Description: Displays counting from 1 to 10 on LCD with 1-second intervals.
//              Priority 2 initially.
void counterTask() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Count:");

  for (int i = 1; i <= 10; i++) {
    lcd.setCursor(7, 0);  // Position after "Count: "
    lcd.print("  ");      // Clear previous number
    lcd.setCursor(7, 0);
    lcd.print(i);
    delay(1000);
  }

  lcd.clear();
}

// Name: musicPlayerTask
// Description: Plays 10 different notes on buzzer while visualizing
//              note frequency as LED2 brightness. Higher notes = brighter LED.
//              Priority 3 initially.
void musicPlayerTask() {
  int notes[] = { 262, 294, 330, 349, 392, 440, 494, 523, 587, 659 };
  for (int i = 0; i < 10; i++) {

    // Play note and adjust brightness
    int frequency = notes[i];
    ledcChangeFrequency(BUZZER_PIN, frequency, PWM_RESOLUTION);
    ledcWrite(BUZZER_PIN, 1023); // Set duty cycle for audible sound range

    // Display note on LED (map frequency to PWM duty cycle);
    int brightness = map(frequency, 200, 700, 0, 225);
    ledcWrite(LED2, brightness);

    delay(500);

    // Stop note
    ledcChangeFrequency(BUZZER_PIN, 0, PWM_RESOLUTION);
    ledcWrite(BUZZER_PIN, 0);
    ledcWrite(LED2, 0);
    delay(200);
  }
}

// Name: alphabetPrinterTask
// Description: Prints the alphabet (A-Z) to Serial Monitor with 500ms delay
//              between each letter. Priority 4 initially.
void alphabetPrinterTask() {
  Serial.print("Alphabet: ");

  for (char c = 'A'; c <= 'Z'; c++) {
    Serial.print(c);
    if (c < 'Z') {
      Serial.print(", ");
    }
    delay(500);
  }
  Serial.println();
}

// Name: setup
// Description: Initializes serial communication, LCD display, GPIO pins,
//              and PWM channels for buzzer and LED control.
void setup() {
  Serial.begin(115200);
  
  // Initialize I2C LCD
  Wire.begin();
  lcd.init();
  lcd.backlight();
  delay(2);

  // Configure GPIO pins
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Configure PWM signal on BUZZER_PIN and LED2 Pin
  ledcAttach(BUZZER_PIN, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(LED2, PWM_FREQ, PWM_RESOLUTION);
}

// Name: loop
// Description: Main scheduler loop. Selects highest priority incomplete task,
//              executes it, marks as complete, and cycles priorities when all
//              tasks finish. Implements a simple round-robin priority scheduler.
void loop() {
  // Get index of highest priority incomplete task
  int taskIndex = getHighestPriority();

  // Execute if task is ready to be completed
  if (taskIndex != -1) {
    // Execute highest priority task
    tasks[taskIndex].task_function();

    // Mark task as complete
    tasks[taskIndex].isComplete = true;

    // Print task name and priority
    Serial.print(tasks[taskIndex].taskName);
    Serial.print(": ");
    Serial.print(tasks[taskIndex].priority);
    Serial.println();
  }

  if (allTasksComplete()) {
    resetAndIncrementPriority();
  }

  delay(100);
}

// ============================================================================
// SCHEDULER HELPER FUNCTIONS
// ============================================================================

// Name: getHighestPriority
// Description: Scans the TCB array and returns the index of the incomplete
//              task with the highest priority (lowest priority number).
//              Returns -1 if no incomplete tasks exist.
int getHighestPriority() {
  // Start with priority greater than NUM_TASKS (4)
  int highestPriority = 5;
  int taskIndex = -1;

  // Scan all tasks to find highest priority
  for (int i = 0; i < NUM_TASKS; i++) {
    TCB currTask = tasks[i];
    if (currTask.priority < highestPriority && !currTask.isComplete) {
      highestPriority = currTask.priority;
      taskIndex = i;
    }
  }
  return taskIndex;
}

// Name: allTasksComplete
// Description: Checks if all tasks in the TCB array have been completed.
//              Returns true if all complete, false otherwise.
bool allTasksComplete() {
  for (int i = 0; i < NUM_TASKS; i++) {
    if (!tasks[i].isComplete) {
      return false;
    }
  }
  return true;
}

// Name: resetAndIncrementPriority
// Description: Resets all task completion flags and cycles each task's
//              priority in round-robin fashion (1→2→3→4→1). 
void resetAndIncrementPriority() {
  for (int i = 0; i < NUM_TASKS; i++) {
    tasks[i].isComplete = false;
    tasks[i].priority = tasks[i].priority % 4 + 1;
  }
}
