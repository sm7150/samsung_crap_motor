/*
 * Copyright (C) 2020 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "CameraMotorService"

#include "CameraMotor.h"
#include <android-base/logging.h>
#include <hidl/HidlTransportSupport.h>
#include <fstream>
#include <cmath>
#include <thread>
#include <string>
#include <chrono>

#define TSP_CMD_PATH "/sys/class/sec/tsp/cmd"
#define SLIDING_ENABLE "/sys/devices/virtual/sec/sliding_motor/enable"
#define SLIDING_PWM "/sys/devices/virtual/sec/sliding_motor/pwm_active"
#define SLIDING_RETRY "/sys/devices/virtual/sec/sliding_motor/retry"
#define SLIDING_TEST "/sys/devices/virtual/sec/sliding_motor/test"

#define HIGH_SPEED	"1"
#define LOW_SPEED	"0"
#define TURN_ON		"1"
#define TURN_OFF	"0"
#define DIRECTION_UP	"1"
#define DIRECTION_DOWN "-1"
#define ENABLED		"1"
#define CAMERA_ID_FRONT "1"

/*
 * #define STARTING_SPEED	13333
 * #define FULL_SPEED	10416
 * #define ENDING_SPEED	40000

 * #define STARTING_SPEED_LOW_TEMP	1280000
 * #define FULL_SPEED_LOW_TEMP	640000
 * #define ENDING_SPEED_LOW_TEMP	426667
 *
 */

namespace vendor {
namespace lineage {
namespace camera {
namespace motor {
namespace V1_0 {
namespace implementation {

using namespace std::chrono_literals;

// Write value to path and close file.
template <typename T>
static void set(const std::string& path, const T& value) {
    std::ofstream file(path);
    file << value;
}

template <typename T>
static T get(const std::string& path, const T& def) {
    std::ifstream file(path);
    T result;

    file >> result;
    return file.fail() ? def : result;
}

static void waitUntilFileChange(const std::string& path, const std::string &val,
        const std::chrono::milliseconds relativeTimeout) {
    auto startTime = std::chrono::steady_clock::now();

    while (true) {
        if (get<std::string>(path, "") == val) {
            return;
        }

        std::this_thread::sleep_for(50ms);

        auto now = std::chrono::steady_clock::now();
        auto timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);

        if (timeElapsed > relativeTimeout) {
            return;
        }
    }
}

CameraMotor::CameraMotor() {
}

Return<void> CameraMotor::onConnect(const hidl_string& cameraId) {
    auto motorPosition = get<std::string>(SLIDING_ENABLE, "");

    if (cameraId == CAMERA_ID_FRONT && motorPosition == DIRECTION_DOWN) {
        LOG(INFO) << "Popping out front camera";

        set(SLIDING_ENABLE, DIRECTION_UP);
        set(SLIDING_PWM, ENABLED);
        waitUntilFileChange(SLIDING_ENABLE, DIRECTION_UP, 5s);
    } else if (cameraId != CAMERA_ID_FRONT && motorPosition == DIRECTION_UP) {
        LOG(INFO) << "Retracting front camera";

        set(SLIDING_ENABLE, DIRECTION_DOWN);
        set(SLIDING_PWM, ENABLED);
        waitUntilFileChange(SLIDING_ENABLE, DIRECTION_DOWN, 5s);
    }

    return Void();
}

Return<void> CameraMotor::onDisconnect(const hidl_string& cameraId) {
    auto motorPosition = get<std::string>(SLIDING_ENABLE, "");

    if (cameraId == CAMERA_ID_FRONT && motorPosition == DIRECTION_UP) {
        LOG(INFO) << "Retracting front camera";

        set(SLIDING_ENABLE, DIRECTION_DOWN);
        set(SLIDING_PWM, ENABLED);
        waitUntilFileChange(SLIDING_ENABLE, POSITION_DOWN, 5s);
    }

    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace motor
}  // namespace camera
}  // namespace lineage
}  // namespace vendor
