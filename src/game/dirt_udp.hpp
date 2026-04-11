// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.
//
// Refer:
// - https://www.eksimracing.org/f-a-q/how-to-get-codemasters-f1-20xx-dirt2-and-dirt3-working-with-latest-software/
// - https://www.eksimracing.org/review-post/codemasters-grid-2019-plugin/
// - https://www.scribd.com/document/350826037/UDP-output-setup
// - https://github.com/ErlerPhilipp/dr2_logger
// - https://github.com/MavoSV8/DirtTelemetryTool
// - https://github.com/ozkar99/cm-telemetry
//
// Codemasters UDP API mappings:
//  1: DiRT 2, DiRT 3, F1 10, F1 11
//  2: F1 12
//  3: F1 13, F1 14, DiRT Rally, DiRT Rally 2.0, DiRT 4, GRID 2, GRID 19
//  4: F1 15, F1 16
//  5: F1 17
//  6: F1 18
//  7: F1 19
//  8: F1 20
//  9: F1 21
// 10: F1 22
// 11: F1 23
// 12: F1 24
// 13: F1 25

#pragma once

#include "../utils.hpp"

constexpr uint16_t DIRT_PORT = 20777;  // max 100Hz

struct PACKED CodemastersAPIv3 {
  float total_time;      // the amount of time since the race session begun, in seconds
  float lap_time;        // current lap time, in seconds
  float lap_distance;    // the distance of the track, in metre
  float total_distance;  // the race distance, in metres
  float position_x;
  float position_y;
  float position_z;
  float speed;  // vehicles linear velocity, in m/s
  float velocity_x;
  float velocity_y;
  float velocity_z;
  float left_dir_x;
  float left_dir_y;
  float left_dir_z;
  float forward_dir_x;
  float forward_dir_y;
  float forward_dir_z;
  float suspension_position_bl;
  float suspension_position_br;
  float suspension_position_fl;
  float suspension_position_fr;
  float suspension_velocity_bl;
  float suspension_velocity_br;
  float suspension_velocity_fl;
  float suspension_velocity_fr;
  float wheel_patch_speed_bl;
  float wheel_patch_speed_br;
  float wheel_patch_speed_fl;
  float wheel_patch_speed_fr;
  float throttle_input;
  float steering_input;
  float brake_input;
  float clutch_input;
  float gear;  // 0 for neutral, -1 for reverse.
  float gforce_lateral;
  float gforce_longitudinal;
  float lap;          // current lap number.
  float engine_rate;  // engine rotation rate in radians/second (rpm/10)
  float native_sli_support;
  float race_position;  // current position within the race
  float kers_level;
  float kers_level_max;
  float drs;
  float traction_control;
  float abs;
  float fuel_in_tank;
  float fuel_capacity;
  float in_pits;
  float race_sector;  // the number of splits in the lap
  float sector_time_1;
  float sector_time_2;
  float brake_temp_bl;
  float brake_temp_br;
  float brake_temp_fl;
  float brake_temp_fr;
  float tyre_pressure_bl;
  float tyre_pressure_br;
  float tyre_pressure_fl;
  float tyre_pressure_fr;
  float laps_completed;
  float total_laps;
  float track_length;   // the length of the track, in metres
  float last_lap_time;  // the time of the last lap, in seconds
  float max_rpm;        // the maximum engine rate, in radians/second (rpm/10)
  float idle_rpm;       // the idle rate of the engine in radians/second (rpm/10)
  float max_gears;      // the number of forward gears available in the vehicle
};
static_assert(sizeof(CodemastersAPIv3) == 264, "Unexpected DiRT Rally packet size!");

union PACKED DirtPkt {
  CodemastersAPIv3 apiV3;
  uint8_t bytes[0];
};

static inline bool IsDirtPacket(size_t len) {
  return (len == sizeof(CodemastersAPIv3));
}
