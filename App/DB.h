#pragma once

#include <set>

#include <soci/session.h>

#include "AppOptions.h"
#include "Device.h"

namespace app
{
    class DB
    {
    public:
        DB();
        ~DB();

        void Connect(const DBConnectionParams& p);

        void CreateDevice(Device device);
        Device ReadDevice(std::string deviceName);
        std::set<std::string> ReadDevicesNames() const;
        void UpdateDeviceFlags(Device device);
        void DeleteDevice(const std::string& deviceName) const;


        void TestSelectMultipleRows();
    private:
        std::unique_ptr<soci::connection_pool> _pool;
    };
} // namespace app