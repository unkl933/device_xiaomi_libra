/*
   Copyright (c) 2020, The LineageOS Project
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.
   THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_

#include "property_service.h"
#include <fstream>
#include <sys/_system_properties.h>
#include <sys/sysinfo.h>
#include <android-base/properties.h>

#define AQUA_BOARD_ID 30
#define LIBRA_BOARD_ID 12
#define BOARD_ID_PATH "/proc/device-tree/qcom,board-id"

using std::ifstream;
using android::base::GetProperty;
using android::init::property_set;

char const *heapstartsize;
char const *heapgrowthlimit;
char const *heapsize;
char const *heapminfree;
char const *heapmaxfree;

void property_override(const char *prop, const char *value)
{
    prop_info *info = (prop_info *)__system_property_find(prop);

    if (info) // Update existing property
    {
        __system_property_update(info, value, strlen(value));
    }
    else // Create new property
    {
        __system_property_add(prop, strlen(prop), value, strlen(value));
    }
}

inline void property_override(const char *system_prop, const char *vendor_prop, const char *value)
{
    property_override(system_prop, value);
    property_override(vendor_prop, value);
}

uint8_t get_board_id()
{
    uint8_t board_id;

    /*
      qcom,board-id contains two 4-byte numbers,
      For libra, 00 00 00 0c and 00 00 00 00.
      For aqua, 00 00 00 1e and 00 00 00 00.
     */
    ifstream board_id_file(BOARD_ID_PATH, ifstream::binary);
    board_id_file.seekg(3); // Shift past the first 3 bytes and only read the 4th one
    board_id_file.read(reinterpret_cast<char *>(&board_id), 1);

    return board_id;
}

void vendor_load_properties()
{
    switch (get_board_id())
    {
    case LIBRA_BOARD_ID:
    {
        struct sysinfo sys;
        sysinfo(&sys);

        // set different Davlik heap properties for 2 GB
        if (sys.totalram > 2048ull * 1024 * 1024) {
            heapgrowthlimit = "192m";
            heapsize = "512m";
        // from phone-xhdpi-4096-dalvik-heap.mk
            heaptargetutilization = "0.6";
            heapminfree = "8m";
            heapmaxfree = "16m";
        } else {
        // from go_default_common.prop
            heapgrowthlimit = "128m";
            heapsize = "256m";
        // from phone-xhdpi-2048-dalvik-heap.mk
            heaptargetutilization = "0.75";
            heapminfree = "512k";
            heapmaxfree = "8m";
        }
        // set go tweaks for LMK for 2/3GB
        if (sys.totalram > 3072ull * 1024 * 1024) {
            property_set("ro.lmk.critical_upgrade", "true");
            property_set("ro.lmk.upgrade_pressure", "40");
            property_set("ro.lmk.downgrade_pressure", "60");
            property_set("ro.lmk.kill_heaviest_task", "false");
        }

        // set rest of Go tweaks for 2 GB
        if (sys.totalram < 2048ull * 1024 * 1024) {
        // set lowram options and enable traced by default
            property_set("ro.config.low_ram", "true");
            property_set("persist.traced.enable", "true");
            property_set("ro.statsd.enable", "true");
        // set threshold to filter unused apps
            property_set("pm.dexopt.downgrade_after_inactive_days", "10");
        // set the compiler filter for shared apks to quicken
            property_set("pm.dexopt.shared", "quicken");
    }

        break;
    }

    case AQUA_BOARD_ID:
    {
        // Set device info for Mi-4s
        property_override("ro.build.product", "aqua");
        property_override("ro.product.device", "ro.vendor.product.device", "aqua");
        property_override("ro.product.model", "ro.vendor.product.model", "Mi-4s");

        // Set memory parameters
        property_override("dalvik.vm.heapgrowthlimit", "288m");
        property_override("dalvik.vm.heapminfree", "512k");
        property_override("dalvik.vm.heapsize", "768m");
        property_override("dalvik.vm.heapstartsize", "8m");
        property_override("ro.product.ramsize", "3g");

        // Set fingerprint parameters
        property_override("ro.frp.pst", "/dev/block/bootdevice/by-name/config");
        property_override("ro.hardware.fingerprint", "fpc");
        property_override("sys.fpc.tu.disabled", "0");

        break;
    }
    }
        property_set("dalvik.vm.heapstartsize", heapstartsize);
        property_set("dalvik.vm.heapgrowthlimit", heapgrowthlimit);
        property_set("dalvik.vm.heapsize", heapsize);
        property_set("dalvik.vm.heaptargetutilization", heaptargetutilization);
        property_set("dalvik.vm.heapminfree", heapminfree);
        property_set("dalvik.vm.heapmaxfree", heapmaxfree);
}
