#include <libguard/guard_interface.hpp>
#include <util.hpp>

namespace openpower::faultlog
{
std::string getGuardReason(const GuardRecords& guardRecords,
                           const std::string& path)
{
    for (const auto& elem : guardRecords)
    {
        auto physicalPath = openpower::guard::getPhysicalPath(elem.targetId);
        if (!physicalPath.has_value())
        {
            lg2::error("Failed to get physical path for record {RECORD_ID}",
                       "RECORD_ID", elem.recordId);
            continue;
        }
        std::string temp(*physicalPath);
        if (temp.find(path) != std::string::npos)
        {
            return openpower::guard::guardReasonToStr(elem.errType);
        }
    }
    return "Unknown";
}
} // namespace openpower::faultlog
