#pragma once

typedef struct _EXPANDED_CMD
{
	char* fullCmd;
	char* cmd;
} EXPANDED_CMD;

BOOL ArgumentFindMatch(EXPANDED_CMD* extendedCmd, const char* cmd);
void ArgumentAdd(char* buffer, int length);