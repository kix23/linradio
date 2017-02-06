#ifndef _WRSERIAL_H_
#define _WRSERIAL_H_

#ifdef __cplusplus
extern "C" {
#endif

BOOL OpenSerialPort(int);
void CloseSerialPort(int);

BOOL ReadSerialByte(int, PBYTE, unsigned long ms);
BOOL SendSerialByte(int, BYTE);

void SetBaudRate(int, DWORD);

#ifdef __cplusplus
}
#endif

#endif 
