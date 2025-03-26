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

#include <psp2/appmgr.h>
#include <psp2/apputil.h>
#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <psp2/system_param.h>
#include <psp2/sysmodule.h>
#include <psp2/power.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/dmac.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/processmgr.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <vita2d.h>

#include "main.h"
#include "menu.h"
#include "states.h"
#include "usb.h"
#include "utils.h"
#include "math_utils.h"

extern SceGxmContext *_vita2d_context;
int sceCommonDialogIsRunning();

vita2d_pgf *font;

int language = 0, enter_button = 0, date_format = 0, time_format = 0;

static int EnterStandbyMode();
static int OpenOfficialSettings();
static int ExitPspEmuApplication();
static int ResetAdrenalineSettings();

static AdrenalineConfig old_config;
static int tab_sel = 0;
static int menu_sel = 0;

static int changed = 0;
static int open_official_settings = 0;

static SceUID settings_semaid = -1;

static int EnterStandbyMode() {
  stopUsb(usbdevice_modid);
  ExitAdrenalineMenu();
  scePowerRequestSuspend();
  return 0;
}

static int OpenOfficialSettings() {
  open_official_settings = 1;
  ExitAdrenalineMenu();
  return 0;
}

static int ExitPspEmuApplication() {
  stopUsb(usbdevice_modid);
  ScePspemuErrorExit(0);
  return 0;
}

static int EnterAdrenalineMenu() {
  initStates();

  memcpy(&old_config, &config, sizeof(AdrenalineConfig));

  changed = 0;
  open_official_settings = 0;

  SceAdrenaline *adrenaline = (SceAdrenaline *)ScePspemuConvertAddress(ADRENALINE_ADDRESS, KERMIT_INPUT_MODE, ADRENALINE_SIZE);

  return 0;
}

int ExitAdrenalineMenu() {
  if (changed) {
    config.magic[0] = ADRENALINE_CFG_MAGIC_1;
    config.magic[1] = ADRENALINE_CFG_MAGIC_2;
    WriteFile("ux0:app/" ADRENALINE_TITLEID "/adrenaline.bin", &config, sizeof(AdrenalineConfig));
  }

  SceAdrenaline *adrenaline = (SceAdrenaline *)ScePspemuConvertAddress(ADRENALINE_ADDRESS, KERMIT_INPUT_MODE, ADRENALINE_SIZE);

  if (old_config.ms_location != config.ms_location)
    SendAdrenalineRequest(ADRENALINE_PSP_CMD_REINSERT_MS);

  SetPspemuFrameBuffer((void *)SCE_PSPEMU_FRAMEBUFFER);
  sceDisplayWaitVblankStart();

  sceKernelSignalSema(settings_semaid, 1);

  finishStates();

  return 0;
}

int ResetAdrenalineSettings() {
  memset(&config, 0, sizeof(AdrenalineConfig));
  config.magic[0] = ADRENALINE_CFG_MAGIC_1;
  config.magic[1] = ADRENALINE_CFG_MAGIC_2;
  config.psp_screen_scale_x = 2.0f;
  config.psp_screen_scale_y = 2.0f;
  config.ps1_screen_scale_x = 1.0f;
  config.ps1_screen_scale_y = 1.0f;
  WriteFile("ux0:app/" ADRENALINE_TITLEID "/adrenaline.bin", &config, sizeof(AdrenalineConfig));

  return 0;
}

void *pops_data = NULL;

int AdrenalineDraw(SceSize args, void *argp) {
  sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, &language);
  sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, &enter_button);
  sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_DATE_FORMAT, &date_format);
  sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_TIME_FORMAT, &time_format);

  settings_semaid = sceKernelCreateSema("AdrenalineSettingsSemaphore", 0, 0, 1, NULL);
  if (settings_semaid < 0)
    return settings_semaid;

  // keep track of entering pops mode
  int lastPops = 0;

  while (1) {
    SceAdrenaline *adrenaline = (SceAdrenaline *)CONVERT_ADDRESS(ADRENALINE_ADDRESS);

    // pause/unpause pops once after switching from psp to pops mode
    // this pause/unpause fixes slowdown in PS1 games that used to require manually entering/exiting menu
    if (!adrenaline->pops_mode) {
      lastPops = 0;
    }

    if (adrenaline->pops_mode && lastPops == 0) {
      ScePspemuPausePops(1);
      sceDisplayWaitVblankStart();
      ScePspemuPausePops(0);
      sceDisplayWaitVblankStart();
      lastPops = 1;
    }

    // Read pad
    readPad();

    // Double click detection
    if (doubleClick(SCE_CTRL_PSBUTTON, 300 * 1000)) {
      stopUsb(usbdevice_modid);

      if (sceAppMgrLaunchAppByName2(ADRENALINE_TITLEID, NULL, NULL) < 0)
        ScePspemuErrorExit(0);
    }

    // Fast forward in pops
    if (adrenaline->pops_mode) {
      // FSM for button combination
      static int combo_state = 0;
      if (current_pad[PAD_LTRIGGER] && current_pad[PAD_SELECT]) {
        combo_state = 1;
      } else {
        if (combo_state == 1)
          combo_state = 2;
        else
          combo_state = 0;
      }

      if (combo_state == 2) {
        uint8_t *val = (uint8_t *)ScePspemuConvertAddress(0xABCD00A9, KERMIT_OUTPUT_MODE, 1);
        *val = !(*val);
        ScePspemuWritebackCache(val, 1);
        combo_state = 0;
      }
    }

    // Sync
    if ((!adrenaline->pops_mode) || adrenaline->draw_psp_screen_in_pops)
      sceCompatLCDCSync();
    else
      sceDisplayWaitVblankStart();
  }

  return sceKernelExitDeleteThread(0);
}

int ScePspemuCustomSettingsHandler(int a1, int a2, int a3, int a4) {
  return ScePspemuSettingsHandler(a1, a2, a3, a4);
}
