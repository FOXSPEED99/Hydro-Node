create table if not exists public.devices (
  device_id text primary key,
  pair_id text not null,
  firmware_version text,
  pump_on boolean not null default false,
  desired_pump_on boolean,
  wifi_rssi integer,
  updated_at timestamptz not null default now()
);

create table if not exists public.water_telemetry (
  id bigint generated always as identity primary key,
  device_id text not null,
  pair_id text not null,
  firmware_version text,
  sequence bigint,
  level_percent integer check (level_percent >= 0 and level_percent <= 100),
  volume_liters numeric,
  filling boolean,
  flow_lpm numeric,
  eta_seconds integer,
  pump_on boolean,
  rssi numeric,
  snr numeric,
  packet_count bigint,
  created_at timestamptz not null default now()
);

create index if not exists water_telemetry_device_created_idx
  on public.water_telemetry (device_id, created_at desc);

create or replace function public.set_updated_at()
returns trigger
language plpgsql
as $$
begin
  new.updated_at = now();
  return new;
end;
$$;

drop trigger if exists devices_set_updated_at on public.devices;
create trigger devices_set_updated_at
before update on public.devices
for each row execute function public.set_updated_at();

alter table public.devices enable row level security;
alter table public.water_telemetry enable row level security;

-- Production note:
-- Add app-specific RLS policies before shipping. During early bench testing,
-- you may temporarily create restricted policies for your authenticated app user
-- or service endpoint, but do not expose service-role keys in device firmware.
