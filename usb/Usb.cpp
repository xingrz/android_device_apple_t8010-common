/*
 * Copyright (C) 2016 The Android Open Source Project
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
#include <assert.h>
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include <cutils/uevent.h>
#include <sys/epoll.h>
#include <utils/Errors.h>
#include <utils/StrongPointer.h>

#include "Usb.h"

namespace android {
namespace hardware {
namespace usb {
namespace V1_0 {
namespace implementation {

// Set by the signal handler to destroy the thread
volatile bool destroyThread;

bool roleIsOk(PortRole role) {
    if (role.type == PortRoleType::POWER_ROLE) {
        if (role.role ==  static_cast<uint32_t> (PortPowerRole::SINK))
            return true;
    } else if (role.type == PortRoleType::DATA_ROLE) {
        if (role.role == static_cast<uint32_t> (PortDataRole::DEVICE))
            return true;
    } else if (role.type == PortRoleType::MODE) {
        if (role.role == static_cast<uint32_t> (PortMode::UFP))
            return true;
    }
    return false;
}

Return<void> Usb::switchRole(const hidl_string& portName,
        const PortRole& newRole) {

    if(roleIsOk(newRole)) {
        ALOGI("Role switch successful");
        Return<void> ret = mCallback->notifyRoleSwitchStatus(portName, newRole, Status::SUCCESS);
        if (!ret.isOk())
            ALOGE("RoleSwitchStatus error %s", ret.description().c_str());
    } else {
        ALOGI("Role switch failed");
        Return<void> ret = mCallback->notifyRoleSwitchStatus(portName, newRole, Status::ERROR);
        if (!ret.isOk())
            ALOGE("RoleSwitchStatus error %s", ret.description().c_str());
    }

    return Void();
}

Status getCurrentRoleHelper(std::string portName,
        PortRoleType type, uint32_t &currentRole)  {
    (void)portName;
    if (type == PortRoleType::POWER_ROLE) {
        currentRole = static_cast<uint32_t>(PortPowerRole::SINK);
    } else if (type == PortRoleType::DATA_ROLE) {
        currentRole = static_cast<uint32_t> (PortDataRole::DEVICE);
    } else if (type == PortRoleType::MODE) {
        currentRole = static_cast<uint32_t> (PortMode::UFP);
    }

    return Status::SUCCESS;
}

bool canSwitchRoleHelper(const std::string portName, PortRoleType type)  {
    (void)portName;
    (void)type;
    return false;
}

Status getPortModeHelper(const std::string portName, PortMode& portMode)  {
    (void)portName;
    portMode = PortMode::UFP;
    return Status::SUCCESS;
}

Status getTypeCPortNamesHelper(std::vector<std::string>& names) {
    names.resize(1);
    names[0] = "usb0";
    return Status::SUCCESS;
}

Status getPortStatusHelper (hidl_vec<PortStatus>& currentPortStatus) {
    std::vector<std::string> names;
    Status result = getTypeCPortNamesHelper(names);

    if (result == Status::SUCCESS) {
        currentPortStatus.resize(names.size());
        for(std::vector<std::string>::size_type i = 0; i < names.size(); i++) {
            ALOGI("%s", names[i].c_str());
            currentPortStatus[i].portName = names[i];

            uint32_t currentRole;
            if (getCurrentRoleHelper(names[i], PortRoleType::POWER_ROLE,
                    currentRole) == Status::SUCCESS) {
                currentPortStatus[i].currentPowerRole =
                static_cast<PortPowerRole> (currentRole);
            } else {
                ALOGE("Error while retreiving portNames");
                goto done;
            }

            if (getCurrentRoleHelper(names[i],
                    PortRoleType::DATA_ROLE, currentRole) == Status::SUCCESS) {
                currentPortStatus[i].currentDataRole =
                        static_cast<PortDataRole> (currentRole);
            } else {
                ALOGE("Error while retreiving current port role");
                goto done;
            }

            if (getCurrentRoleHelper(names[i], PortRoleType::MODE,
                    currentRole) == Status::SUCCESS) {
                currentPortStatus[i].currentMode =
                    static_cast<PortMode> (currentRole);
            } else {
                ALOGE("Error while retreiving current data role");
                goto done;
            }

            currentPortStatus[i].canChangeMode =
                canSwitchRoleHelper(names[i], PortRoleType::MODE);
            currentPortStatus[i].canChangeDataRole =
                canSwitchRoleHelper(names[i], PortRoleType::DATA_ROLE);
            currentPortStatus[i].canChangePowerRole =
                canSwitchRoleHelper(names[i], PortRoleType::POWER_ROLE);

            ALOGI("canChangeMode: %d canChagedata: %d canChangePower:%d",
                currentPortStatus[i].canChangeMode,
                currentPortStatus[i].canChangeDataRole,
                currentPortStatus[i].canChangePowerRole);

            if (getPortModeHelper(names[i], currentPortStatus[i].supportedModes)
                  != Status::SUCCESS) {
                ALOGE("Error while retrieving port modes");
                goto done;
            }
        }
        return Status::SUCCESS;
    }
done:
    return Status::ERROR;
}

Return<void> Usb::queryPortStatus() {
    hidl_vec<PortStatus> currentPortStatus;
    Status status;

    status = getPortStatusHelper(currentPortStatus);
    Return<void> ret = mCallback->notifyPortStatusChange(currentPortStatus,
       status);
    if (!ret.isOk())
        ALOGE("queryPortStatus error %s", ret.description().c_str());

    return Void();
}
struct data {
    int uevent_fd;
    android::hardware::usb::V1_0::implementation::Usb *usb;
};

static void uevent_event(uint32_t /*epevents*/, struct data *payload) {
    char msg[UEVENT_MSG_LEN + 2];
    char *cp;
    int n;

    n = uevent_kernel_multicast_recv(payload->uevent_fd, msg, UEVENT_MSG_LEN);
    if (n <= 0)
        return;
    if (n >= UEVENT_MSG_LEN)   /* overflow -- discard */
        return;

    msg[n] = '\0';
    msg[n + 1] = '\0';
    cp = msg;

    while (*cp) {
        if (!strcmp(cp, "SUBSYSTEM=dual_role_usb")) {
            ALOGE("uevent received %s", cp);
            if (payload->usb->mCallback != NULL) {
                hidl_vec<PortStatus> currentPortStatus;
                Status status = getPortStatusHelper(currentPortStatus);
                Return<void> ret =
                    payload->usb->mCallback->notifyPortStatusChange(currentPortStatus, status);
                if (!ret.isOk())
                    ALOGE("error %s", ret.description().c_str());
            }
            break;
        }
        /* advance to after the next \0 */
        while (*cp++);
    }
}

void* work(void* param) {
    int epoll_fd, uevent_fd;
    struct epoll_event ev;
    int nevents = 0;
    struct data payload;

    ALOGE("creating thread");

    uevent_fd = uevent_open_socket(64*1024, true);

    if (uevent_fd < 0) {
        ALOGE("uevent_init: uevent_open_socket failed\n");
        return NULL;
    }

    payload.uevent_fd = uevent_fd;
    payload.usb = (android::hardware::usb::V1_0::implementation::Usb *)param;

    fcntl(uevent_fd, F_SETFL, O_NONBLOCK);

    ev.events = EPOLLIN;
    ev.data.ptr = (void *)uevent_event;

    epoll_fd = epoll_create(64);
    if (epoll_fd == -1) {
        ALOGE("epoll_create failed; errno=%d", errno);
        goto error;
    }

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, uevent_fd, &ev) == -1) {
        ALOGE("epoll_ctl failed; errno=%d", errno);
        goto error;
    }

    while (!destroyThread) {
        struct epoll_event events[64];

        nevents = epoll_wait(epoll_fd, events, 64, -1);
        if (nevents == -1) {
            if (errno == EINTR)
                continue;
            ALOGE("usb epoll_wait failed; errno=%d", errno);
            break;
        }

        for (int n = 0; n < nevents; ++n) {
            if (events[n].data.ptr)
                (*(void (*)(int, struct data *payload))events[n].data.ptr)
                    (events[n].events, &payload);
        }
    }

    ALOGI("exiting worker thread");
error:
    close(uevent_fd);

    if (epoll_fd >= 0)
        close(epoll_fd);

    return NULL;
}

void sighandler(int sig)
{
    if (sig == SIGUSR1) {
        destroyThread = true;
        ALOGI("destroy set");
        return;
    }
    signal(SIGUSR1, sighandler);
}

Return<void> Usb::setCallback(const sp<IUsbCallback>& callback) {

    pthread_mutex_lock(&mLock);
    if ((mCallback == NULL && callback == NULL) ||
            (mCallback != NULL && callback != NULL)) {
        mCallback = callback;
        pthread_mutex_unlock(&mLock);
        return Void();
    }

    mCallback = callback;
    ALOGI("registering callback");

    if (mCallback == NULL) {
        if  (!pthread_kill(mPoll, SIGUSR1)) {
            pthread_join(mPoll, NULL);
            ALOGI("pthread destroyed");
        }
        pthread_mutex_unlock(&mLock);
        return Void();
    }

    destroyThread = false;
    signal(SIGUSR1, sighandler);

    if (pthread_create(&mPoll, NULL, work, this)) {
        ALOGE("pthread creation failed %d", errno);
        mCallback = NULL;
    }
    pthread_mutex_unlock(&mLock);
    return Void();
}

// Protects *usb assignment
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
Usb *usb;

Usb::Usb() {
    pthread_mutex_lock(&lock);
    // Make this a singleton class
    assert(usb == NULL);
    usb = this;
    pthread_mutex_unlock(&lock);
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace usb
}  // namespace hardware
}  // namespace android
