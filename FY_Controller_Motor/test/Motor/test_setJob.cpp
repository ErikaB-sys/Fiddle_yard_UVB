#include <Arduino.h>
#include <unity.h>
#include "Protokoll.h"
#include "Motor.h"

extern REF_STATE FY_Refrun;

/**
 * @brief Prüft die Annahme eines gültigen, referenzfreien Kommandos.
 *
 * GET_STATUS benötigt laut CommandDefinition keine Referenz.
 * Erwartung: setJob() akzeptiert das Kommando im Zustand IDLE.
 */
void test_setJob_accepts_valid_command_without_reference()
{
    Motor motor;

    TEST_ASSERT_TRUE(motor.begin(4, 3, 2));

    MotorJob_t job{};
    job.cmd = CMD_GET_STATUS;

    TEST_ASSERT_TRUE(motor.setJob(job));
}

/**
 * @brief Prüft die Ablehnung einer unbekannten Command-ID.
 *
 * Erwartung: Eine Command-ID, die nicht in commandDefinitions vorhanden ist,
 * wird nicht angenommen.
 */
void test_setJob_rejects_unknown_command()
{
    Motor motor;

    TEST_ASSERT_TRUE(motor.begin(4, 3, 2));

    MotorJob_t job{};
    job.cmd = 0xFF;

    TEST_ASSERT_FALSE(motor.setJob(job));
}

/**
 * @brief Prüft die Referenzbedingung für ein Bewegungskommando.
 *
 * LEFT benötigt laut CommandDefinition eine gültige Referenz.
 * Ohne Referenz darf setJob() das Kommando nicht annehmen.
 */
void test_setJob_rejects_reference_required_command_without_reference()
{
    Motor motor;

    TEST_ASSERT_TRUE(motor.begin(4, 3, 2));
    FY_Refrun = REF_STATE::REF_REQ;

    MotorJob_t job{};
    job.cmd = CMD_LEFT;

    TEST_ASSERT_FALSE(motor.setJob(job));
}

/**
 * @brief Prüft die Annahme eines Bewegungsjobs nach gültiger Referenz.
 *
 * Erwartung: setJob() akzeptiert CMD_LEFT im Zustand IDLE, wenn die
 * Referenz gültig ist.
 */
void test_setJob_accepts_reference_required_command_after_reference()
{
    Motor motor;

    TEST_ASSERT_TRUE(motor.begin(4, 3, 2));
    FY_Refrun = REF_STATE::REF_VALID;

    MotorJob_t job{};
    job.cmd = CMD_LEFT;

    TEST_ASSERT_TRUE(motor.setJob(job));
}

/**
 * @brief Prüft, dass ein Motor im MOVING-Zustand keinen zweiten Job annimmt.
 *
 * Der Test nutzt bewusst die öffentliche Schnittstelle:
 * setJob() nimmt den LEFT-Auftrag an, Update() führt ihn in den
 * Bewegungszustand und ein zweiter Auftrag muss danach abgelehnt werden.
 */
void test_setJob_rejects_job_while_motor_is_busy()
{
    Motor motor;

    TEST_ASSERT_TRUE(motor.begin(4, 3, 2));
    FY_Refrun = REF_STATE::REF_VALID;

    MotorJob_t first{};
    first.cmd = CMD_LEFT;

    TEST_ASSERT_TRUE(motor.setJob(first));
    motor.Update();

    TEST_ASSERT_EQUAL(MotorState_t::MOVING, motor.motor_getState());
    TEST_ASSERT_TRUE(motor.isMoving());

    MotorJob_t second{};
    second.cmd = CMD_RIGHT;

    TEST_ASSERT_FALSE(motor.setJob(second));
}

void setup()
{
    UNITY_BEGIN();
    RUN_TEST(test_setJob_accepts_valid_command_without_reference);
    RUN_TEST(test_setJob_rejects_unknown_command);
    RUN_TEST(test_setJob_rejects_reference_required_command_without_reference);
    RUN_TEST(test_setJob_accepts_reference_required_command_after_reference);
    RUN_TEST(test_setJob_rejects_job_while_motor_is_busy);
    UNITY_END();
}

void loop()
{
}
