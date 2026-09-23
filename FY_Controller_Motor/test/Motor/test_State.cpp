#include <Arduino.h>
#include <unity.h>
#include "Protokoll.h"
#include "Motor.h"

/**
 * @brief Prüft den definierten Anfangszustand des Motors nach begin().
 *
 * Erwartung:
 * - Motor befindet sich im Zustand IDLE.
 * - Motor meldet keine Bewegung.
 * - Position ist noch unbekannt, weil noch kein Referenzlauf erfolgt ist.
 */
void test_motor_initial_state()
{
    Motor motor;

    TEST_ASSERT_TRUE(motor.begin(4, 3, 2));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(MotorState_t::IDLE), static_cast<int>(motor.motor_getState()));
    TEST_ASSERT_FALSE(motor.isMoving());
    TEST_ASSERT_EQUAL_UINT16(POSITION_UNKNOWN, motor.getPosition());
}

/**
 * @brief Prüft isMoving() im definierten IDLE-Zustand.
 *
 * Erwartung: IDLE wird nicht als Bewegung gemeldet.
 */
void test_motor_isMoving_idle()
{
    Motor motor;

    TEST_ASSERT_TRUE(motor.begin(4, 3, 2));
    TEST_ASSERT_FALSE(motor.isMoving());
}

void setup()
{
    UNITY_BEGIN();
    RUN_TEST(test_motor_initial_state);
    RUN_TEST(test_motor_isMoving_idle);
    UNITY_END();
}

void loop()
{
}
