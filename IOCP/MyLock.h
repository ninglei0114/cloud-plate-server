#pragma once 
#include <Windows.h>

class CMyLock
{
public:
	CMyLock()
	{
		InitializeCriticalSection(&m_cs);
	}
	~CMyLock()
	{
		DeleteCriticalSection(&m_cs);
	}
	void Lock()
	{
		EnterCriticalSection(&m_cs);
	}
	void UnLock()
	{
		LeaveCriticalSection(&m_cs);
	}
private:
	CRITICAL_SECTION  m_cs;
};