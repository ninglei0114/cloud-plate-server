#ifndef _IKERNEL_H

#define _IKERNEL_H

#include "INet.h"

class IKernel
{
public:
	IKernel()
	{}
	virtual ~IKernel()
	{}
public:
	virtual bool Open() =0;
	virtual void Close() = 0;
	virtual bool DealData(MySocket *pSock,char *szbuf) = 0;

public:
	 CNet *m_pNet;
};



#endif