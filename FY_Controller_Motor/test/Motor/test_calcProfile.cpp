#include <Arduino.h>
#include <unity.h>

#include "Motor.h"

Motor motor;


void test_calcProfile_positive(void)
{
    const uint16_t distance = 1000;

    TEST_ASSERT_TRUE(motor.calcProfile(distance));

    uint16_t sum = 0;

    for (uint8_t i = 0; i < 6; i++)
    {
        sum += motor.Motor_profil[i].Steps;
    }

    TEST_ASSERT_EQUAL_UINT16(distance, sum);

    TEST_ASSERT_EQUAL_UINT16(
        motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::ACC1)].Steps,
        motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::BRE2)].Steps
    );

    TEST_ASSERT_EQUAL_UINT16(
        motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::ACC2)].Steps,
        motor.Motor_profil[static_cast<uint8_t>(MotorProfile_t::BRE1)].Steps
    );
}


void setup()
{
    UNITY_BEGIN();

    RUN_TEST(test_calcProfile_positive);

    UNITY_END();
}


void loop()
{
}