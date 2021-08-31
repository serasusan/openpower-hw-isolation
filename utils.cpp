// SPDX-License-Identifier: Apache-2.0

#include "utils.hpp"

#include "openpower_guard_interface.hpp"
#include "phal_devtree_utils.hpp"

#include <xyz/openbmc_project/State/Chassis/server.hpp>

namespace hw_isolation
{
namespace utils
{

void initExternalModules()
{
    devtree::initPHAL();

    // Don't initialize the phal device tree again, it will init through
    // devtree::initPHAL because, phal device tree should initialize
    // only once (as per pdbg expectation) in single process context.
    // so, passing as false to libguard_init.
    openpower_guard::libguard::libguard_init(false);
}

std::string getDBusServiceName(sdbusplus::bus::bus& bus,
                               const std::string& path,
                               const std::string& interface)
{
    std::vector<std::pair<std::string, std::vector<std::string>>> servicesName;

    try
    {
        auto method =
            bus.new_method_call(type::ObjectMapperName, type::ObjectMapperPath,
                                type::ObjectMapperName, "GetObject");

        method.append(path);
        method.append(std::vector<std::string>({interface}));

        auto reply = bus.call(method);
        reply.read(servicesName);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>(fmt::format("Exception [{}] to get dbus service name "
                                    "for object [{}] and interface [{}]",
                                    e.what(), path, interface)
                            .c_str());
        throw sdbusplus::exception::SdBusError(
            const_cast<sd_bus_error*>(e.get_error()), "HW-Isolation");
    }

    // In OpenBMC, the object path will be hosted by a single service
    // i.e more than one service cannot host the same object path.
    if (servicesName.size() > 1)
    {
        std::string serviceNameList{""};

        std::for_each(servicesName.begin(), servicesName.end(),
                      [&serviceNameList](const auto& serviceName) {
                          serviceNameList.append(serviceName.first + ",");
                      });

        log<level::ERR>(fmt::format("The given object path hosted by "
                                    "more than one services [{}]",
                                    serviceNameList)
                            .c_str());
        throw std::runtime_error(
            "Given object path hosted by more than one service");
    }

    return servicesName[0].first;
}

bool isHwIosolationPolicyEnabled(sdbusplus::bus::bus& bus)
{
    return utils::getDBusPropertyVal<bool>(
        bus, "/xyz/openbmc_project/hardware_isolation/hw_isolation_policy",
        "xyz.openbmc_project.Object.Enable", "Enabled");
}

bool isHwDeisolationAllowed(sdbusplus::bus::bus& bus)
{
    // Make sure policy is enabled or not
    if (isHwIosolationPolicyEnabled(bus))
    {
        log<level::ERR>(
            fmt::format("HardwareIsolation policy is enabled").c_str());
        return false;
    }

    using Chassis = sdbusplus::xyz::openbmc_project::State::server::Chassis;

    auto systemPowerState = utils::getDBusPropertyVal<std::string>(
        bus, "/xyz/openbmc_project/state/chassis0",
        "xyz.openbmc_project.State.Chassis", "CurrentPowerState");

    if (Chassis::convertPowerStateFromString(systemPowerState) !=
        Chassis::PowerState::Off)
    {
        log<level::ERR>(fmt::format("Manual hardware de-isolation is allowed "
                                    "only when chassis powerstate is off")
                            .c_str());
        return false;
    }
    return true;
}

void setEnabledProperty(sdbusplus::bus::bus& bus,
                        const std::string& dbusObjPath, bool enabledPropVal)
{
    /**
     * Make sure "Object::Enable" interface is implemented for the given
     * dbus object path and don't throw an exception if the interface or
     * property "Enabled" is not implemented since "Enabled" property
     * update requires only for few hardware which are going isolate from
     * external interface i.e Redfish
     */
    constexpr auto enabledPropIface = "xyz.openbmc_project.Object.Enable";

    // Using two try and catch block to avoid more trace for same issue
    // since using common utils API "setDBusPropertyVal"
    try
    {
        getDBusServiceName(bus, dbusObjPath, enabledPropIface);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        if (std::string(e.name()) ==
            std::string("xyz.openbmc_project.Common.Error.ResourceNotFound"))
        {
            return;
        }
        throw sdbusplus::exception::SdBusError(
            const_cast<sd_bus_error*>(e.get_error()), "HW-Isolation");
    }

    try
    {
        setDBusPropertyVal<bool>(bus, dbusObjPath, enabledPropIface, "Enabled",
                                 enabledPropVal);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        if (std::string(e.name()) ==
            std::string("org.freedesktop.DBus.Error.UnknownProperty"))
        {
            return;
        }
        throw sdbusplus::exception::SdBusError(
            const_cast<sd_bus_error*>(e.get_error()), "HW-Isolation");
    }
}

} // namespace utils
} // namespace hw_isolation
