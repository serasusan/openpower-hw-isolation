#include <guard_with_eid_records.hpp>
#include <libguard/guard_interface.hpp>
#include <libguard/include/guard_record.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>

#include <iostream>

using namespace openpower::faultlog;
using ::nlohmann::json;
using ::openpower::guard::GuardRecords;

#define GUARD_RESOLVED 0xFFFFFFFF

int main(int /*arg*/, char** /*argv*/)
{
    try
    {
        auto bus = sdbusplus::bus::new_default();

        nlohmann::json faultLogJson = json::array();

        openpower::guard::libguard_init();
        // Don't get ephemeral records because those type records are not
        // intended to expose to the end user, just created for internal purpose
        // to use by the BMC and Hostboot.
        openpower::guard::GuardRecords records = openpower::guard::getAll(true);
        GuardRecords unresolvedRecords;
        // filter out all unused or resolved records
        for (const auto& elem : records)
        {
            if (elem.recordId != GUARD_RESOLVED)
            {
                unresolvedRecords.emplace_back(elem);
            }
        }

        GuardWithEidRecords::populate(bus, unresolvedRecords, faultLogJson);
        std::cout << faultLogJson.dump(2) << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
    return 0;
}
