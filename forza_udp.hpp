// ETS2 LCD Dashboard for ESP8266/ESP32C3
//
// Copyright (C) 2026 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.
//
// Refer: https://support.forzamotorsport.net/hc/en-us/articles/21742934024211-Forza-Motorsport-Data-Out-Documentation
// Refer: https://github.com/geeooff/forza-data-web

#ifndef __FORZA_UDP_H__
#define __FORZA_UDP_H__

#include <cstdint>

constexpr const uint16_t FORZA_PORT = 8888;
constexpr int FORZA_PPS = 40;  // 60 packets per second, slow down to 40fps.

#define PACKED __attribute__((packed))

struct PACKED ForzaSledData {
  // = 1 when race is on. = 0 when in menus/race stopped
  int32_t IsRaceOn;

  // Can overflow to 0 eventually
  uint32_t TimestampMS;

  float EngineMaxRpm;
  float EngineIdleRpm;
  float CurrentEngineRpm;

  // In the car's local space; X = right, Y = up, Z = forward
  float AccelerationX;
  float AccelerationY;
  float AccelerationZ;

  // In the car's local space; X = right, Y = up, Z = forward
  float VelocityX;
  float VelocityY;
  float VelocityZ;

  // In the car's local space; X = pitch, Y = yaw, Z = roll
  float AngularVelocityX;
  float AngularVelocityY;
  float AngularVelocityZ;

  float Yaw;
  float Pitch;
  float Roll;

  // Suspension travel normalized: 0.0f = max stretch; 1.0 = max compression
  float NormalizedSuspensionTravelFrontLeft;
  float NormalizedSuspensionTravelFrontRight;
  float NormalizedSuspensionTravelRearLeft;
  float NormalizedSuspensionTravelRearRight;

  // Tire normalized slip ratio, = 0 means 100% grip and |ratio| > 1.0 means loss of grip.
  float TireSlipRatioFrontLeft;
  float TireSlipRatioFrontRight;
  float TireSlipRatioRearLeft;
  float TireSlipRatioRearRight;

  // Wheel rotation speed radians/sec.
  float WheelRotationSpeedFrontLeft;
  float WheelRotationSpeedFrontRight;
  float WheelRotationSpeedRearLeft;
  float WheelRotationSpeedRearRight;

  // = 1 when wheel is on rumble strip, = 0 when off.
  int32_t WheelOnRumbleStripFrontLeft;
  int32_t WheelOnRumbleStripFrontRight;
  int32_t WheelOnRumbleStripRearLeft;
  int32_t WheelOnRumbleStripRearRight;

  // = from 0 to 1, where 1 is the deepest puddle
  float WheelInPuddleDepthFrontLeft;
  float WheelInPuddleDepthFrontRight;
  float WheelInPuddleDepthRearLeft;
  float WheelInPuddleDepthRearRight;

  // Non-dimensional surface rumble values passed to controller force feedback
  float SurfaceRumbleFrontLeft;
  float SurfaceRumbleFrontRight;
  float SurfaceRumbleRearLeft;
  float SurfaceRumbleRearRight;

  // Tire normalized slip angle, = 0 means 100% grip and |angle| > 1.0 means loss of grip.
  float TireSlipAngleFrontLeft;
  float TireSlipAngleFrontRight;
  float TireSlipAngleRearLeft;
  float TireSlipAngleRearRight;

  // Tire normalized combined slip, = 0 means 100% grip and |slip| > 1.0 means loss of grip.
  float TireCombinedSlipFrontLeft;
  float TireCombinedSlipFrontRight;
  float TireCombinedSlipRearLeft;
  float TireCombinedSlipRearRight;

  // Actual suspension travel in meters
  float SuspensionTravelMetersFrontLeft;
  float SuspensionTravelMetersFrontRight;
  float SuspensionTravelMetersRearLeft;
  float SuspensionTravelMetersRearRight;

  // Unique ID of the car make/model
  int32_t CarOrdinal;

  // Between 0 (D -- worst cars) and 7 (X class -- best cars) inclusive
  int32_t CarClass;

  // Between 100 (slowest car) and 999 (fastest car) inclusive
  int32_t CarPerformanceIndex;

  // Corresponds to EDrivetrainType; 0 = FWD, 1 = RWD, 2 = AWD
  int32_t DrivetrainType;

  // Number of cylinders in the engine
  int32_t NumCylinders;
};
static_assert(sizeof(ForzaSledData) == 232, "Unexpected Sled size!");

struct PACKED ForzaDashData {
  // Position (meters)
  float PositionX;
  float PositionY;
  float PositionZ;

  float Speed;   // meters per second
  float Power;   // watts
  float Torque;  // newton meter

  float TireTempFrontLeft;
  float TireTempFrontRight;
  float TireTempRearLeft;
  float TireTempRearRight;

  float Boost;
  float Fuel;
  float DistanceTraveled;
  float BestLap;
  float LastLap;
  float CurrentLap;
  float CurrentRaceTime;

  ushort LapNumber;
  uint8_t RacePosition;

  uint8_t Accel;
  uint8_t Brake;
  uint8_t Clutch;
  uint8_t HandBrake;
  uint8_t Gear;
  int8_t Steer;

  int8_t NormalizedDrivingLine;
  int8_t NormalizedAIBrakeDifference;
};
static_assert(sizeof(ForzaDashData) == 79, "Unexpected Dash size!");

struct PACKED ForzaHorizonExtData {
  int32_t CarCategory;
  float ImpactX;
  float ImpactY;
};
static_assert(sizeof(ForzaHorizonExtData) == 12, "Unexpected Horizon Ext size!");

struct PACKED ForzaMotorsportExtData {
  float TireWearFrontLeft;
  float TireWearFrontRight;
  float TireWearRearLeft;
  float TireWearRearRight;

  // ID for track
  int32_t TrackOrdinal;
};
static_assert(sizeof(ForzaMotorsportExtData) == 20, "Unexpected Motorsport Ext size!");

// Forza Motorsport 7
struct PACKED ForzaMotorsportPktV1 {
  ForzaSledData sled;
  ForzaDashData dash;
};
static_assert(sizeof(ForzaMotorsportPktV1) == 311, "Unexpected Motorsport 7 packet size!");

// Forza Motorsport 2023
struct PACKED ForzaMotorsportPktV2 {
  ForzaSledData sled;
  ForzaDashData dash;
  ForzaMotorsportExtData ext;
};
static_assert(sizeof(ForzaMotorsportPktV2) == 331, "Unexpected Motorsport 2k3 packet size!");

// Froza Horizon 4/5
struct PACKED ForzaHorizonPkt {
  ForzaSledData sled;
  ForzaHorizonExtData ext;
  ForzaDashData dash;
  uint8_t padding1;
};
static_assert(sizeof(ForzaHorizonPkt) == 324, "Unexpected Horizon packet size!");

union PACKED ForzaPkt {
  ForzaMotorsportPktV1 motosportV1;
  ForzaMotorsportPktV2 motosportV2;
  ForzaHorizonPkt horizon;
  uint8_t bytes[0];
};

static inline bool IsForzaPacket(size_t len) {
  return (len == sizeof(ForzaMotorsportPktV1) || len == sizeof(ForzaMotorsportPktV2) || len == sizeof(ForzaHorizonPkt));
}

#endif
