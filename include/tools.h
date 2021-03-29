#ifndef TOOLS_H_
#define TOOLS_H_
#include <stddef.h>
#include <stdint.h>

void serialDumpPage(uint8_t* buf);
bool checkCrcBuf(uint8_t* buf, size_t bufSize);
bool updateCrcBuf(uint8_t* buf, size_t bufSize);

#endif
