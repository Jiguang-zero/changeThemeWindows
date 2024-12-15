//
// Created by 86158 on 2024/11/28.
//

#include "configs.h"
#include <windows.h>

namespace windows::wifi {
    void configs::applyConfigs() const {
        if (utf8Encoded) {
            SetConsoleOutputCP(CP_UTF8);
        } else {
            SetConsoleOutputCP(CP_ACP); // ANSI
        }
    }

    void configs::setUtf8Encoded() {
        utf8Encoded = true; // default value
    }

    void configs::setUtf8Encoded(const bool flag) {
        utf8Encoded = flag;
    }

    std::wstring GetErrorMessage(const DWORD errorCode) {
        wchar_t* messageBuffer = nullptr;

        // ʹ�� FormatMessageW ��ȡ���ַ��Ĵ����ַ���
        const DWORD size = FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            errorCode,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // ʹ��Ĭ������
            reinterpret_cast<LPWSTR>(&messageBuffer),  // �Զ������ڴ�
            0,
            nullptr
        );

        std::wstring errorMessage;
        if (size > 0 && messageBuffer) {
            errorMessage = messageBuffer; // ת��Ϊ std::wstring
        } else {
            errorMessage.clear(); // �հ�
        }

        // �ͷŷ�����ڴ�
        if (messageBuffer) {
            LocalFree(messageBuffer);
        }

        return errorMessage;
    }
}
