#include <attributes_info.H>

#include <deconfig_reason.hpp>
#include <deconfig_records.hpp>
#include <libguard/guard_interface.hpp>
#include <phosphor-logging/lg2.hpp>

extern "C"
{
#include <libpdbg.h>
}
namespace openpower::faultlog
{

using ::nlohmann::json;

constexpr auto stateConfigured = "CONFIGURED";
constexpr auto stateDeconfigured = "DECONFIGURED";

struct DeconfigDataList
{
    std::vector<pdbg_target*> targetList;
    void addPdbgTarget(pdbg_target* tgt)
    {
        targetList.push_back(tgt);
    }
};

/**
 * @brief Get pdbg targets which has been deconfigured
 *
 * This callback function is called as part of the recursive method
 * pdbg_target_traverse, recursion will exit when the method return 1
 * else continues till the target is found
 *
 * @param[in] target - pdbg target to compare
 * @param[inout] priv - data structure to pass to the callback method
 *
 * @return 1 when target is found else 0
 */
static int getDeconfigTargets(struct pdbg_target* target, void* priv)
{
    auto deconfigList = reinterpret_cast<DeconfigDataList*>(priv);
    ATTR_HWAS_STATE_Type hwasState;
    if (!DT_GET_PROP(ATTR_HWAS_STATE, target, hwasState))
    {
        if ((hwasState.deconfiguredByEid != 0) &&
            (DECONFIGURED_BY_PLID_MASK & hwasState.deconfiguredByEid) == 0)
        {
            // inlcude only specific states and other might be by association
            switch (hwasState.deconfiguredByEid)
            {
                case DECONFIGURED_BY_MANUAL_GARD:
                case DECONFIGURED_BY_FIELD_CORE_OVERRIDE:
                case DECONFIGURED_BY_PRD:
                case DECONFIGURED_BY_PHYP:
                case DECONFIGURED_BY_SPCN:
                {
                    deconfigList->addPdbgTarget(target);
                    break;
                }
                default:
                {
                    break;
                }
            }
        }
    }
    return 0;
}

int DeconfigRecords::getCount(const GuardRecords& guardRecords)
{
    std::vector<std::string> pathList;
    for (const auto& elem : guardRecords)
    {
        auto physicalPath = openpower::guard::getPhysicalPath(elem.targetId);
        if (!physicalPath.has_value())
        {
            continue;
        }
        pathList.push_back(*physicalPath);
    }

    DeconfigDataList deconfigList;
    pdbg_target_traverse(nullptr, getDeconfigTargets, &deconfigList);
    DeconfigDataList onlyDeconfigList;
    for (const auto& target : deconfigList.targetList)
    {
        ATTR_PHYS_DEV_PATH_Type attrPhyDevPath;
        if (!DT_GET_PROP(ATTR_PHYS_DEV_PATH, target, attrPhyDevPath))
        {
            // consider only those targets that are not part of guard list
            if (std::find(pathList.begin(), pathList.end(), attrPhyDevPath) !=
                pathList.end())
            {
                onlyDeconfigList.addPdbgTarget(target);
            }
        }
    }
    return static_cast<int>(onlyDeconfigList.targetList.size());
}

void DeconfigRecords::populate(const GuardRecords& guardRecords,
                               nlohmann::json& jsonNag)
{
    // get physical path list of guarded targets
    std::vector<std::string> pathList;
    for (const auto& elem : guardRecords)
    {
        auto physicalPath = openpower::guard::getPhysicalPath(elem.targetId);
        if (!physicalPath.has_value())
        {
            continue;
        }
        pathList.push_back(*physicalPath);
    }

    DeconfigDataList deconfigList;
    pdbg_target_traverse(nullptr, getDeconfigTargets, &deconfigList);

    // consider only those targets that are not part of guard list
    DeconfigDataList onlyDeconfigList;
    for (const auto& target : deconfigList.targetList)
    {
        ATTR_PHYS_DEV_PATH_Type attrPhyDevPath;
        if (!DT_GET_PROP(ATTR_PHYS_DEV_PATH, target, attrPhyDevPath))
        {
            if (std::find(pathList.begin(), pathList.end(), attrPhyDevPath) !=
                pathList.end())
            {
                onlyDeconfigList.addPdbgTarget(target);
            }
        }
    }

    for (const auto& target : onlyDeconfigList.targetList)
    {
        try
        {
            json deconfigJson = json::object();
            if (pdbg_target_name(target) != nullptr)
            {
                deconfigJson["TYPE"] = pdbg_target_name(target);
            }
            std::string state = stateDeconfigured;
            ATTR_HWAS_STATE_Type hwasState;
            if (!DT_GET_PROP(ATTR_HWAS_STATE, target, hwasState))
            {
                if (hwasState.functional)
                {
                    state = stateConfigured;
                }
                std::stringstream ss;
                ss << "0x" << std::hex << hwasState.deconfiguredByEid;
                deconfigJson["PLID"] = ss.str();
                deconfigJson["REASON_DESCRIPTION"] =
                    getDeconfigReason(static_cast<DeconfiguredByReason>(
                        hwasState.deconfiguredByEid));
            }
            deconfigJson["CURRENT_STATE"] = std::move(state);

            ATTR_PHYS_DEV_PATH_Type attrPhyDevPath;
            if (!DT_GET_PROP(ATTR_PHYS_DEV_PATH, target, attrPhyDevPath))
            {
                deconfigJson["PHYS_PATH"] = attrPhyDevPath;
            }
            else
            {
                // if physical path is not found do not add the record
                // as it will be of no use
                continue;
            }

            ATTR_LOCATION_CODE_Type attrLocCode;
            if (!DT_GET_PROP(ATTR_LOCATION_CODE, target, attrLocCode))
            {
                deconfigJson["LOCATION_CODE"] = attrLocCode;
            }

            json header = json::object();
            header["DECONFIGURED"] = std::move(deconfigJson);
            jsonNag.push_back(std::move(header));
        }
        catch (const std::exception& ex)
        {
            lg2::error("Failed to add deconfig records {TARGET} {ERROR}",
                       "TARGET", pdbg_target_name(target), "ERROR", ex.what());
        }
    }
}
} // namespace openpower::faultlog
