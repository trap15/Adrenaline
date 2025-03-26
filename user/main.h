/*
  Adrenaline
  Copyright (C) 2016-2018, TheFloW

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __MAIN_H__
#define __MAIN_H__

#include <psp2/compat.h>
#include <psp2/ctrl.h>
#include <psp2/io/stat.h>
#include <taihen.h>
#include "../adrenaline_compat.h"

#define INCLUDE_EXTERN_RESOURCE(name) extern unsigned char _binary_flash0_##name##_start; extern unsigned char _binary_flash0_##name##_size;

#define ALIGN(x, align) (((x) + ((align) - 1)) & ~((align) - 1))

#define MAX_PATH_LENGTH 1024
#define MAX_NAME_LENGTH 256

#define ADRENALINE_CFG_MAGIC_1 0x31483943
#define ADRENALINE_CFG_MAGIC_2 0x334F4E35

enum MemoryStickLocations {
  MEMORY_STICK_LOCATION_UX0,
  MEMORY_STICK_LOCATION_UR0,
  MEMORY_STICK_LOCATION_IMC0,
  MEMORY_STICK_LOCATION_XMC0,
  MEMORY_STICK_LOCATION_UMA0,
};

enum UsbDevices {
  USBDEVICE_MODE_MEMORY_CARD,
  USBDEVICE_MODE_INTERNAL_STORAGE,
  USBDEVICE_MODE_SD2VITA,
  USBDEVICE_MODE_PSVSD,
};

enum AdrenalineScreenSizes {
  SCREEN_SIZE_2_00,
  SCREEN_SIZE_1_75,
  SCREEN_SIZE_1_50,
  SCREEN_SIZE_1_25,
  SCREEN_SIZE_1_00,
};

enum AdrenalineScreenModes {
  SCREEN_MODE_ORIGINAL,
  SCREEN_MODE_NORMAL,
  SCREEN_MODE_ZOOM,
  SCREEN_MODE_FULL,
};

typedef struct {
  int magic[2];
  int no_smooth_graphics;
  int screen_size;
  int ms_location;
  int use_ds3_ds4;
  int screen_mode;
  float psp_screen_scale_x;
  float psp_screen_scale_y;
  float ps1_screen_scale_x;
  float ps1_screen_scale_y;
  int usbdevice;
} AdrenalineConfig;

extern AdrenalineConfig config;
extern SceUID usbdevice_modid;

extern int (* ScePspemuDivide)(uint64_t x, uint64_t y);
extern int (* ScePspemuErrorExit)(int error);
extern int (* ScePspemuConvertAddress)(uint32_t addr, int mode, uint32_t cache_size);
extern int (* ScePspemuWritebackCache)(void *addr, int size);
extern int (* ScePspemuKermitWaitAndGetRequest)(int mode, SceKermitRequest **request);
extern int (* ScePspemuKermitSendResponse)(int mode, SceKermitRequest *request, uint64_t response);
extern int (* ScePspemuConvertStatTimeToUtc)(SceIoStat *stat);
extern int (* ScePspemuConvertStatTimeToLocaltime)(SceIoStat *stat);
extern int (* ScePspemuSettingsHandler)(int a1, int a2, int a3, int a4);
extern int (* ScePspemuSetDisplayConfig)();
extern int (* ScePspemuPausePops)(int pause);
extern int (* ScePspemuInitPops)();
extern int (* ScePspemuInitPocs)();

extern tai_hook_ref_t sceCompatSuspendResumeRef;
extern tai_hook_ref_t sceCompatWriteSharedCtrlRef;
extern tai_hook_ref_t sceCompatWaitSpecialRequestRef;
extern tai_hook_ref_t sceShellUtilRegisterSettingsHandlerRef;
extern tai_hook_ref_t sceKernelCreateThreadRef;
extern tai_hook_ref_t sceIoOpenRef;
extern tai_hook_ref_t sceIoGetstatRef;
extern tai_hook_ref_t sceAudioOutOpenPortRef;
extern tai_hook_ref_t sceAudioOutOutputRef;
extern tai_hook_ref_t sceDisplaySetFrameBufForCompatRef;

extern tai_hook_ref_t ScePspemuInitTitleSpecificInfoRef;
extern tai_hook_ref_t ScePspemuGetStartupPngRef;
extern tai_hook_ref_t ScePspemuGetTitleidRef;
extern tai_hook_ref_t ScePspemuInitAudioOutRef;
extern tai_hook_ref_t ScePspemuConvertAddressRef;
extern tai_hook_ref_t ScePspemuDecodePopsAudioRef;
extern tai_hook_ref_t ScePspemuGetParamRef;

extern uint32_t module_nid;
extern uint32_t text_addr, text_size, data_addr, data_size;

int sceKernelIsPSVitaTV();
int kuCtrlPeekBufferPositive(int port, SceCtrlData *pad_data, int count);

void lockPower();
void unlockPower();

void SendAdrenalineRequest(int cmd);

#endif
