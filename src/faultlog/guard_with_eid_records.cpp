#include <attributes_info.H>

#include <guard_with_eid_records.hpp>
#include <libguard/guard_interface.hpp>
#include <phosphor-logging/lg2.hpp>
#include <util.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

extern "C"
{
#include <libpdbg.h>
}

namespace openpower::faultlog
{
using ::nlohmann::json;

constexpr auto stateConfigured = "CONFIGURED";
constexpr auto stateDeconfigured = "DECONFIGURED";

/** Data structure to pass to pdbg_target_traverse callback method*/
struct GuardedTarget
{
    pdbg_target* target = nullptr;
    std::string phyDevPath = {};
    GuardedTarget(const std::string& path) : phyDevPath(path)
    {}
};
using ::sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
/**
 * @brief Get PDBG target matching the guarded target physicalpath
 *
 * This callback function is called as part of the recursive method
 * pdbg_target_traverse, recursion will exit when the method return 1
 * else continues till the target is found
 *
 * @param[in] target - pdbg target to compare
 * @param[inout] priv - data structure passed to the callback method
 *
 * @return 1 when target is found else 0
 */
static int getGuardedTarget(struct pdbg_target* target, void* priv)
{
    GuardedTarget* guardTarget = reinterpret_cast<GuardedTarget*>(priv);
    ATTR_PHYS_DEV_PATH_Type phyPath;
    if (!DT_GET_PROP(ATTR_PHYS_DEV_PATH, target, phyPath))
    {
        if (strcmp(phyPath, guardTarget->phyDevPath.c_str()) == 0)
        {
            guardTarget->target = target;
            return 1;
        }
    }
    return 0;
}

int GuardWithEidRecords::getCount(const GuardRecords& guardRecords)
{
    int count = 0;
    for (const auto& elem : guardRecords)
    {
        // ignore manual guard records
        if (elem.elogId == 0)
        {
            continue;
        }
        count += 1;
    }
    return count;
}

void GuardWithEidRecords::populate(sdbusplus::bus::bus& bus,
                                   const GuardRecords& guardRecords,
                                   json& jsonNag)
{

    for (const auto& elem : guardRecords)
    {
        try
        {
            // ignore manual guard records
            if (elem.elogId == 0)
            {
                continue;
            }

            // add ERROR_LOG secion
            uint32_t bmcLogId;
            auto method1 = bus.new_method_call(
                "xyz.openbmc_project.Logging", "/xyz/openbmc_project/logging",
                "org.open_power.Logging.PEL", "GetBMCLogIdFromPELId");

            method1.append(static_cast<uint32_t>(elem.elogId));
            auto resp1 = bus.call(method1);
            resp1.read(bmcLogId);
            json jsonErrorLog = json::object();
            std::string pel;

            auto method2 = bus.new_method_call(
                "xyz.openbmc_project.Logging", "/xyz/openbmc_project/logging",
                "org.open_power.Logging.PEL", "GetPELJSON");
            method2.append(bmcLogId);
            auto resp2 = bus.call(method2);
            resp2.read(pel);
            json pelJson = std::move(json::parse(pel));

            jsonErrorLog["ERR_PLID"] =
                pelJson["Private Header"]["Platform Log Id"];
            jsonErrorLog["Callout Section"] =
                pelJson["Primary SRC"]["Callout Section"];
            jsonErrorLog["SRC"] = pelJson["Primary SRC"]["Reference Code"];
            jsonErrorLog["DATE_TIME"] = pelJson["Private Header"]["Created at"];

            json jsonErrorLogSection = json::array();
            jsonErrorLogSection.push_back(std::move(jsonErrorLog));

            // add resource actions section
            auto physicalPath =
                openpower::guard::getPhysicalPath(elem.targetId);
            if (!physicalPath.has_value())
            {
                lg2::error("Failed to get physical path for record {RECORD_ID}",
                           "RECORD_ID", elem.recordId);
                continue;
            }

            GuardedTarget guardedTarget(*physicalPath);
            pdbg_target_traverse(nullptr, getGuardedTarget, &guardedTarget);
            if (guardedTarget.target == nullptr)
            {
                lg2::error("Failed to find the pdbg target for the guarded "
                           "target {RECORD_ID}",
                           "RECORD_ID", elem.recordId);
                continue;
            }
            json jsonResource = json::object();
            jsonResource["TYPE"] = pdbg_target_name(guardedTarget.target);
            std::string state = stateDeconfigured;
            ATTR_HWAS_STATE_Type hwasState;
            if (!DT_GET_PROP(ATTR_HWAS_STATE, guardedTarget.target, hwasState))
            {
                if (hwasState.functional)
                {
                    state = stateConfigured;
                }
            }
            jsonResource["CURRENT_STATE"] = std::move(state);

            ATTR_LOCATION_CODE_Type attrLocCode;
            if (!DT_GET_PROP(ATTR_LOCATION_CODE, guardedTarget.target,
                             attrLocCode))
            {
                jsonResource["LOCATION_CODE"] = attrLocCode;
            }

            jsonResource["REASON_DESCRIPTION"] =
                getGuardReason(guardRecords, *physicalPath);

            jsonResource["GARD_RECORD"] = true;

            json jsonEventData = json::object();
            jsonEventData["RESOURCE_ACTIONS"] = std::move(jsonResource);
            jsonErrorLogSection.push_back(jsonEventData);

            json jsonErrlogObj = json::object();
            jsonErrlogObj["CEC_ERROR_LOG"] = std::move(jsonErrorLogSection);

            json jsonServiceEvent = json::object();
            jsonServiceEvent["SERVICABLE_EVENT"] = std::move(jsonErrlogObj);
            jsonNag.push_back(std::move(jsonServiceEvent));
        }
        catch (const InvalidArgument& ex)
        {
            lg2::info(
                "PEL might be deleted but guard entry is around {ELOG_ID)",
                "ELOG_ID", elem.elogId);
        }
        catch (const std::exception& ex)
        {
            lg2::info("Failed to add guard records {ELOG_ID}, {ERROR}",
                      "ELOG_ID", elem.elogId, "ERROR", ex.what());
        }
    }
}
} // namespace openpower::faultlog
