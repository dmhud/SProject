#include "DB.h"

#include <iostream>

#include <fmt/format.h>

#include <soci/soci.h>
#include <soci/postgresql/soci-postgresql.h>

namespace app
{
    struct DB_Device
    {
        size_t      device_id;
        std::string device_name;
    };

    struct DB_Flag
    {
        size_t      flag_id;
        size_t      device_id;
        std::string flag_name;
        bool        flag_value;
        size_t      flag_timestamp;
    };
}

// ORM specializations
namespace soci
{
    using namespace app;

    template<>
    struct type_conversion<DB_Device>
    {
        typedef values base_type;

        static void from_base(values const& v, indicator /* ind */, DB_Device& dev)
        {
            dev.device_id      = v.get<long long>  ("device_id");
            dev.device_name    = v.get<std::string>("device_name");
        }

        static void to_base(const DB_Device& dev, values& v, indicator& ind)
        {
            v.set("device_id",       static_cast<long long>(dev.device_id));
            v.set("device_name",                            dev.device_name);
            ind = i_ok;
        }
    };

    template<>
    struct type_conversion<DB_Flag>
    {
        typedef values base_type;

        static void from_base(values const& v, indicator /* ind */, DB_Flag& flag)
        {
            flag.flag_id        = v.get<long long>  ("flag_id");
            flag.device_id      = v.get<long long>  ("device_id");
            flag.flag_name      = v.get<std::string>("flag_name");
            flag.flag_value     = v.get<int>        ("flag_value");
            flag.flag_timestamp = v.get<long long>  ("flag_timestamp");
        }

        static void to_base(const DB_Flag& flag, values& v, indicator& ind)
        {
            v.set("flag_id",         static_cast<long long>(flag.flag_id));
            v.set("device_id",       static_cast<long long>(flag.device_id));
            v.set("flag_name",                              flag.flag_name);
            v.set("flag_value",      static_cast<int>      (flag.flag_value));
            v.set("flag_timestamp",  static_cast<long long>(flag.flag_timestamp));
            ind = i_ok;
        }
    };
}

namespace app
{
    using namespace soci;

    DB::DB() = default;
    DB::~DB() = default;

    void DB::Connect(const DBConnectionParams& p)
    {
        std::string connectString = fmt::format(
            "dbname={} user={} password={} host={} port={} connect_timeout={} application_name='app'",
            p.dbname,
            p.user,
            p.password,
            p.host,
            p.port,
            p.timeout);

        size_t poolSize = std::max<size_t>(p.pool_size, 1);
        _pool = std::make_unique<connection_pool>(poolSize);
        for (size_t i = 0; i != poolSize; ++i)
        {
            session & sql = _pool->at(i);
            sql.open(postgresql, connectString);

            if (!sql.is_connected())
                throw std::runtime_error("DB connect: FAILED");
        }
        std::cout << "DB connect: OK" << std::endl;
    }

    void DB::CreateDevice(Device device)
    {
        session sql(*_pool);

        try
        {
            transaction tr(sql);

            long long deviceId;
            sql << "INSERT INTO devices(device_name) "
                   "            values(:device_name) "
                   "ON CONFLICT(device_name) "
                   "    DO UPDATE SET device_name = EXCLUDED.device_name "
                   "RETURNING device_id",
                use(device.name), into(deviceId);

            for (const auto& it : device.flags)
            {
                auto& flagName = it.first;
                auto& flag = it.second;

                DB_Flag dbFlag;
                dbFlag.device_id      = deviceId;
                dbFlag.flag_name      = flagName;
                dbFlag.flag_value     = flag.value;
                dbFlag.flag_timestamp = flag.timestamp;
                sql << "INSERT INTO flags(device_id,  flag_name,  flag_value,  flag_timestamp) "
                       "SELECT           :device_id, :flag_name, :flag_value, :flag_timestamp "
                       "WHERE NOT EXISTS ( "
                       "	SELECT * FROM flags "
                       "		WHERE device_id = :device_id "
                       "		  AND flag_name = :flag_name "
                       ")",
                        use(dbFlag);
            }

            tr.commit();
        }
        catch (std::exception& ex)
        {
            std::cout << ex.what() << std::endl;
        }
    }

    Device DB::ReadDevice(std::string deviceName)
    {
        session sql(*_pool);
        
        Device device;

        try
        {
            transaction tr(sql);

            DB_Device dev;
            sql << "SELECT * FROM devices WHERE device_name = :name", use(deviceName), into(dev);
            device.name = dev.device_name;

            rowset<DB_Flag> rs = (sql.prepare << "SELECT * FROM flags WHERE device_id = :devId", use(dev.device_id));
            for (auto dbFlag = rs.begin(); dbFlag != rs.end(); ++dbFlag)
            {
                auto& flag = device.flags[dbFlag->flag_name];
                flag.value     = dbFlag->flag_value;
                flag.timestamp = dbFlag->flag_timestamp;
            }

            tr.commit();
        }
        catch(std::exception& ex)
        {
            std::cout << ex.what() << std::endl;
            return {};
        }
        return device;
    }
    
    std::set<std::string> DB::ReadDevicesNames() const
    {
        session sql(*_pool);
        rowset<std::string> rs = (sql.prepare << "SELECT device_name FROM devices");
        std::set<std::string> names;
        std::copy(rs.begin(), rs.end(), std::inserter(names, names.end()));
        return names;
    }

    void DB::UpdateDeviceFlags(Device device)
    {
        session sql(*_pool);

        try
        {
            transaction tr(sql);
            
            DB_Device dbDev;
            sql << "SELECT * FROM devices WHERE device_name = :name", use(device.name), into(dbDev);

            for (const auto& it : device.flags)
            {
                auto& flagName = it.first;
                auto& flag = it.second;

                DB_Flag dbFlag;
                dbFlag.device_id = dbDev.device_id;
                dbFlag.flag_name = flagName;
                dbFlag.flag_value = flag.value;
                dbFlag.flag_timestamp = flag.timestamp;
                
                sql << "UPDATE flags "
                       "SET flag_value     = :flag_value, "
                       "    flag_timestamp = :flag_timestamp "
                       "WHERE device_id = :device_id "
                       "  AND flag_name = :flag_name;",
                    use(dbFlag);
            }

            tr.commit();
        }
        catch (std::exception& ex)
        {
            std::cout << ex.what() << std::endl;
        }
    }

    void DB::DeleteDevice(const std::string& deviceName) const
    {      
        session sql(*_pool);
        sql << "DELETE FROM devices WHERE device_name = :name;", use(deviceName);
    }

    void DB::TestSelectMultipleRows()
    {
        session sql(*_pool);

        rowset<DB_Flag> rs = (sql.prepare << "SELECT * FROM flags");
        for (auto flag = rs.begin(); flag != rs.end(); ++flag)
        {
            auto str = fmt::format("flag_id: {}, device_id: {}, flag_name: {}, flag_value: {}, flag_timestamp: {}",
                flag->flag_id,
                flag->device_id,
                flag->flag_name,
                flag->flag_value,
                flag->flag_timestamp);
            std::cout << str << std::endl;
        }
    }
} // namespace app