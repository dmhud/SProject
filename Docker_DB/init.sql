CREATE TABLE IF NOT EXISTS public.devices
(
    device_id bigint PRIMARY KEY NOT NULL GENERATED ALWAYS AS IDENTITY ( INCREMENT 1 START 1 MINVALUE 1 MAXVALUE 9223372036854775807 CACHE 1 ),
    device_name text COLLATE pg_catalog."default" NOT NULL,
    CONSTRAINT name_unique UNIQUE (device_name)
);
    
CREATE TABLE IF NOT EXISTS public.flags
(
    flag_id bigint NOT NULL GENERATED ALWAYS AS IDENTITY ( INCREMENT 1 START 1 MINVALUE 1 MAXVALUE 9223372036854775807 CACHE 1 ),
    device_id bigint NOT NULL,
    flag_name text COLLATE pg_catalog."default" NOT NULL,
    flag_value boolean NOT NULL,
    flag_timestamp BIGINT NOT NULL
);
    
GRANT ALL ON devices TO app_user;
GRANT ALL ON flags TO app_user;