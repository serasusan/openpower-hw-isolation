#pragma once

#include <libguard/include/guard_record.hpp>
#include <nlohmann/json.hpp>

namespace openpower::faultlog
{
using ::openpower::guard::GuardRecords;
/**
 * @class DeconfigRecords
 *
 * Capture Deconfig records in JSON file
 *
 * Filed code override is a method of enabling only a
 * limited number of processor cores in the system.
 */
class DeconfigRecords
{
  private:
    DeconfigRecords() = delete;
    DeconfigRecords(const DeconfigRecords&) = delete;
    DeconfigRecords& operator=(const DeconfigRecords&) = delete;
    DeconfigRecords(DeconfigRecords&&) = delete;
    DeconfigRecords& operator=(DeconfigRecords&&) = delete;
    ~DeconfigRecords() = delete;

  public:
    /** @brief Get deconfigured records count by parsing through pdbg targets
     *
     *  @return 0 if no records are found else count of records
     *  @param[in] guardRecords - list of guarded targets to ignore as part of
     * parsing
     */
    static int getCount(const GuardRecords& guardRecords);

    /** @brief Populate target details that have deconfiguredByEid set
     *
     *  @param[in] guardRecords - list of guarded targets to ignore as part of
     *  @param[inout] jsonNag - Update JSON deconfigure records
     * parsing
     */
    static void populate(const GuardRecords& guardRecords,
                         nlohmann::json& jsonNag);
};
} // namespace openpower::faultlog
