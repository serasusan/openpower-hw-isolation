#pragma once
#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>

namespace openpower::faultlog
{
/**
 * @class FaultLogPolicy
 *
 * Capture faultlog policy and FCO value
 */
class FaultLogPolicy
{
  private:
    FaultLogPolicy() = delete;
    FaultLogPolicy(const FaultLogPolicy&) = delete;
    FaultLogPolicy& operator=(const FaultLogPolicy&) = delete;
    FaultLogPolicy(FaultLogPolicy&&) = delete;
    FaultLogPolicy& operator=(FaultLogPolicy&&) = delete;
    ~FaultLogPolicy() = delete;

  public:
    /** @brief Populate hardware isolation policy and FCO value
     *
     *  @param[in] bus - D-Bus to attach to
     *  @param[in] json - nag JSON file
     *  @return void
     */
    static void populate(sdbusplus::bus::bus& bus, nlohmann::json& nagJson);
};
} // namespace openpower::faultlog
