#ifndef _WRIO_H_
#define _WRIO_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Signed timer (milliseconds). */
/* Test GetTickCount()-timeout>0, not GetTickCount()>timeout. */
long GetTickCount();

BOOL WriteMcuByte(int, BYTE);
BOOL ReadMcuByte(int, PBYTE);
BOOL McuTransfer(int, int, PBYTE, int, PBYTE);
BYTE GetMcuStatus(int);
BOOL PerformReset(int);

BOOL LockDetect(int);
BOOL ValidateHandle(int, LPRADIOSETTINGS *);
void Delay(int);
BOOL SendI2CData(int, int, int, PBYTE);

#ifdef __cplusplus
}
#endif

#endif 
