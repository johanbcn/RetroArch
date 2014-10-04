/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2011-2014 - Daniel De Matteis
 *  Copyright (C) 2013-2014 - Jason Fetters
 * 
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __APPLE_RARCH_INPUT_H__
#define __APPLE_RARCH_INPUT_H__

#include "../general.h"

/* Input responder */
#define MAX_TOUCHES 16
#define MAX_KEYS 256

#ifndef NUM_HATS
#define NUM_HATS 4
#endif

typedef struct
{
   int16_t screen_x, screen_y;
   int16_t fixed_x, fixed_y;
   int16_t full_x, full_y;
} apple_touch_data_t;

typedef struct
{
   apple_touch_data_t touches[MAX_TOUCHES];
   uint32_t touch_count;

   uint32_t mouse_buttons;
   int16_t mouse_delta[2];

   uint32_t keys[MAX_KEYS];

   uint32_t buttons[MAX_PLAYERS];
   int16_t axes[MAX_PLAYERS][4];
   int8_t  hats[NUM_HATS][2];
    
   const rarch_joypad_driver_t *joypad;
} apple_input_data_t;

struct pad_connection;

struct pad_connection_interface
{
   void* (*connect)(void *data, uint32_t slot);

   void (*disconnect)(void* device);

   void (*packet_handler)(void* device, uint8_t *packet, uint16_t size);

   void (*set_rumble)(void* device, enum retro_rumble_effect effect,
         uint16_t strength);
};

/* Joypad data */
int32_t joypad_connection_connect(const char* name, void *data);

int32_t apple_joypad_connect_gcapi(void);

void pad_connection_disconnect(uint32_t slot);

void pad_connection_packet(uint32_t slot, uint8_t* data, uint32_t length);

/* Determine if connected joypad is a hidpad backed device.
 * If false, pad_connection_packet cannot be used */
bool apple_joypad_has_interface(uint32_t slot);

void apple_input_enable_icade(bool on);

void apple_input_enable_small_keyboard(bool on);

uint32_t apple_input_get_icade_buttons(void);

void apple_input_reset_icade_buttons(void);

void apple_input_keyboard_event(bool down, unsigned code,
      uint32_t character, uint32_t mod);

extern int32_t apple_input_find_any_key(void);

extern int32_t apple_input_find_any_button(uint32_t port);

extern int32_t apple_input_find_any_axis(uint32_t port);

#endif
