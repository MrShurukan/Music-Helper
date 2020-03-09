#pragma once
#include <winuser.h>

double dRand(double fMin, double fMax)
{
    double d = static_cast<double>(rand()) / RAND_MAX;
    return fMin + d * (fMax - fMin);
}

// ������������ ����� � �������� ��� ������
double hertz(double hertz)
{
    return hertz * 2 * PI;
}

bool keyIsHeld(char key)
{
    // �������� �� ����� ������� ���
    return GetAsyncKeyState(static_cast<unsigned char>(key)) & 0x8000;
}