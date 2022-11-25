#include "Device.h"

namespace app
{
    Device::Flags Device::GetDefaultFlags()
    {
        Flags flags;
        flags["flag1"] = {};
        flags["flag2"] = {};
        flags["flag3"] = {};
        return flags;
    }

    void Device::CalcDynamicFlags(size_t threshold)
    {
        acceptanceResult = std::all_of(flags.begin(), flags.end(), [threshold](const auto& f)
        {
            const Flag& flag = f.second;
            return flag.value && flag.changeTimestamp < threshold;
        });
    }

    void to_json(json& j, const Device& device)
    {
        j = json{
            {"id",               device.name},
            {"acceptanceResult", device.acceptanceResult},
        };
        for (auto& it : device.flags)
        {
            auto& flagName = it.first;
            auto& flag = it.second;
            json jsonFlag = json{
                {"name",            flagName},
                {"value",           flag.value},
                {"changeTimestamp", flag.changeTimestamp}
            };

            j["flags"].push_back(jsonFlag);
        }
    }
    
    void from_json(const json& j, Device& device)
    {
        j.at("id").get_to(device.name);
        j.at("acceptanceResult").get_to(device.acceptanceResult);
        auto& flags = j.at("flags");
        for (auto& jsonFlag : flags)
        {
            auto flagName = jsonFlag["name"].get<std::string>();
            auto& flag = device.flags[flagName];
            jsonFlag.at("value").get_to(flag.value);
            jsonFlag.at("changeTimestamp").get_to(flag.changeTimestamp);
        }
    }
} // namespace app