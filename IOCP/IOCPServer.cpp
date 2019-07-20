#include "IOCPServer.h"


IOCPServer::IOCPServer(IKernel *pMediator)
{
	m_sockListen = NULL;
	m_nNumPro = 0;
	m_hiocp = NULL;
	m_bFlagQuit = true;
	m_pMediator = pMediator;
}


IOCPServer::~IOCPServer(void)
{
}
bool IOCPServer::InitNetWork()
{
	 //0.选择类型 --- 加载库
	WORD wVersionRequested;
    WSADATA wsaData;
    int err;


    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) 
	{
    
		return false;
    }



    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
      
		UnInitNetWork();
        return false;
    }
   
     
	m_sockListen =	WSASocket(AF_INET,SOCK_STREAM,IPPROTO_TCP,0,0,WSA_FLAG_OVERLAPPED);
	if(INVALID_SOCKET == m_sockListen)
	{
		UnInitNetWork();
        return false;
	}
	sockaddr_in addr;
	addr.sin_addr.S_un.S_addr =/* GetLocalIP();*/INADDR_ANY;
	addr.sin_port = htons(_DEFAULT_PORT);
	addr.sin_family = AF_INET;
	if( SOCKET_ERROR == bind(m_sockListen,(const sockaddr*)&addr,sizeof(addr)))
	{
		UnInitNetWork();
        return false;
	}

	SYSTEM_INFO  si;
	GetSystemInfo(&si);
	m_nNumPro = si.dwNumberOfProcessors*2;
	if(SOCKET_ERROR ==  listen(m_sockListen,m_nNumPro))
	{
		UnInitNetWork();
        return false;
	}

	//内存池
	m_aryMySocket = new MySocket[MAX_RQ_COUNT];


	//初始化队列--索引池
	if(!m_qIndex.InitQueue(MAX_RQ_COUNT))
	{
		UnInitNetWork();
        return false;
	}

	for(int i = 0; i<MAX_RQ_COUNT;i++ )
	{

		m_qIndex.push(new long(i));
	}

	//投递请求--接收连接
	for(int i = 0; i< m_nNumPro;i++)
	{
		if(!PostAccept())
		{
			continue;
		}
	}
	//完成端口

	//创建完成端口
	m_hiocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,NULL,0);
	if(m_hiocp == NULL)
	{
		UnInitNetWork();
        return false;
	}
	
	//将m_sockListen 交给完成端口管理
	CreateIoCompletionPort((HANDLE)m_sockListen,m_hiocp,m_sockListen,0);

	//创建线程池
	for(int i = 0; i <m_nNumPro;i++ )
	{

		 HANDLE hThread = (HANDLE)_beginthreadex(0,0,&ThreadProc,this,0,0);
		 if(hThread)
		 {
			 m_lstThread.push_back(hThread);
		 }
	}

	

	return true;
}

unsigned _stdcall IOCPServer::ThreadProc( void * lpvoid)
{
	IOCPServer *pthis = (IOCPServer *)lpvoid;
	DWORD dwNumberOfBytes;
	SOCKET sock;
	MySocket *pSock = NULL;
	BOOL bflag;
	while(pthis->m_bFlagQuit)
	{
		//获得完成端口的状态
		bflag = GetQueuedCompletionStatus(pthis->m_hiocp,&dwNumberOfBytes,(PULONG_PTR)&sock,(LPOVERLAPPED*)&pSock,INFINITE);
		if(!bflag)
		{
			//将waiter换到数组中
			pthis->m_qIndex.push(pSock->pIndex);
			//将sockwaiter 从map中移走
			auto ite =   pthis->m_mapIpToMySocket.begin();
			while(ite !=  pthis->m_mapIpToMySocket.end())
			{
				if(ite->second == pSock)
				{
					pthis->m_mapIpToMySocket.erase(ite);
					pSock->pIndex = NULL;
					break;
				}
				ite++;
			}

			continue;
		}

		if(sock  && pSock)
		{
			switch (pSock->m_nType)
			{
			case NT_ACCEPT:  //已经有客户端链接啦
				{
					sockaddr_in *LocalSockaddr,*RemoteSockaddr;
					int LocalSockaddrLength,RemoteSockaddrLength;
					//获得客户端的ip
					GetAcceptExSockaddrs(pSock->szbuf,0,sizeof(sockaddr_in) + 16,sizeof(sockaddr_in) + 16,
						(sockaddr**)&LocalSockaddr,&LocalSockaddrLength,(sockaddr**)&RemoteSockaddr,&RemoteSockaddrLength);
					pthis->m_mapIpToMySocket[RemoteSockaddr->sin_addr.S_un.S_addr] = pSock;
					
					//再次投递连接请求
					pthis->PostAccept();

					//投递接收数据的请求
					pthis->PostRecv(pSock);
                    
				   //将waiter 交给完成端口管理
					CreateIoCompletionPort((HANDLE)pSock->sock,pthis->m_hiocp,pSock->sock,0);
				}
				break;
			case NT_READ:
				{
					//处理数据
				
					pthis->m_pMediator->DealData(pSock,pSock->szbuf);
					//投递接收数据的请求
					pthis->PostRecv(pSock);
                    

				}
				break;
			default:
				break;
			}
		}
		//已经有客户端


	}
	return 0;
}

bool IOCPServer::PostRecv(MySocket *pSock)
{
	DWORD dwNumberOfBytesRecvd;
	DWORD dwFlags = false;
	WSABUF wb;
	wb.buf = pSock->szbuf;
	wb.len = sizeof(pSock->szbuf);
	pSock->m_nType = NT_READ;
	if(WSARecv(pSock->sock,&wb,1,&dwNumberOfBytesRecvd,&dwFlags,&pSock->olp,NULL))
	{
		if(WSAGetLastError() !=  WSA_IO_PENDING)
		{
			return false;
		}
	}

	return true;
}

bool IOCPServer::PostAccept()
{
	long *pIndex  = NULL;
	m_qIndex.pop(pIndex);
	if(pIndex == NULL)return false;
	
	MySocket *pSocket= &m_aryMySocket[*pIndex];
	if(pSocket == NULL)return false;

//	MySocket *pSocket = new  MySocket;
	pSocket->m_nType = NT_ACCEPT;
	pSocket->olp.hEvent = WSACreateEvent();
	pSocket->sock  = WSASocket(AF_INET,SOCK_STREAM,IPPROTO_TCP,0,0,WSA_FLAG_OVERLAPPED); 
	ZeroMemory(pSocket->szbuf,sizeof(pSocket->szbuf));
	pSocket->pIndex = pIndex;

	DWORD dwBytesReceived;
	if(!AcceptEx(m_sockListen,pSocket->sock,pSocket->szbuf,0,sizeof(sockaddr_in)+16,sizeof(sockaddr_in)+16,
		     &dwBytesReceived,&pSocket->olp))
	{
		  if(WSAGetLastError() != ERROR_IO_PENDING)
		  {
			  delete pSocket;
			  pSocket = NULL;
			  return false;
		  }
	}


	return true;
}

void IOCPServer::UnInitNetWork()
{
	
	//退出通知
	m_bFlagQuit = false;
	//使线程不阻塞GetQueue
	int i =0;
	while(i++ <m_nNumPro)
	{
		PostQueuedCompletionStatus(m_hiocp,0,0,0);
	}
	

	 WSACleanup();
	if(m_sockListen)
	{
		closesocket(m_sockListen);
		m_sockListen = NULL;
	}

	
	auto ite = m_lstThread.begin();
	while(ite != m_lstThread.end())
	{
		if(WAIT_TIMEOUT == WaitForSingleObject(*ite,100))
		{
			TerminateThread(*ite,-1);
		}
		CloseHandle(*ite);
		*ite = NULL;
		ite++;
	}
	
	////POSTACCEPT
	for(int i= 0; i<MAX_RQ_COUNT;i++)
	{
		if(m_aryMySocket[i].pIndex)
		{
			delete m_aryMySocket[i].pIndex;
		   m_aryMySocket[i].pIndex = NULL;
		}

	}

	if(m_aryMySocket)
	{
		delete []m_aryMySocket;
		m_aryMySocket = NULL;
	}



	m_qIndex.UnInitQueue();

	
}

bool IOCPServer::SendData(MySocket *pSockex,char* szbuf,int nLen)
{
	//MySocket *pSock =  m_mapIpToMySocket[lSendIP];
	if(pSockex == nullptr || szbuf == nullptr||nLen <=0)
	{
		return false;
	}
	//发数据前，先发送包大小
	if(send(pSockex->sock,(const char*)&nLen,sizeof(int),0)<0)
	{
		return false;
	}

	if(send(pSockex->sock,szbuf,nLen,0)<0)
	{
		return false;
	}

	return true;
}