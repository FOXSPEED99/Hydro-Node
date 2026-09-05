-- Migration record: applied to the live Supabase project (water-level /
-- aidpejxlofvdrtemurft) on 2026-07-13 as `hydro_hub_tank_config_ota_setup`.
-- Adds: tank configuration (spec §3.2 step 4) + setup-complete tracking,
-- the set_hydro_hub_tank_config RPC, OTA firmware releases (spec §9), and
-- locks app-facing RPCs down to service_role only.

-- ── Tank configuration + setup-complete tracking on the device row ──
alter table public.hydro_hub_devices
  add column if not exists tank_height_cm integer,
  add column if not exists blind_area_cm integer,
  add column if not exists tank_count integer not null default 1,
  add column if not exists tank_volumes_liters jsonb,
  add column if not exists tank_capacity_liters integer,
  add column if not exists tank_config_version integer not null default 0,
  add column if not exists setup_completed_at timestamptz;

-- ── OTA firmware releases (service-role managed; served to devices via device_sync) ──
create table if not exists public.hydro_hub_firmware_releases (
  version text primary key,
  firmware_url text not null,
  sha256 text,
  force_update boolean not null default false,
  is_active boolean not null default true,
  notes text,
  released_at timestamptz not null default timezone('utc', now())
);
alter table public.hydro_hub_firmware_releases enable row level security;

-- ── App-facing RPC: save tank configuration (Step 4 of setup) ──
create or replace function public.set_hydro_hub_tank_config(
  p_user_id text,
  p_device_id text,
  p_tank_height_cm integer,
  p_blind_area_cm integer,
  p_tank_volumes jsonb
) returns jsonb
language plpgsql
security definer
set search_path to 'public'
as $$
declare
  v_device_id text := public.normalize_hydro_hub_device_id(p_device_id);
  v_device public.hydro_hub_devices%rowtype;
  v_capacity numeric := 0;
  v_count integer := 0;
  v_vol numeric;
begin
  select * into v_device from public.hydro_hub_devices where device_id = v_device_id for update;
  if not found then raise exception 'Unknown device'; end if;

  -- Only a linked (paired) user may configure the tank.
  if not exists (
    select 1 from public.hydro_hub_links l
    where l.device_id = v_device.device_id
      and l.reset_generation = v_device.reset_generation
      and l.user_id = trim(coalesce(p_user_id, ''))
  ) then
    raise exception 'Device is not linked to this account';
  end if;

  if p_tank_height_cm is null or p_tank_height_cm < 10 or p_tank_height_cm > 2000 then
    raise exception 'Tank height must be between 10 and 2000 cm';
  end if;
  if p_blind_area_cm is null or p_blind_area_cm < 0 or p_blind_area_cm > 500 then
    raise exception 'Blind area must be between 0 and 500 cm';
  end if;
  if p_tank_volumes is null or jsonb_typeof(p_tank_volumes) <> 'array'
     or jsonb_array_length(p_tank_volumes) < 1 or jsonb_array_length(p_tank_volumes) > 10 then
    raise exception 'Tank volumes must be an array of 1..10 entries';
  end if;

  for v_vol in select (value)::numeric from jsonb_array_elements_text(p_tank_volumes) loop
    if v_vol is null or v_vol <= 0 or v_vol > 1000000 then
      raise exception 'Each tank volume must be a positive number of liters';
    end if;
    v_capacity := v_capacity + v_vol;
    v_count := v_count + 1;
  end loop;

  update public.hydro_hub_devices set
    tank_height_cm = p_tank_height_cm,
    blind_area_cm = p_blind_area_cm,
    tank_count = v_count,
    tank_volumes_liters = p_tank_volumes,
    tank_capacity_liters = round(v_capacity)::integer,
    tank_config_version = tank_config_version + 1,
    setup_completed_at = coalesce(setup_completed_at, timezone('utc', now()))
  where device_id = v_device.device_id
  returning * into v_device;

  return jsonb_build_object(
    'success', true,
    'device_id', v_device.device_id,
    'tank_config_version', v_device.tank_config_version,
    'tank_capacity_liters', v_device.tank_capacity_liters
  );
end;
$$;

revoke all on function public.set_hydro_hub_tank_config(text, text, integer, integer, jsonb) from public, anon, authenticated;
grant execute on function public.set_hydro_hub_tank_config(text, text, integer, integer, jsonb) to service_role;

-- ── device_sync_hydro_hub was extended to return: ──
--   setup_complete   boolean (paired + tank data saved)
--   tank_config      {version, height_cm, blind_cm, capacity_l, tank_count} | null
--   ota              {version, url, sha256, force} | null (latest active release
--                    whose version differs from the reported firmware)
-- (full body lives in the applied migration; see Supabase migration history)

-- ── list_user_hydro_hubs was extended to return: ──
--   tank_height_cm, blind_area_cm, tank_count, tank_volumes_liters,
--   tank_capacity_liters, setup_complete

-- ── Grants ── (see follow-up migration hydro_hub_regrant_app_rpcs_anon)
-- The app-facing RPCs (create_hydro_hub_pair_request, get_hydro_hub_pair_request_status,
-- set_hydro_hub_pump_state, unlink_hydro_hub_for_user, list_pairing_hydro_hubs,
-- list_user_hydro_hubs, set_hydro_hub_tank_config) were briefly made
-- service_role-only, but the tRPC server currently runs with the ANON key
-- (.env has no SUPABASE_SERVICE_ROLE_KEY), so anon execute was restored.
-- TODO before production: add SUPABASE_SERVICE_ROLE_KEY to the server .env,
-- restart the server, then revoke anon/PUBLIC execute on all of the above —
-- they take p_user_id as input, so anon execute lets anyone holding the anon
-- key act on another user's behalf.

-- ── Follow-up migration hydro_hub_wifi_ssid (2026-07-13) ──
-- hydro_hub_devices.wifi_ssid column; device_sync_hydro_hub gained an optional
-- p_wifi_ssid parameter (the old 14-arg overload was DROPPED to avoid PostgREST
-- ambiguity — firmware 1.1.0 still resolves against the 15-arg version via the
-- default). list_user_hydro_hubs returns wifi_ssid. Firmware >= 1.1.1 sends the
-- connected network name, shown in the app profile.

-- ── Follow-up migration hydro_hub_ota_offer_newest_only (2026-07-23) ──
-- Fixes an infinite update loop. The OTA lookup used to be:
--   select * from hydro_hub_firmware_releases
--    where is_active and version <> device.firmware_version
--    order by released_at desc limit 1;
-- Filtering the device's own version out INSIDE the query means that with two
-- active releases the hub is always handed "the other one": a hub on 1.2.3 was
-- offered 1.2.2, rebooted, was offered 1.2.3, rebooted, forever. It now selects
-- the newest active release unconditionally and only offers it when it differs
-- from the reported firmware, so an older release can never be pushed out.
-- Deactivating superseded releases is therefore optional, not required.

-- ── Sensor Node binding (2026-07-23) ──
-- Migrations: hydro_hub_sensor_node_link, hydro_hub_sensor_node_unlink_generation,
--             hydro_hub_device_sync_sensor_node, hydro_hub_sensor_node_expose_to_app
--
-- hydro_hub_devices gains: sensor_node_id, sensor_node_linked_at,
--   sensor_node_unlink_gen (integer, monotonic).
-- The hub locks onto one Sensor Node (node fw >= 1.1.0 sends a permanent
-- EEPROM id) and mirrors it here; packets from any other node on the same pair
-- id are dropped.
--
-- device_sync_hydro_hub gained p_sensor_node_id + p_sensor_node_unlink_gen and
-- returns unlink_sensor_node + sensor_node_unlink_gen. The 18-arg overload was
-- DROPPED (two overloads differing only by defaulted trailing args make
-- PostgREST ambiguous); older firmware resolves here via the defaults.
--
-- A GENERATION, not a boolean: unlink_hydro_hub_sensor_node increments the
-- counter and the hub reports back the highest one it has applied. A boolean
-- would loop — if the hub re-linked to the same node between applying the
-- unlink and its next sync, the server would see the same id and ask again
-- forever (the same failure mode as the OTA ping-pong above).
--
-- list_user_hydro_hubs returns sensor_node_id + sensor_node_linked_at (dropped
-- and recreated: RETURNS TABLE columns can't be added with CREATE OR REPLACE).
--
-- Grants: unlink_hydro_hub_sensor_node and list_user_hydro_hubs are
-- service_role ONLY, matching the other app RPCs. The tRPC server now holds
-- SUPABASE_SERVICE_ROLE_KEY, so the anon-grant workaround noted above is
-- retired — do not re-grant anon on anything taking p_user_id.

-- ── Publishing an OTA release ──
-- 1. Build the firmware (Sketch > Export compiled binary) -> Hydro-Hub.ino.bin
-- 2. Upload the .bin somewhere HTTPS-reachable (e.g. a public Supabase Storage bucket).
-- 3. insert into public.hydro_hub_firmware_releases (version, firmware_url, notes)
--    values ('1.2.0-hydrohub', 'https://.../Hydro-Hub-1.2.0.bin', 'what changed');
-- Every online hub picks it up on its next sync and reboots into it.
-- Set is_active=false to pull a release.
