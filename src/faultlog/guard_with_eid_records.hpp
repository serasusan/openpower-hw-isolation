#pragma once

#include <libguard/include/guard_record.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>

namespace openpower::faultlog
{
using ::openpower::guard::GuardRecords;

/**
 * @class GuardWithEidRecords
 *
 *  Captures all permanent hardware isolation or hardware error records
 *  (guard) that has associated errorlog object created in JSON file.
 */
class GuardWithEidRecords
{
  private:
    GuardWithEidRecords() = delete;
    GuardWithEidRecords(const GuardWithEidRecords&) = delete;
    GuardWithEidRecords& operator=(const GuardWithEidRecords&) = delete;
    GuardWithEidRecords(GuardWithEidRecords&&) = delete;
    GuardWithEidRecords& operator=(GuardWithEidRecords&&) = delete;
    ~GuardWithEidRecords() = delete;

  public:
    /** @brief Get guard records with associated error log count
     *
     *  @param[in] bus - D-Bus to attach to
     *  @param[in] guardRecords - hardware isolated records to parse
     *
     *  @return 0 if no records are found else count of records
     */
    static int getCount(sdbusplus::bus::bus& bus,
                        const GuardRecords& guardRecords);

    /** @brief Populate permanent hardware errors to NAG json file
     *
     *  @param[in] bus - D-Bus to attach to
     *  @param[in] guardRecords - hardware isolated records to parse
     *  @param[in] jsonServEvent - Json capturing cec error log data
     */
    static void populate(sdbusplus::bus::bus& bus,
                         const GuardRecords& guardRecords,
                         nlohmann::json& jsonServEvent);
};
} // namespace openpower::faultlog
