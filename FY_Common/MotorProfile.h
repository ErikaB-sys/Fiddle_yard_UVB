#pragma once

#include <stdint.h>

// -----------------------------------------------------------------------------
// Gemeinsame Fahrprofilberechnung
//
// Die Berechnung ist bewusst hardwarefrei. Sie kennt weder Timer noch ISR.
// Beide Seiten - Produktions-Motor und HW-Test - können dieselbe Mathematik
// verwenden.
// -----------------------------------------------------------------------------

enum class MotionProfileElement_t : uint8_t
{
    ACC1,
    ACC2,
    KONST,
    BRE1,
    BRE2,
    POSI
};

struct MotionProfileElementData_t
{
    uint16_t Steps;
    int16_t Accel;
};

struct MotionProfileParam_t
{
    uint16_t PosiMin;
    uint16_t KonstMin;

    uint16_t Acc1Steps;
    uint16_t Acc2Steps;

    int16_t Acc1Accel;
    int16_t Acc2Accel;
    int16_t Bre1Accel;
    int16_t Bre2Accel;
};

// Berechnet die sechs Profilsegmente.
// Die Summe aller Steps entspricht exakt distance.
inline bool calculateMotionProfile(
    uint16_t distance,
    const MotionProfileParam_t& param,
    MotionProfileElementData_t profile[6])
{
    for (uint8_t i = 0; i < 6; ++i)
    {
        profile[i].Steps = 0;
        profile[i].Accel = 0;
    }

    if (distance <= (param.PosiMin + param.KonstMin))
    {
        profile[static_cast<uint8_t>(MotionProfileElement_t::KONST)].Steps =
            distance;
        return true;
    }

    profile[static_cast<uint8_t>(MotionProfileElement_t::POSI)].Steps =
        param.PosiMin;

    profile[static_cast<uint8_t>(MotionProfileElement_t::KONST)].Steps =
        param.KonstMin;

    uint16_t remaining =
        distance - param.PosiMin - param.KonstMin;

    uint16_t outer = remaining / 2;

    if (outer > param.Acc1Steps)
        outer = param.Acc1Steps;

    profile[static_cast<uint8_t>(MotionProfileElement_t::ACC1)].Steps =
        outer;
    profile[static_cast<uint8_t>(MotionProfileElement_t::ACC1)].Accel =
        param.Acc1Accel;

    profile[static_cast<uint8_t>(MotionProfileElement_t::BRE2)].Steps =
        outer;
    profile[static_cast<uint8_t>(MotionProfileElement_t::BRE2)].Accel =
        param.Bre2Accel;

    remaining -= outer * 2;

    uint16_t inner = remaining / 2;

    if (inner > param.Acc2Steps)
        inner = param.Acc2Steps;

    profile[static_cast<uint8_t>(MotionProfileElement_t::ACC2)].Steps =
        inner;
    profile[static_cast<uint8_t>(MotionProfileElement_t::ACC2)].Accel =
        param.Acc2Accel;

    profile[static_cast<uint8_t>(MotionProfileElement_t::BRE1)].Steps =
        inner;
    profile[static_cast<uint8_t>(MotionProfileElement_t::BRE1)].Accel =
        param.Bre1Accel;

    remaining -= inner * 2;

    profile[static_cast<uint8_t>(MotionProfileElement_t::KONST)].Steps +=
        remaining;

    uint16_t sum = 0;

    for (uint8_t i = 0; i < 6; ++i)
        sum += profile[i].Steps;

    if (sum < distance)
    {
        profile[static_cast<uint8_t>(MotionProfileElement_t::POSI)].Steps +=
            distance - sum;
    }

    return true;
}
