#include "xyz/openbmc_project/Logging/Entry/server.hpp"

#include <phosphor-logging/lg2.hpp>
#include <poweron_time.hpp>

#include <chrono>
#include <fstream>

namespace openpower::faultlog
{
constexpr auto poweronTimeFile =
    "/var/lib/op-hw-isolation/persistdata/powerontime";
using namespace std::chrono;
using Severity = sdbusplus::xyz::openbmc_project::Logging::server::Entry::Level;

std::string epochTimeToBCD(uint64_t milliSeconds)
{
    auto decimalToBCD = [](int value) {
        std::stringstream ss;
        ss << std::setw(2) << std::setfill('0') << value;
        return ss.str();
    };
    std::chrono::milliseconds ms{milliSeconds};
    std::chrono::time_point<std::chrono::system_clock> time{ms};

    time_t t = std::chrono::system_clock::to_time_t(time);
    tm* timeInfo = localtime(&t);

    int year = 1900 + timeInfo->tm_year;
    std::string bcdYear = decimalToBCD(year / 100);
    bcdYear += decimalToBCD(year % 100);
    std::string bcdMonth = decimalToBCD(timeInfo->tm_mon + 1); // Month (1-12)
    std::string bcdDay = decimalToBCD(timeInfo->tm_mday);  // Day of the month
    std::string bcdHour = decimalToBCD(timeInfo->tm_hour); // Hour (0-23)
    std::string bcdMin = decimalToBCD(timeInfo->tm_min);   // Minutes
    std::string bcdSec = decimalToBCD(timeInfo->tm_sec);   // Seconds

    // Construct the BCD time string
    //"DATE_TIME": "04/11/2023 09:39:15",
    std::string bcdTime = bcdMonth + "/" + bcdDay + "/" + bcdYear + " " +
                          bcdHour + ":" + bcdMin + ":" + bcdSec;
    return bcdTime;
}

void writePowerOnTime(sdbusplus::bus::bus& bus)
{
    try
    {
        uint64_t value =
            duration_cast<milliseconds>(system_clock::now().time_since_epoch())
                .count();

        std::ofstream file(poweronTimeFile, std::ios::binary);
        file.exceptions(std::ofstream::failbit | std::ofstream::badbit);

        file.write(reinterpret_cast<const char*>(&value), sizeof(value));

        lg2::info("Latest chassis poweron time  written is :{TIME}", "TIME",
                  epochTimeToBCD(value));
        file.close();
    }
    catch (const std::exception& ex)
    {
        lg2::error("Failed to write poweron time to file {FILE}", "FILE",
                   poweronTimeFile);
        std::unordered_map<std::string, std::string> data = {
            {"FILE_PATH", poweronTimeFile}};

        auto method = bus.new_method_call(
            "xyz.openbmc_project.Logging", "/xyz/openbmc_project/logging",
            "xyz.openbmc_project.Logging.Create", "Create");
        method.append("org.open_power.Faultlog.PoweronTime.WriteFail",
                      Severity::Informational, data);
        auto reply = method.call();
        if (reply.is_method_error())
        {
            lg2::error("Error in calling D-Bus method to create PEL");
        }
    }
}

uint64_t readPowerOnTime(sdbusplus::bus::bus& bus)
{
    uint64_t powerOnTime = 0;
    try
    {
        // Open the file for reading
        std::ifstream file(poweronTimeFile, std::ios::binary);
        file.exceptions(std::ofstream::failbit | std::ofstream::badbit);

        file.read(reinterpret_cast<char*>(&powerOnTime), sizeof(powerOnTime));

        lg2::info("Latest chassis poweron time read is :{TIME}", "TIME",
                  epochTimeToBCD(powerOnTime));
        file.close();
    }
    catch (const std::exception& ex)
    {
        lg2::error("Failed to read poweron time from file {FILE}", "FILE",
                   poweronTimeFile);
        std::unordered_map<std::string, std::string> data = {
            {"FILE_PATH", poweronTimeFile}};

        auto method = bus.new_method_call(
            "xyz.openbmc_project.Logging", "/xyz/openbmc_project/logging",
            "xyz.openbmc_project.Logging.Create", "Create");
        method.append("org.open_power.Faultlog.PoweronTime.ReadFail",
                      Severity::Informational, data);
        auto reply = method.call();
        if (reply.is_method_error())
        {
            lg2::error("Error in calling D-Bus method to create PEL");
        }
    }
    return powerOnTime;
}

} // namespace openpower::faultlog
