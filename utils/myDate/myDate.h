//
// Created by 86158 on 2024/11/1.
//

#ifndef MY_DATE_H
#define MY_DATE_H

#include <string>

class myDate {
public:
    /**
     * Tomohiko Sakamoto �㷨����ȡ��ǰ���������ڼ� (1582-10-15 ֮��)
     * @param year
     * @param month
     * @param day
     * @return int
     */
    static int dow(int year, int month, int day);

    /**
     * ���������ִ��󣬷��� ��
     * @param day ���ڼ� (0~6), ���Ϊ0,����������
     * @return ���ڼ���Ӣ��
     */
    static std::string getDowString(int day);

    myDate();

private:
    int year;
    int month;
    int day;
    int hour;
    int min;
    int sec;

public:
    [[nodiscard]] int getYear() const;
    [[nodiscard]] int getMonth() const;
    [[nodiscard]] int getDay() const;
    [[nodiscard]] int getHour() const;
    [[nodiscard]] int getMinute() const;
    [[nodiscard]] int getSecond() const;
    [[nodiscard]] std::string getDow() const;
};

#endif //MY_DATE_H
