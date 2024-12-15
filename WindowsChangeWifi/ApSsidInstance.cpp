//
// Created by 86158 on 2024/11/29.
//

#include "ApSsidInstance.h"
#include "utils/CDefer.h"
#include "configs.h"

#include <iomanip>

namespace windows::wifi {
    bool ApSsidInstance::InitialHandle() {
        DWORD dwCurVersion = 0;
        if (wlanHandle == nullptr) {
            DWORD dwResult;
            if (constexpr DWORD dwMaxClient = 2;
                (dwResult = WlanOpenHandle(dwMaxClient, nullptr,
                    &dwCurVersion, &wlanHandle)) != ERROR_SUCCESS) {
                std::cout << "wlanOpenHandle failed with error: " << dwResult << std::endl;
                wlanHandle = nullptr;
                return false;
            }
            // 注册消息通知回调
            if ((dwResult = WlanRegisterNotification(wlanHandle, WLAN_NOTIFICATION_SOURCE_ALL,
                TRUE, OnNotificationCallback,
                nullptr, nullptr, nullptr)) != ERROR_SUCCESS) {
                std::cout << "wlanRegisterNotification failed with error: " << dwResult << std::endl;
                wlanHandle = nullptr;
                return false;
            }
        }
        return true;
    }

    bool ApSsidInstance::getNetworkAndInterfaceList(
            PWLAN_AVAILABLE_NETWORK_LIST&pBssList,
            PWLAN_INTERFACE_INFO_LIST&pIfList,
            const _INTERNAL_APPLY_NETWORK_LIST &fn) {
        PWLAN_INTERFACE_INFO pIfInfo;
        WCHAR GuidString[39] = {};

        unsigned int i;

        DWORD dwResult = WlanEnumInterfaces(wlanHandle, nullptr, &pIfList);
        if (dwResult != ERROR_SUCCESS) {
            return false;
        }
        for (i = 0; i < pIfList->dwNumberOfItems; i++) {
            pIfInfo = &pIfList->InterfaceInfo[i];

            const int iRet = StringFromGUID2(pIfInfo->InterfaceGuid, reinterpret_cast<LPOLESTR>(&GuidString),
                                             std::size(GuidString));
            if (iRet == 0) {
                std::cout << "String From GUID2 Failed." << std::endl;
                return false;
            }
            // 向无线网卡发送探测请求
            dwResult = WlanScan(wlanHandle, &pIfInfo->InterfaceGuid, nullptr, nullptr, nullptr);
            if (dwResult != ERROR_SUCCESS) {
                return false;
            }
        }

        {
            std::unique_lock lock(mtx);
            cv.wait_for(lock, std::chrono::seconds(4));
        }

        for (i = 0; i < static_cast<int>(pIfList->dwNumberOfItems); i++)
        {
            pIfInfo = &pIfList->InterfaceInfo[i];
            dwResult = WlanGetAvailableNetworkList(wlanHandle,
                                                   &pIfInfo->InterfaceGuid,
                                                   0,
                                                   nullptr,
                                                   &pBssList);

            if (dwResult != ERROR_SUCCESS)
            {
                std::cout << "WlanGetAvailableNetworkList failed with error:" << dwResult;
                return false;
            }
            if (fn) {
                // if fn().first 返回真，那么跳出循环
                if (auto [fst, snd] = fn(pIfInfo, pBssList); fst) {
                    return snd;
                }
            }
        }

        return true;
    }

    bool ApSsidInstance::connectToWifi(const std::string &targetSsid) {
        // 初始化错误
        if (!InitialHandle()) {
            return false;
        }

        PWLAN_AVAILABLE_NETWORK_LIST pNetworkList = nullptr;
        PWLAN_INTERFACE_INFO_LIST pIfList = nullptr;
        // free memory
        utils::CDefer pIfRaii(
            nullptr,
            [&pNetworkList, &pIfList]() {
                freeMemory(pNetworkList, pIfList);
            });

        const _INTERNAL_APPLY_NETWORK_LIST fn = [this, &targetSsid](const PWLAN_INTERFACE_INFO& info, const PWLAN_AVAILABLE_NETWORK_LIST& networkList) -> std::pair<bool, bool> {
            auto res = this->connectingWifi(networkList, targetSsid, info);
            return {true, res};
        };

        return getNetworkAndInterfaceList(pNetworkList, pIfList, fn);
    }

    bool ApSsidInstance::connectingWifi(const PWLAN_AVAILABLE_NETWORK_LIST &pNetworkList, const std::string &targetSsid, const PWLAN_INTERFACE_INFO &info) const {
        for (unsigned int j = 0; j < pNetworkList->dwNumberOfItems; ++j) {
            // ReSharper disable once CppUseStructuredBinding
            auto& network = pNetworkList->Network[j];

            if (std::string ssid(reinterpret_cast<const char*>(network.dot11Ssid.ucSSID), network.dot11Ssid.uSSIDLength); ssid == targetSsid) {
                // 创建连接参数
                WLAN_CONNECTION_PARAMETERS connParams = {};
                connParams.wlanConnectionMode = wlan_connection_mode_profile; // 使用保存的配置文件
                connParams.strProfile = network.strProfileName; // 使用网络的配置文件名
                connParams.pDot11Ssid = &network.dot11Ssid;
                connParams.dot11BssType = network.dot11BssType;
                connParams.dwFlags = 0;

                // 尝试连接
                if (const auto dwResult = WlanConnect(wlanHandle, &info->InterfaceGuid, &connParams, nullptr); dwResult == ERROR_SUCCESS) {
                    std::cout << "Successfully connected to " << targetSsid << std::endl;
                    return true;
                } else {
                    std::cerr << "WlanConnect failed with error: ";
                    if (dwResult == ERROR_INVALID_PARAMETER) {
                        std::cout << "error invalid parameter." << std::endl;
                    }
                    else {
                        std::wcout << GetErrorMessage(dwResult) << std::endl;
                    }

                    return false;
                }
            }
        }
        return false;
    }

    void ApSsidInstance::insertWifi(const PWLAN_AVAILABLE_NETWORK_LIST& pBssList) {
        // clear ssidList.
        ssidList.clear();
        for (unsigned int j = 0; j < pBssList->dwNumberOfItems; j++)
        {
            // ReSharper disable once CppLocalVariableMayBeConst
            PWLAN_AVAILABLE_NETWORK pBssEntry = &pBssList->Network[j];

            if (auto temp = std::string(reinterpret_cast<char*>(pBssEntry->dot11Ssid.ucSSID));
                    !temp.empty()  && pBssEntry->bNetworkConnectable ) {
                SelfWifi wifi(temp, static_cast<int>(pBssEntry->wlanSignalQuality));
                ssidList.insert(wifi);
            }
        }
    }

    // ReSharper disable once CppParameterMayBeConst
    void OnNotificationCallback(PWLAN_NOTIFICATION_DATA data, [[maybe_unused]] PVOID context) {
        if (data != nullptr && data->NotificationSource == WLAN_NOTIFICATION_SOURCE_ACM &&
            (data->NotificationCode == wlan_notification_acm_scan_complete || data->NotificationCode == wlan_notification_acm_start)) {
            ApSsidInstance::getInstance()->CvNotify();
        }
    }

    void ApSsidInstance::CvNotify() {
        cv.notify_all();
    }

    void freeMemory(PWLAN_AVAILABLE_NETWORK_LIST& pBssList, PWLAN_INTERFACE_INFO_LIST& pIfList) {
        if (pBssList != nullptr) {
            WlanFreeMemory(pBssList);
            pBssList = nullptr;
        }

        if (pIfList != nullptr) {
            WlanFreeMemory(pIfList);
            pIfList = nullptr;
        }
    }

    ApSsidInstance::~ApSsidInstance() {
        if (const auto result = WlanCloseHandle(wlanHandle, nullptr); result != ERROR_SUCCESS) {
            std::cerr << "WlanCloseHandle failed with error: " << result << std::endl;
        }
    }

    bool ApSsidInstance::GetSsidList() {
        if (!InitialHandle()) {
            std::cout << "Initialize Wlan Handle Failed." << std::endl;
            return false;
        }
        PWLAN_INTERFACE_INFO_LIST pIfList = nullptr;
        PWLAN_AVAILABLE_NETWORK_LIST pBssList = nullptr;

        // free memory
        utils::CDefer pIfRaii(
            nullptr,
            [&pBssList, &pIfList]() {
                freeMemory(pBssList, pIfList);
            });

        const _INTERNAL_APPLY_NETWORK_LIST fn = [this](const PWLAN_INTERFACE_INFO&,const PWLAN_AVAILABLE_NETWORK_LIST& list) {
            this->insertWifi(list);
            constexpr std::pair r = {false, false};
            return r;
        };
        return getNetworkAndInterfaceList(pBssList, pIfList, fn);
    }

    bool ApSsidInstance::connectToWifi(const int index) {
        if (index < 0 || index > static_cast<int>(ssidList.size())) {
            return false;
        }
        auto it = ssidList.begin(); // iterator
        std::advance(it, index - 1);
        if (it == ssidList.end()) {
            return false;   // There is no any WI-FI in the index of [index]
        }

        return connectToWifi(it->getName());
    }

    void ApSsidInstance::showSsidListWithIndex() const {
        const int ssidListNumber = static_cast<int>(ssidList.size());

        std::cout << "There are " << ssidListNumber << " Wifi you can choose" << std::endl;
        int i = 0;
        for (const auto & ssid : ssidList) {
            std::cout << std::right;
            std::cout << std::setw(5) << ++i << ". ";

            std::cout << std::left;
            std::cout << ssid << std::endl;
        }
    }
}

