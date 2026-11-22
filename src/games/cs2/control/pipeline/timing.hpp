#pragma once
#include "games/cs2/control/policy.hpp"
namespace mousebox::games::cs2::pipeline
{
inline double predictionHorizon(double sampleAgeMs, double responseMs, TimeFactors factors = {})
{
    return (factors.sampleAge ? sampleAgeMs : 0) + (factors.responseLead ? responseMs : 0);
}
} // namespace mousebox::games::cs2::pipeline
