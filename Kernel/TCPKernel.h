#ifndef _TCPKERNEL_H

#define _TCPKERNEL_H

#include "IKernel.h"
#include "CMySql.h"
#include "Packdef.h"
#include <map>
#include<time.h>
// 文件id   -- 文件指针  文件名 文件MD5 文件大小 文件位置 用户id 
struct UpLoadFileInfo
{
	INT64 m_fileid;
	FILE * m_pfile;
	char m_szFileName[_DEF_SIZE];
	char m_szFilemd5[_DEF_SIZE];
	INT64 m_filesize;
	INT64 m_nPos;
	INT64 m_userid;
};

typedef UpLoadFileInfo DownLoadFileInfo;


class TCPKernel;
typedef void (TCPKernel:: *PFUN)(MySocket *,char *);
struct ProtocolMap
{
	PackType m_nType;
	PFUN    m_pfun;
};

class TCPKernel :public IKernel
{
public:
	TCPKernel();
	virtual ~TCPKernel();
	
public:
	 bool Open();
	 void Close();
	 bool DealData(MySocket *pSock,char *szbuf);
public:
	 void RegisterRq(MySocket *pSock,char *szbuf);
	 void LoginRq(MySocket *pSock,char *szbuf);
	 void GetFileListRq(MySocket *pSock,char *szbuf);
	 void GetSelectedFileInfoRq(MySocket *pSock,INT64 u_id , INT64 f_id);
	 void UpLoadFileHeaderRq(MySocket *pSock,char *szbuf);
	 void UpLoadFileBlockRq(MySocket *pSock,char *szbuf);
	 void DownLoadFileHeaderRq(MySocket *pSock,char *szbuf);
	 void DownLoadFileBlockRq(MySocket *pSock,char *szbuf);
	 void DownLoadFileBlockRs(MySocket *pSock,char *szbuf);
	 void ShareLinkRq(MySocket *pSock,char *szbuf);
	 void GetLinkRq(MySocket *pSock,char *szbuf);
public:
	 CMySql m_sql;
	 char m_szSystemPath[MAX_PATH];
	 std::map<INT64,UpLoadFileInfo*>  m_mapFileIdToFileInfo;
	 std::map<INT64,DownLoadFileInfo*>  m_mapFileIdToDownLoadFileInfo;
//获得分享码	 
	static INT64 GetShareLinkCode( INT64 nUserID)
	{
		DWORD dwCurrentTime = (DWORD)time(NULL);
		return dwCurrentTime*nUserID;
	}
};







#endif