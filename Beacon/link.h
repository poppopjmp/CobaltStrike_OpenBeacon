#pragma once
#include "protocol.h"

void LinkViaTcp(char* buffer, int length);

BOOL LinkAdd(PROTOCOL* protocol, int flags);

void PipeReopen(char* buffer, int length);