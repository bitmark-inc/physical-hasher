// Copyright Bitmark Inc. 2015-2015

#ifndef _FOCUS_H_
#define _FOCUS_H_ 1

#include <stdbool.h>
#include <cyu3types.h>

bool Focus_Initialise(void);
void Focus_SetLine(const int32_t line, const uint8_t *buffer, const size_t buffer_length);
void Focus_EndFrame(const int32_t line);
void Focus_Start(void);
void Focus_Stop(void);

#endif
