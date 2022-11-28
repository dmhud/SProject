# SProject: use docker-compose

## Install
```
apt update && apt upgrade
apt install git docker docker-compose
```

## Clone
```
git clone https://github.com/dmhud/SProject.git
cd SProject
```

## Build and Run
### on server (ubuntu 22.04)
```
docker-compose -f docker-compose.yml up
```

# SProject: manual build and run

## Install soft
```
apt update && apt upgrade
apt install git cmake pip 
pip install conan

apt install postgresql postgresql-contrib
systemctl start postgresql.service
systemctl status postgresql.service
psql -V

apt install mosquitto mosquitto-clients
systemctl status mosquitto

```

## Clone
```
git clone git@github.com:dmhud/SProject.git
cd SProject
```

## Build and Run
### on desktop (Win10)
```
mkdir _build && cd _build
conan install .. --profile ../conan_profile_win.txt --build=missing
cmake .. -G "Visual Studio 15 2017" -A x64 -DCMAKE_CXX_FLAGS="/W4 /EHsc" -DCMAKE_MODULE_PATH=%cd% 
cmake --build . --config Release
bin\App.exe --app_port=54545 --app_worker_count=4 --app_change_timestamp_threshold=259200 --db_name="app_postgres" --db_user="app_user" --db_password="app_password" --db_host="127.0.0.1" --db_port=5432 --db_timeout=10 --db_pool_size=10 --mqtt_host="127.0.0.1" --mqtt_port=1883 --mqtt_timeout=60 --mqtt_topic="+/out/data"
```
### on server (ubuntu 22.04)
```
mkdir _build && cd _build
conan install .. --profile ../conan_profile_linux.txt --build=missing
cmake .. -DCMAKE_MODULE_PATH="$PWD"
cmake --build . --config Release
./bin/App --app_port=54545 --app_worker_count=4 --app_change_timestamp_threshold=259200 --db_name="app_postgres" --db_user="app_user" --db_password="app_password" --db_host="127.0.0.1" --db_port=5432 --db_timeout=10 --db_pool_size=10 --mqtt_host="127.0.0.1" --mqtt_port=1883 --mqtt_timeout=60 --mqtt_topic="+/out/data"
```

## Create database
```
sudo -u postgres psql postgres
    CREATE DATABASE app_postgres;
    \q
    
sudo -u postgres psql app_postgres
    create user app_user with encrypted password 'app_password';
    grant all privileges on database app_postgres to app_user;
    \q
    
sudo -u postgres psql app_postgres
    CREATE TABLE IF NOT EXISTS devices
    (
        device_id bigint PRIMARY KEY NOT NULL GENERATED ALWAYS AS IDENTITY ( INCREMENT 1 START 1 MINVALUE 1 MAXVALUE 9223372036854775807 CACHE 1 ),
        device_name text COLLATE pg_catalog."default" NOT NULL,
        CONSTRAINT name_unique UNIQUE (device_name)
    );
    
    CREATE TABLE IF NOT EXISTS flags
    (
        flag_id bigint NOT NULL GENERATED ALWAYS AS IDENTITY ( INCREMENT 1 START 1 MINVALUE 1 MAXVALUE 9223372036854775807 CACHE 1 ),
        device_id bigint NOT NULL,
        flag_name text COLLATE pg_catalog."default" NOT NULL,
        flag_value boolean NOT NULL,
        flag_timestamp BIGINT NOT NULL
    );
    
    GRANT ALL ON devices TO app_user;
    GRANT ALL ON flags TO app_user;
    \q
```

## Show net connections
```
netstat -plunt |grep postgres
netstat -plunt |grep mosquitto
```

## Test MQTT-messages
### Edit MQTT-server settings (TODO need access by password?)
```
cp /etc/mosquitto/mosquitto.conf my_mosquitto.conf
nano my_mosquitto.conf
    listener 1883
    allow_anonymous true
systemctl edit mosquitto
    [Service]
    ExecStart=
    ExecStart=/usr/sbin/mosquitto -c /root/my_mosquitto.conf
systemctl restart mosquitto
systemctl status mosquitto
```
### Subscribe
```
mosquitto_sub -h localhost -t +/out/data
```
### Publish
```
mosquitto_pub -h localhost -p 1884 -t dev0/out/data -m '
[{"dev_eui":"dev0","param_id":1, "value":0},
{"dev_eui":"dev0","param_id":1,  "value":1},
{"dev_eui":"dev0","param_id":2,  "value":12}
]
'
```

## Test REST methods
### CURL from desktop (Win10)
Change <SERVER_IP>  
POST (Create devices):
```
curl -i -X POST http://<SERVER_IP>:54545/devices/dev0 -H "Accept: application/json"

cd SProject\Test\
curl -i -X POST http://<SERVER_IP>:54545/devices -H "Accept: application/json" -H "Content-Type: application/json" -d @devicesIds.json
```
GET (Get devices):
```
curl -i -X GET http://<SERVER_IP>:54545/devices -H "Accept: application/json"
curl -i -X GET http://<SERVER_IP>:54545/devices/ -H "Accept: application/json"
curl -i -X GET http://<SERVER_IP>:54545/devices/dev0 -H "Accept: application/json"
```
DELETE (Delete devices):
```
curl -i -X DELETE http://<SERVER_IP>:54545/devices -H "Accept: application/json"
curl -i -X DELETE http://<SERVER_IP>:54545/devices/ -H "Accept: application/json"
curl -i -X DELETE http://<SERVER_IP>:54545/devices/dev0 -H "Accept: application/json"
```
