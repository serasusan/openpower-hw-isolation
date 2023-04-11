#include <attributes_info.H>

#include <libguard/guard_interface.hpp>
#include <phosphor-logging/log.hpp>
#include <unresolved_pels.hpp>
#include <util.hpp>

extern "C"
{
#include <libpdbg.h>
}

namespace openpower::faultlog
{
using ::nlohmann::json;

using PropertyValue =
    std::variant<std::string, bool, uint8_t, int16_t, uint16_t, int32_t,
                 uint32_t, int64_t, uint64_t, double>;

using Properties = std::map<std::string, PropertyValue>;

using Interfaces = std::map<std::string, Properties>;

using Objects = std::map<sdbusplus::message::object_path, Interfaces>;

constexpr auto stateConfigured = "CONFIGURED";
constexpr auto stateDeconfigured = "DECONFIGURED";

struct GuardedTarget
{
    pdbg_target* target = nullptr;
    std::string phyDevPath;
    GuardedTarget(const std::string& path) : phyDevPath(path)
    {}
};

static int getGuardedTarget(struct pdbg_target* target, void* priv)
{
    // recursive callback function that exits when the target matching the
    // guarded targets physical path is found in the device tree.
    // to exit the recursive function return 1 to continue return 0
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

void UnresolvedPELs::populate(sdbusplus::bus::bus& bus,
                              GuardRecords& guardRecords, json& jsonNag)
{
    try
    {
        Objects objects;
        auto method = bus.new_method_call(
            "xyz.openbmc_project.Logging", "/xyz/openbmc_project/logging",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
        auto reply = bus.call(method);
        reply.read(objects);
        for (const auto& [path, interfaces] : objects)
        {
            bool resolved = true;
            uint32_t id = 0;
            std::string severity =
                "xyz.openbmc_project.Logging.Entry.Level.Informational";
            for (const auto& [intf, properties] : interfaces)
            {
                if (intf != "xyz.openbmc_project.Logging.Entry")
                {
                    continue;
                }
                for (const auto& [prop, propValue] : properties)
                {
                    if (prop == "Id")
                    {
                        auto idPtr = std::get_if<uint32_t>(&propValue);
                        if (idPtr != nullptr)
                        {
                            id = *idPtr;
                        }
                    }
                    else if (prop == "Resolved")
                    {
                        auto resolvedPtr = std::get_if<bool>(&propValue);
                        if (resolvedPtr != nullptr)
                        {
                            resolved = *resolvedPtr;
                        }
                    }
                    else if (prop == "Severity")
                    {
                        auto severityPtr = std::get_if<std::string>(&propValue);
                        if (severityPtr != nullptr)
                        {
                            severity = *severityPtr;
                        }
                    }
                }
                break;
            }
            if (resolved == true)
            {
                continue;
            }

            // ignore informational and debug errors
            if ((severity == "xyz.openbmc_project.Logging.Entry.Level.Debug") ||
                (severity ==
                 "xyz.openbmc_project.Logging.Entry.Level.Informational") ||
                (severity == "xyz.openbmc_project.Logging.Entry.Level.Notice"))
            {
                continue;
            }

            // get pel json file
            std::string pel;
            auto method2 = bus.new_method_call(
                "xyz.openbmc_project.Logging", "/xyz/openbmc_project/logging",
                "org.open_power.Logging.PEL", "GetPELJSON");
            method2.append(id);
            auto resp2 = bus.call(method2);
            resp2.read(pel);
            json pelJson = std::move(json::parse(pel));

            // add cec errorlog
            json jsonErrorLog = json::object();
            jsonErrorLog["Callout Section"] =
                pelJson["Primary SRC"]["Callout Section"];
            bool deconfigured = false;
            json& primarySRC = pelJson["Primary SRC"];
            if (primarySRC.contains("Deconfigured") &&
                !primarySRC["Deconfigured"].is_null())
            {
                if (primarySRC["Deconfigured"] == "True")
                {
                    deconfigured = true;
                }
            }
            if (deconfigured == false)
            {
                continue;
            }

            bool guarded = false;
            if (primarySRC.contains("Guarded") &&
                !primarySRC["Guarded"].is_null())
            {
                if (primarySRC["Guarded"] == "True")
                {
                    guarded = true;
                }
            }
            if (guarded == true)
            {
                continue; // will be captured as part of guard records
            }

            jsonErrorLog["ERR_PLID"] =
                pelJson["Private Header"]["Platform Log Id"];
            jsonErrorLog["Callout Section"] =
                pelJson["Primary SRC"]["Callout Section"];
            jsonErrorLog["SRC"] = pelJson["Primary SRC"]["Reference Code"];
            jsonErrorLog["DATE_TIME"] = pelJson["Private Header"]["Created at"];

            json jsonErrorLogSection = json::array();
            jsonErrorLogSection.push_back(std::move(jsonErrorLog));

            std::string bmcLogIdStr =
                pelJson["Private Header"]["Platform Log Id"];
            uint32_t bmcLogId =
                static_cast<uint32_t>(std::stoul(bmcLogIdStr, nullptr, 16));

            // add resource action check if guard record is found
            json jsonResource = json::object();
            for (const auto& elem : guardRecords)
            {
                if (elem.elogId == bmcLogId)
                {
                    auto physicalPath =
                        openpower::guard::getPhysicalPath(elem.targetId);
                    GuardedTarget guardedTarget(*physicalPath);
                    pdbg_target_traverse(nullptr, getGuardedTarget,
                                         &guardedTarget);
                    if (guardedTarget.target == nullptr)
                    {
                        lg2::info("Failed to find the pdbg target for guarded "
                                  "target {RECORD_ID}",
                                  "RECORD_ID", elem.recordId);
                        continue;
                    }
                    jsonResource["TYPE"] =
                        pdbg_target_name(guardedTarget.target);
                    std::string state = stateDeconfigured;
                    ATTR_HWAS_STATE_Type hwasState;
                    if (!DT_GET_PROP(ATTR_HWAS_STATE, guardedTarget.target,
                                     hwasState))
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

                    break;
                }
            } // endfor
            json jsonEventData = json::object();
            jsonEventData["RESOURCE_ACTIONS"] = std::move(jsonResource);
            jsonErrorLogSection.push_back(jsonEventData);

            json jsonErrlogObj = json::object();
            jsonErrlogObj["CEC_ERROR_LOG"] = std::move(jsonErrorLogSection);
            json jsonServiceEvent = json::object();
            jsonServiceEvent["SERVICABLE_EVENT"] = std::move(jsonErrlogObj);
            jsonNag.push_back(std::move(jsonServiceEvent));
        }
    }
    catch (const std::exception& ex)
    {
        lg2::error(
            "Failed to add unresolved pels with deconfig bit set {ERROR}",
            "ERROR", ex.what());
    }
}
} // namespace openpower::faultlog
