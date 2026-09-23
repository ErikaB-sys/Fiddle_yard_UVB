#include <Arduino.h>
#include <unity.h>

/*
 * Testzugriff auf die private Testeinheit calcProfile().
 *
 * calcProfile() ist bewusst keine öffentliche Motor-Schnittstelle. Für diesen
 * Unit-Test wird der private Bereich ausschließlich im Test sichtbar gemacht;
 * die Produktionsschnittstelle bleibt unverändert.
 */
#define private public
#include "Motor.h"
#undef private

/**
 * @brief Prüft den Sonderfall für sehr kurze Strecken.
 *
 * Erwartung:
 * - calcProfile() liefert true.
 * - die komplette Strecke liegt in KONST.
 * - alle anderen Profilelemente bleiben leer.
 */
void test_calcProfile_very_short_distance()
{
    Motor motor;

    const uint16_t distance = 100;

    TEST_ASSERT_TRUE(motor.calcProfile(distance));

    TEST_ASSERT_EQUAL_UINT16(0,   motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::ACC1)].Steps);
    TEST_ASSERT_EQUAL_UINT16(0,   motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::ACC2)].Steps);
    TEST_ASSERT_EQUAL_UINT16(100, motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::KONST)].Steps);
    TEST_ASSERT_EQUAL_UINT16(0,   motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::BRE1)].Steps);
    TEST_ASSERT_EQUAL_UINT16(0,   motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::BRE2)].Steps);
    TEST_ASSERT_EQUAL_UINT16(0,   motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::POSI)].Steps);
}

/**
 * @brief Prüft exakt die Grenze zwischen Kurzstrecke und vollständigem Profil.
 *
 * Erwartung:
 * Bei PosiMin + KonstMin wird noch die Kurzstreckenregel verwendet.
 */
void test_calcProfile_short_distance_boundary()
{
    Motor motor;

    const uint16_t distance = POS_MIN + KONST_MIN;

    TEST_ASSERT_TRUE(motor.calcProfile(distance));

    TEST_ASSERT_EQUAL_UINT16(
        distance,
        motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::KONST)].Steps);

    TEST_ASSERT_EQUAL_UINT16(
        0,
        motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::POSI)].Steps);
}

/**
 * @brief Prüft ein normales, symmetrisches Fahrprofil.
 *
 * Erwartung für 1000 Steps:
 * ACC1=100, ACC2=100, KONST=440, BRE1=100, BRE2=100, POSI=160.
 * Die Summe muss exakt der angeforderten Strecke entsprechen.
 */
void test_calcProfile_normal_distance()
{
    Motor motor;

    const uint16_t distance = 1000;

    TEST_ASSERT_TRUE(motor.calcProfile(distance));

    TEST_ASSERT_EQUAL_UINT16(100, motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::ACC1)].Steps);
    TEST_ASSERT_EQUAL_UINT16(100, motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::ACC2)].Steps);
    TEST_ASSERT_EQUAL_UINT16(440, motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::KONST)].Steps);
    TEST_ASSERT_EQUAL_UINT16(100, motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::BRE1)].Steps);
    TEST_ASSERT_EQUAL_UINT16(100, motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::BRE2)].Steps);
    TEST_ASSERT_EQUAL_UINT16(160, motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::POSI)].Steps);

    const uint16_t sum =
        motor.Motor_profil[0].Steps +
        motor.Motor_profil[1].Steps +
        motor.Motor_profil[2].Steps +
        motor.Motor_profil[3].Steps +
        motor.Motor_profil[4].Steps +
        motor.Motor_profil[5].Steps;

    TEST_ASSERT_EQUAL_UINT16(distance, sum);
}

/**
 * @brief Prüft eine lange Strecke mit maximalen Beschleunigungssegmenten.
 *
 * Erwartung:
 * ACC1/ACC2/BRE1/BRE2 werden jeweils auf ihre Maximalgröße begrenzt.
 * Der verbleibende Weg landet in KONST.
 */
void test_calcProfile_long_distance()
{
    Motor motor;

    const uint16_t distance = 2000;

    TEST_ASSERT_TRUE(motor.calcProfile(distance));

    TEST_ASSERT_EQUAL_UINT16(100,  motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::ACC1)].Steps);
    TEST_ASSERT_EQUAL_UINT16(100,  motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::ACC2)].Steps);
    TEST_ASSERT_EQUAL_UINT16(1440, motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::KONST)].Steps);
    TEST_ASSERT_EQUAL_UINT16(100,  motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::BRE1)].Steps);
    TEST_ASSERT_EQUAL_UINT16(100,  motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::BRE2)].Steps);
    TEST_ASSERT_EQUAL_UINT16(160,  motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::POSI)].Steps);
}

/**
 * @brief Prüft die wichtigste Invariante von calcProfile().
 *
 * Für eine ungerade Strecke muss die Summe aller sechs Profilbereiche
 * trotzdem exakt der angeforderten Strecke entsprechen.
 */
void test_calcProfile_odd_distance_preserves_total()
{
    Motor motor;

    const uint16_t distance = 1235;

    TEST_ASSERT_TRUE(motor.calcProfile(distance));

    const uint16_t sum =
        motor.Motor_profil[0].Steps +
        motor.Motor_profil[1].Steps +
        motor.Motor_profil[2].Steps +
        motor.Motor_profil[3].Steps +
        motor.Motor_profil[4].Steps +
        motor.Motor_profil[5].Steps;

    TEST_ASSERT_EQUAL_UINT16(distance, sum);
}

/**
 * @brief Prüft die Symmetrie der Beschleunigungs- und Bremswege.
 *
 * Erwartung:
 * ACC1 == BRE2 und ACC2 == BRE1.
 */
void test_calcProfile_profile_symmetry()
{
    Motor motor;

    TEST_ASSERT_TRUE(motor.calcProfile(1500));

    TEST_ASSERT_EQUAL_UINT16(
        motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::ACC1)].Steps,
        motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::BRE2)].Steps);

    TEST_ASSERT_EQUAL_UINT16(
        motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::ACC2)].Steps,
        motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::BRE1)].Steps);
}

void setup()
{
    UNITY_BEGIN();
    RUN_TEST(test_calcProfile_very_short_distance);
    RUN_TEST(test_calcProfile_short_distance_boundary);
    RUN_TEST(test_calcProfile_normal_distance);
    RUN_TEST(test_calcProfile_long_distance);
    RUN_TEST(test_calcProfile_odd_distance_preserves_total);
    RUN_TEST(test_calcProfile_profile_symmetry);
    UNITY_END();
}

void loop()
{
}
