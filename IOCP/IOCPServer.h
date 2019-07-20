
#ifndef IOCPSERVER_H
#define IOCPSERVER_H




#include "INet.h"
#include "IKernel.h"
#include "Packdef.h"


enum NetType{NT_UNKOWN,NT_READ,NT_WRITE,NT_ACCEPT};

struct MySocket
{
	MySocket()
	{
		pIndex = NULL;
		olp.hEvent = NULL;
		sock = NULL;
		m_nType = NT_UNKOWN;
		ZeroMemory(szbuf,_DEFAULT_SIZE);
	}
	OVERLAPPED olp; //事件
	SOCKET sock ;//要发生网络事件的socket
	NetType m_nType; //网络事件
	char   szbuf[_DEFAULT_SIZE]; //缓冲区
	long   *pIndex;


};




class IOCPServer :public CNet
{
public:
	IOCPServer(IKernel *pMediator);
	virtual ~IOCPServer();
public:
	 bool InitNetWork() ; 
	 void UnInitNetWork();
	 bool SendData(MySocket *pSockex,char* szbuf,int nLen);
	 
public:
	bool  PostAccept();
	bool  PostRecv(MySocket *pSock);
	static  unsigned _stdcall ThreadProc( void * );

public:
	static ULONG GetLocalIP()
	{
			//获取主机IP
		char szHostName[MAX_PATH] = {0};
		in_addr IPaddr;
		if(!gethostname(szHostName,MAX_PATH))
		{
			//通过主机名称获得IP列表
			hostent* pHostIpList = gethostbyname(szHostName);
			IPaddr.S_un.S_addr = *(ULONG*)pHostIpList->h_addr_list[0];
		}
		return IPaddr.S_un.S_addr;
	}
private:
	SOCKET m_sockListen;
	int    m_nNumPro;
	HANDLE m_hiocp;
	list<HANDLE> m_lstThread;

	bool   m_bFlagQuit;
	map<long,MySocket*> m_mapIpToMySocket;
	MyQueue<long*>  m_qIndex;

	MySocket*    m_aryMySocket;

	IKernel *m_pMediator;
};
#endif
