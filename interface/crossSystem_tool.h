#ifndef CROSSSYSTEM_TOOL_H
#define CROSSSYSTEM_TOOL_H
#include "MySystem.h"

void sleep_ms(int milliseconds); // 跨系统sleep延时函数
void printF(const char* format, ...); // 跨系统打印函数
#endif