#include <phosphor-logging/lg2.hpp>
#include <poweron_time.hpp>

int main(int /*argc*/, char** /*argv*/)
{
    try
    {
        lg2::info("faultlog poweron time - write poweron time to file");
        auto bus = sdbusplus::bus::new_default();
        (void)openpower::faultlog::writePowerOnTime(bus);
    }
    catch (const std::exception& e)
    {
        lg2::error("Failed {ERROR}", "ERROR", e.what());
    }
    return 0;
}
