//
// Created by 86158 on 2024/11/29.
//

#ifndef AP_SSID_INSTANCE_H
#define AP_SSID_INSTANCE_H

#include "../utils/Singleton.h"
#include "SelfWifi.h"

#include <windows.h>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <set>
#include <wlanapi.h>

namespace windows::wifi {
    void OnNotificationCallback(PWLAN_NOTIFICATION_DATA data, PVOID context);

    void freeMemory(PWLAN_AVAILABLE_NETWORK_LIST& pBssList, PWLAN_INTERFACE_INFO_LIST& pIfList);

    // �ڲ���������
    // �� fn.first == trueʱ���������ô˺�����ѭ����ǰ�˳�����ֱ�ӷ���fn.second
    using _INTERNAL_APPLY_NETWORK_LIST = std::function<std::pair<bool, bool>(PWLAN_INTERFACE_INFO, PWLAN_AVAILABLE_NETWORK_LIST &)>; // NOLINT(*-reserved-identifier)

    class ApSsidInstance : public Singleton<ApSsidInstance> {
    public:
        // destructor , close handle and so on.
        ~ApSsidInstance();

        void CvNotify();

        /**
         * ���ڿ���ǰ��ɨ����ڶ���ɨ���ʱ����Ƚϴ�wifi���ܱ����رյȵȺܶ����������ÿ�ζ����ʼ���ֱ�����ͷ��ʼ
         * @return if we scan successfully, it will return true.
         */
        bool GetSsidList();

        /**
         *
         * @param index int, ��ʾssid�ĵ�i��Ԫ�� ���Ӹ�wifi
         * @return ���ӳɹ�����true
         */
        bool connectToWifi(int index);

        // show ssidList in the format like the one below.
        // 1: Jiguang
        // 2: Zearo
        void showSsidListWithIndex() const;

    private:
        // ��ʼ���ֱ�
        bool InitialHandle();

        /**
         * ��ȡ�ӿ��б��Լ����������б�, ��ִ��fn
         * @param pBssList
         * @param pIfList
         * @param fn ���fn �����棬��ô��ѭ���л���ǰ�˳������򲻻�
         */
        bool getNetworkAndInterfaceList(
            PWLAN_AVAILABLE_NETWORK_LIST& pBssList,
            PWLAN_INTERFACE_INFO_LIST& pIfList,
            const _INTERNAL_APPLY_NETWORK_LIST &fn);

        // try to connect to Wi-Fi
        bool connectToWifi(const std::string& targetSsid);

        // connecting to Wi-Fi
        [[nodiscard]] bool connectingWifi(const PWLAN_AVAILABLE_NETWORK_LIST &pNetworkList, const std::string& targetSsid, const PWLAN_INTERFACE_INFO &info) const;

        // ���뵽 ssidList set ��
        void insertWifi(const PWLAN_AVAILABLE_NETWORK_LIST& pBssList);

        HANDLE wlanHandle = nullptr;
        std::mutex mtx;
        std::condition_variable cv;

        // WIFI list
        std::set<SelfWifi, _MaxComparatorSelfWifi> ssidList;
    };

}

#endif //AP_SSID_INSTANCE_H
