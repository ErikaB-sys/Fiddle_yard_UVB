#include <Arduino.h>
#include <unity.h>
#include "Protokoll.h"
#include "Motor.h"

extern REF_STATE FY_Refrun;

/**
 * @brief Prüft den eindeutig beschriebenen Happy Path von prepareLeft().
 *
 * Erwartung:
 * - Motor wurde initialisiert.
 * - Referenz ist gültig.
 * - der entsprechende Bewegungsauftrag wird angenommen.
 * - Update() führt den Auftrag in MOVING.
 *
 * Die eigentliche Vorbereitung bleibt intern; der Test prüft bewusst die
 * öffentliche Wirkung der Funktion über setJob() und Update().
 */
void test_prepareLeft_valid_job()
{
    Motor motor;

    TEST_ASSERT_TRUE(motor.begin(4, 3, 2));
    FY_Refrun = REF_STATE::REF_VALID;

    MotorJob_t job{};
    job.cmd = CMD_LEFT;

    TEST_ASSERT_TRUE(motor.setJob(job));
    motor.Update();

    TEST_ASSERT_EQUAL(MotorState_t::MOVING, motor.motor_getState());
    TEST_ASSERT_TRUE(motor.isMoving());
}

void setup()
{
    UNITY_BEGIN();
    RUN_TEST(test_prepareLeft_valid_job);
    UNITY_END();
}

void loop()
{
}
