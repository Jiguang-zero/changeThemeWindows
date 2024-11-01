//
// Created by 86158 on 2024/11/1.
//

#ifndef LOGGER_H
#define LOGGER_H

// Logger.h
#include "../Singleton.h"

#include <fstream>

// CRTP, ����̳�ģ����ʽ
class Logger : public Singleton<Logger> {
public:
    ~Logger();

private:
    // ȫ����־�ļ��Ĵ洢
    const static char* LOG_FILE;

    // ȫ����־�ļ����ļ��� log
    const static char* LOG_PATH;

    std::ofstream outFile;
    // ������(�̰߳�ȫ)
    std::mutex mtx;

public:
    std::shared_ptr<Logger> &operator << (const std::string &message) const {
        _instance->info(message);

        return _instance;
    }

    static std::shared_ptr<Logger> getInstance() {
        static std::once_flag s_flag;
        std::call_once(s_flag, [&]() {
            _instance = std::make_shared<Logger>();
            _instance->start();
        });

        return _instance;
    }
private:
    // ����־�ļ� ��
    void start();

    // �ر���־�ļ� ��
    void stop();

    /**
     * ����־�ļ���Ϣ�洢���ļ���
     * @param message const std::string&
     */
    void info(const std::string& message);

};

// ����꣬����ʹ��
#ifndef LOG_GLOBAL
#define LOG *Logger::getInstance()
#endif //LOG_GLOBAL

#endif //LOGGER_H
