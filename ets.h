// ETS2 LCD Dashboard for ESP32C3
//
// Copyright (C) 2023 Ding Zhaojie <zhaojie_ding@msn.com>
//
// This work is licensed under the terms of the GNU GPL, version 2 or later.
// See the COPYING file in the top-level directory.

#ifndef __ETS_H__
#define __ETS_H__

typedef struct {
  int eta_dist;
  int eta_time;  // minutes
  int speed;
  int cruise;  // 0 for off
  int limit;   // 0 for no limit
  int fuel;    // percentage
  int fuel_dist;
} EtsState;

#endif
