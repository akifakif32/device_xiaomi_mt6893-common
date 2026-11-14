// SPDX-License-Identifier: Apache-2.0
//
// Send AT commands to the MediaTek modem through the mtkradioex HAL, the
// same way the EngineerMode app does it (IMtkRadioEx::sendRequestStrings on
// the "mtkEm<slot>" instance).
//
// Usage:
//   em_at <slot 1|2> <AT command> [<AT command> ...]
//   em_at --iran
//
// --iran: for every SIM slot whose operator numeric (gsm.sim.operator.numeric)
// starts with 432 (Iran), set persist.sys.em_at.iran_sim (init then switches
// ViLTE off), send the IMS configuration the Iranian networks need once and
// remember it in persist.sys.em_at.iran<slot>. Other networks are never touched.

#define LOG_TAG "em_at"

#include <vendor/mediatek/hardware/mtkradioex/3.0/IMtkRadioEx.h>

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/strings.h>

#include <unistd.h>

#include <cstdio>
#include <string>
#include <vector>

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::vendor::mediatek::hardware::mtkradioex::V3_0::IMtkRadioEx;

static sp<IMtkRadioEx> get_radio(int slot) {
    const std::string name = "mtkEm" + std::to_string(slot);
    for (int i = 0; i < 30; i++) {
        sp<IMtkRadioEx> radio = IMtkRadioEx::getService(name);
        if (radio != nullptr) return radio;
        sleep(2);
    }
    LOG(ERROR) << "no IMtkRadioEx/" << name << " service";
    return nullptr;
}

static bool send_at(const sp<IMtkRadioEx>& radio, int slot, int serial, const std::string& cmd) {
    // The RIL expects exactly two strings: the AT command and the response
    // prefix (empty here). Any other length is answered without sending
    // anything to the modem.
    hidl_vec<hidl_string> data;
    data.resize(2);
    data[0] = cmd;
    data[1] = "";

    auto ret = radio->sendRequestStrings(serial, data);
    if (!ret.isOk()) {
        LOG(ERROR) << "sendRequestStrings failed on slot " << slot << ": " << ret.description();
        return false;
    }
    LOG(INFO) << "slot " << slot << ": " << cmd;
    usleep(200 * 1000);
    return true;
}

static bool send_ims_cfg(const sp<IMtkRadioEx>& radio, int slot, const char* operator_code,
                         const char* ua_precondition, int serial) {
    return send_at(radio, slot, serial,
                   std::string("AT+ECFGSET=\"operator_code\",\"") + operator_code + "\"") &&
           send_at(radio, slot, serial + 1, "AT+ECFGSET=\"ims_signaling_qci\",\"5\"") &&
           send_at(radio, slot, serial + 2,
                   std::string("AT+ECFGSET=\"UA_call_precondition\",\"") + ua_precondition + "\"");
}

static int run_iran() {
    // "43211,43235" - one entry per slot, empty when there is no SIM.
    const std::string numeric = android::base::GetProperty("gsm.sim.operator.numeric", "");
    const std::vector<std::string> ops = android::base::Split(numeric, ",");

    for (size_t i = 0; i < ops.size() && i < 2; i++) {
        const int slot = static_cast<int>(i) + 1;
        if (!android::base::StartsWith(ops[i], "432")) continue;

        // Tell init right away so it can switch ViLTE off for Iranian SIMs
        // before the IMS configuration below takes effect.
        android::base::SetProperty("persist.sys.em_at.iran_sim", "1");

        const std::string marker = "persist.sys.em_at.iran" + std::to_string(slot);
        if (android::base::GetProperty(marker, "") == "1") continue;

        sp<IMtkRadioEx> radio = get_radio(slot);
        if (radio == nullptr) continue;

        // Same sequence EngineerMode's IMS page runs: the default profile
        // first, then the "CMW500 setting".
        if (send_ims_cfg(radio, slot, "0", "1", 100) && (sleep(3), true) &&
            send_ims_cfg(radio, slot, "16386", "0", 110)) {
            android::base::SetProperty(marker, "1");
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    android::base::InitLogging(argv, android::base::KernelLogger);

    if (argc == 2 && std::string(argv[1]) == "--iran") return run_iran();

    if (argc < 3) {
        fprintf(stderr, "usage: %s <slot 1|2> <AT command> [<AT command> ...]\n       %s --iran\n",
                argv[0], argv[0]);
        return 2;
    }

    const int slot = atoi(argv[1]);
    sp<IMtkRadioEx> radio = get_radio(slot);
    if (radio == nullptr) return 1;

    for (int i = 2; i < argc; i++) {
        if (!send_at(radio, slot, 100 + i, argv[i])) return 1;
    }
    return 0;
}
