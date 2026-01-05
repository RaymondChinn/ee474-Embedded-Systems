/**
 * @file Lab4_Part1.ino
 * @author Raymond Chinn & Xander Geurkink
 * @date 12/5/2025
 * @version 1.0
 * @brief This file implements a preemptive Shortest Remaining Time First (SRTF) scheduler
 * with three worker tasks:
 * 1) LED blinker, 2) LCD counter, 3) serial monitor alphabet display.
 * * The scheduler prioritizes the task with the least remaining execution time.
 */

// =============================== INCLUDES ===============================
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <string.h>
#include <Wire.h> // Included for Wire.begin() used in setup

// =============================== MACROS ===============================
/// GPIO pin for LED task
#define LED_PIN 4

// ========================= GLOBAL VARIABLES ===========================

// ========================= task timing variables =======================

/// Total execution time required for the LED task (500 ms)
const TickType_t ledTaskExecutionTime = 500 / portTICK_PERIOD_MS;
/// Total execution time required for the Counter task (2 seconds)
const TickType_t counterTaskExecutionTime = 2000 / portTICK_PERIOD_MS;
/// Total execution time required for the Alphabet task (13 seconds)
const TickType_t alphabetTaskExecutionTime = 13000 / portTICK_PERIOD_MS;

/// Remaining Execution Time for the LED task
volatile TickType_t remainingLedTime = ledTaskExecutionTime;
/// Remaining Execution Time for the Counter task
volatile TickType_t remainingCounterTime = counterTaskExecutionTime;
/// Remaining Execution Time for the Alphabet task
volatile TickType_t remainingAlphabetTime = alphabetTaskExecutionTime;

/// Small time slice (10 ms) used by all tasks to allow interruption by the scheduler
const TickType_t workSliceDelay = pdMS_TO_TICKS(10);

/// Wake time for the LED worker task, used by the scheduler
volatile TickType_t ledWakeTime = pdMS_TO_TICKS(0);
/// Wake time for the Counter worker task, used by the scheduler
volatile TickType_t counterWakeTime = pdMS_TO_TICKS(0);
/// Wake time for the Alphabet worker task, used by the scheduler
volatile TickType_t alphabetWakeTime = pdMS_TO_TICKS(0);

// ========================= task handles ===========================
/// Handle for the LED blinking task
TaskHandle_t ledTaskHandle = NULL;
/// Handle for the LCD counter task
TaskHandle_t counterTaskHandle = NULL;
/// Handle for the Serial alphabet display task
TaskHandle_t alphabetTaskHandle = NULL;
/// Handle for the SRTF scheduler task
TaskHandle_t schedulerTaskHandle = NULL;

// Task completion flags
/// Flag indicating if the LED task has completed its total execution time
volatile bool ledComplete = false;
/// Flag indicating if the Counter task has completed its total execution time
volatile bool counterComplete = false;
/// Flag indicating if the Alphabet task has completed its total execution time
volatile bool alphabetComplete = false;

/// Initialization flag for the I2C LCD
volatile bool lcdInitialized = false;

/// LCD Configuration object (assuming address 0x27, 16 columns, 2 rows)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ========================= FUNCTION PROTOTYPES ===========================
void myTaskDelay(TickType_t delayTicks, int taskNum);
void ledTask(void *arg);
void counterTask(void *arg);
void alphabetTask(void *arg);
void scheduleTasks(void *arg);
void setup();
void loop();

// ========================= FUNCTION DEFINITIONS ===========================

/**
 * @brief Custom task delay function that records the task's future wake time
 * and suspends the task.
 * * This effectively implements a non-blocking delay mechanism for the SRTF scheduler.
 *
 * @param delayTicks The number of ticks the task intends to delay (i.e., its current work slice).
 * @param taskNum Identifier for the calling task (1=LED, 2=Counter, 3=Alphabet) to update the correct wake time variable.
 */
void myTaskDelay(TickType_t delayTicks, int taskNum) {
    TickType_t wakeTime = xTaskGetTickCount() + delayTicks;

    // Calculate the wake time: current time + delay
    if (taskNum == 1) {
        ledWakeTime = wakeTime;
    } else if (taskNum == 2) {
        counterWakeTime = wakeTime;
    } else if (taskNum == 3) {
        alphabetWakeTime = wakeTime;
    }

    vTaskSuspend(NULL);
}

/**
 * @brief Task that blinks an LED on a dedicated pin.
 * * The task uses a small work slice, tracks execution time, and decrements its
 * remaining time for SRTF scheduling. It completes after its total execution time is met.
 * * @param arg Unused parameter (required by FreeRTOS task function signature).
 */
void ledTask(void *arg) {
    TickType_t ticksElapsed = 0;

    while (1) {
        vTaskSuspend(NULL);
        if (remainingLedTime > 0) {

            // Blink the LED logic, running in small time chunks
            ticksElapsed += workSliceDelay;
            if (ticksElapsed >= pdMS_TO_TICKS(50)) {
                digitalWrite(LED_PIN, !digitalRead(LED_PIN));
                vTaskDelay(pdMS_TO_TICKS(50));
                digitalWrite(LED_PIN, !digitalRead(LED_PIN));
                vTaskDelay(pdMS_TO_TICKS(50));
                ticksElapsed = 0;
            }

            // Decrement remaining time
            if (remainingLedTime > workSliceDelay) {
                remainingLedTime -= workSliceDelay;
            } else {
                remainingLedTime = 0;
            }
            myTaskDelay(workSliceDelay, 1);

        } else {
            // Mark Complete
            ledWakeTime = 0;
            ledComplete = true;
            digitalWrite(LED_PIN, LOW); // Ensure LED is off when complete
        }
    }
}

/**
 * @brief Task that displays an incrementing counter (1-20) on the I2C LCD display.
 * * The task updates its remaining execution time for SRTF scheduling and clears the LCD
 * when it completes its total execution time.
 * * @param arg Unused parameter (required by FreeRTOS task function signature).
 */
void counterTask(void *arg) {
    // Wait for LCD to be initialized
    while (!lcdInitialized) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SRTF Scheduler");
    lcd.setCursor(0, 1);
    lcd.print("Counter:");

    int count = 1;
    TickType_t ticksElapsed = 0;

    while (1) {
        vTaskSuspend(NULL);
        if (remainingCounterTime > 0) {
            ticksElapsed += workSliceDelay;
            if (ticksElapsed >= pdMS_TO_TICKS(100)) { // Update counter every 100ms slice

                if (count <= 20) {
                    lcd.setCursor(8, 1);
                    lcd.print(count);
                    lcd.print(" "); // Clear trailing digit if needed

                    count++;
                }
                ticksElapsed = 0;
            }

            // Decrement remaining time
            if (remainingCounterTime > workSliceDelay) {
                remainingCounterTime -= workSliceDelay;
            } else {
                remainingCounterTime = 0;
            }
            myTaskDelay(workSliceDelay, 2);

        } else {
            counterWakeTime = 0;
            count = 1; // Reset counter for next cycle
            // Clear and reprint fixed text
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("SRTF Scheduler");
            lcd.setCursor(0, 1);
            lcd.print("Counter: ");
            counterComplete = true;
        }
    }
}


/**
 * @brief Task that prints the alphabet (A-Z) to Serial output.
 * * The task prints one letter every 500ms slice and tracks its execution time
 * for SRTF scheduling. Resets the letter to 'A' upon completion.
 * * @param arg Unused parameter (required by FreeRTOS task function signature).
 */
void alphabetTask(void *arg) {
    char currLetter = 'A';
    TickType_t ticksElapsed = 0;

    while (1) {
        vTaskSuspend(NULL);
        if (remainingAlphabetTime > 0) {
            ticksElapsed += workSliceDelay;
            if (ticksElapsed >= pdMS_TO_TICKS(500)) { // Print letter every 500ms slice

                // Print letters A-Z or until execution time exhausted
                Serial.print(currLetter);
                if (currLetter < 'Z') {
                    Serial.print(", ");
                }

                currLetter++;
                if (currLetter > 'Z') {
                    currLetter = 'Z'; // Stop printing beyond 'Z' until reset
                }
                ticksElapsed = 0;
            }


            // Decrement remaining time
            if (remainingAlphabetTime > workSliceDelay) {
                remainingAlphabetTime -= workSliceDelay;
            } else {
                remainingAlphabetTime = 0;
            }

            myTaskDelay(workSliceDelay, 3);

        } else {
            alphabetWakeTime = 0;
            currLetter = 'A';
            alphabetComplete = true;
        }
    }
}


/**
 * @brief Implements the preemptive SRTF (Shortest Remaining Time First) scheduler.
 * * This task runs at a higher priority, continuously evaluates the remaining time 
 * for all incomplete, awake tasks, and resumes the task with the shortest remaining time.
 * It also handles resetting all tasks when a full cycle (all tasks complete) is achieved.
 * * @param arg Unused parameter (required by FreeRTOS task function signature).
 */
void scheduleTasks(void *arg) {
    while (1) {
        // Check if all tasks have completed their work and reset timing variables if true
        if (ledComplete && counterComplete && alphabetComplete) {
            // Reset task flags and remaining times if all tasks have been completed
            Serial.println("\n=== All tasks completed! Resetting... ===\n");

            // Reset completion flags
            ledComplete = false;
            counterComplete = false;
            alphabetComplete = false;

            // Reset wake times
            ledWakeTime = pdMS_TO_TICKS(0);
            counterWakeTime = pdMS_TO_TICKS(0);
            alphabetWakeTime = pdMS_TO_TICKS(0);

            // Reset remaining task times
            remainingLedTime = ledTaskExecutionTime;
            remainingCounterTime = counterTaskExecutionTime;
            remainingAlphabetTime = alphabetTaskExecutionTime;
        }

        // Determine next task based on smallest remaining time
        TickType_t minTime = portMAX_DELAY;
        TaskHandle_t nextTask = NULL;
        TickType_t currentTime = xTaskGetTickCount();

        // Evaluate led task (1)
        if (!ledComplete && remainingLedTime < minTime && currentTime >= ledWakeTime) {
            minTime = remainingLedTime;
            nextTask = ledTaskHandle;
        }

        // Evaluate counter task (2). Use <= to break ties in favor of lower task ID
        if (!counterComplete && remainingCounterTime <= minTime && currentTime >= counterWakeTime) {
            minTime = remainingCounterTime;
            nextTask = counterTaskHandle;
        }

        // Evaluate alphabet task (3). Use <= to break ties in favor of lower task ID
        if (!alphabetComplete && remainingAlphabetTime <= minTime && currentTime >= alphabetWakeTime) {
            minTime = remainingAlphabetTime;
            nextTask = alphabetTaskHandle;
        }

        // Suspend tasks not selected to ensure only the shortest runs.
        // This is necessary because vTaskResume() is non-preemptive with equal priority tasks.
        if (nextTask != ledTaskHandle && !ledComplete) {
            vTaskSuspend(ledTaskHandle);
        }
        if (nextTask != counterTaskHandle && !counterComplete) {
            vTaskSuspend(counterTaskHandle);
        }
        if (nextTask != alphabetTaskHandle && !alphabetComplete) {
            vTaskSuspend(alphabetTaskHandle);
        }

        // Execute SRTF task (nextTask). Safe to call even if nextTask is NULL (all complete/asleep)
        if (nextTask != NULL) {
            vTaskResume(nextTask);
        } else {
             // If all tasks are complete or asleep, just wait a short period before re-evaluating
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        // Small delay for execution of nextTask before preemption occurs
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}


/**
 * @brief Arduino standard setup function.
 * * Initializes Serial communication, the I2C LCD, and creates all FreeRTOS tasks.
 * Worker tasks are immediately suspended, and the high-priority scheduler task
 * is started last.
 */
void setup() {
    Serial.begin(115200);
    // Initialize I2C LCD
    Wire.begin();
    lcd.init();
    lcd.backlight();
    lcdInitialized = true;
    delay(50);

    pinMode(LED_PIN, OUTPUT);
    
    // Create worker tasks and pin them to core 0 (priority 1)
    xTaskCreatePinnedToCore(ledTask, "LED Blinker", 2048, NULL, 1, &ledTaskHandle, 0);
    xTaskCreatePinnedToCore(counterTask, "Counter", 2048, NULL, 1, &counterTaskHandle, 0);
    xTaskCreatePinnedToCore(alphabetTask, "Alphabet Printer", 4096, NULL, 1, &alphabetTaskHandle, 0);


    // Suspend the worker tasks immediately after creation.
    vTaskSuspend(ledTaskHandle);
    vTaskSuspend(counterTaskHandle);
    vTaskSuspend(alphabetTaskHandle);

    // Create the scheduler task (priority 2, higher than workers) and pin to core 0
    xTaskCreatePinnedToCore(scheduleTasks, "Scheduler", 4096, NULL, 2, &schedulerTaskHandle, 0);
}

/**
 * @brief Arduino standard loop function.
 * * Remains empty as all work is handled by FreeRTOS tasks.
 */
void loop() {}