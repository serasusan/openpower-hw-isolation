#pragma once
#include <sdbusplus/bus.hpp>

namespace openpower::faultlog
{
/**
 * @brief Return time in BCD from milliSeconds since epoch time
 * @param[in] milliSeconds - milli seconds since epoch time
 *
 * @return string time value in string format
 */
std::string epochTimeToBCD(uint64_t milliSeconds);

/**
 * @brief Write current timestamp to a persistent file
 *
 */
void writePowerOnTime(sdbusplus::bus::bus& bus);

/**
 * @brief Read timestamp from poweron persistent file
 *
 * @return poweron time in milliseconds
 */
uint64_t readPowerOnTime(sdbusplus::bus::bus& bus);
} // namespace openpower::faultlog
