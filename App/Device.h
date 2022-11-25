#pragma once

#include <unordered_map>
#include <string>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace app
{
    struct Device
    {
        struct Flag
        {
            bool        value = false;
            uint64_t    timestamp = 0;
            uint64_t    changeTimestamp = 0;
        };
        using Flags = std::unordered_map<std::string, Flag>;

        std::string name;
        bool        acceptanceResult = false;
        Flags       flags = GetDefaultFlags();

        static Flags GetDefaultFlags();
        void CalcDynamicFlags(size_t threshold);
    };

    void to_json(json& j, const Device& device);
    void from_json(const json& j, Device& device);

} // namespace app