#ifndef _PACKDEF_H
#define _PACKDEF_H

#define SERVER_IP   "127.0.0.1"
#define _DEFAULT_PORT  1234

#define _DEFAULT_SIZE   1024


#define MAX_RQ_COUNT 2000

//协议
#define _DEF_PROTOCOL_BASE  10

#define _DEF_PROTOCOL_REGISTER_RQ _DEF_PROTOCOL_BASE +1
#define _DEF_PROTOCOL_REGISTER_RS _DEF_PROTOCOL_BASE +2


#define _DEF_PROTOCOL_LOGIN_RQ _DEF_PROTOCOL_BASE +3
#define _DEF_PROTOCOL_LOGIN_RS _DEF_PROTOCOL_BASE +4

#define _DEF_PROTOCOL_GETFILELIST_RQ _DEF_PROTOCOL_BASE +5
#define _DEF_PROTOCOL_GETFILELIST_RS _DEF_PROTOCOL_BASE +6

#define _DEF_PROTOCOL_UPLOAD_FILEHEADER_RQ _DEF_PROTOCOL_BASE +7
#define _DEF_PROTOCOL_UPLOAD_FILEHEADER_RS _DEF_PROTOCOL_BASE +8

#define _DEF_PROTOCOL_UPLOAD_FILEBLOCK_RQ _DEF_PROTOCOL_BASE +9
#define _DEF_PROTOCOL_UPLOAD_FILEBLOCK_RS _DEF_PROTOCOL_BASE +10


#define _DEF_PROTOCOL_DOWNLOAD_FILEHEADER_RQ _DEF_PROTOCOL_BASE +11
#define _DEF_PROTOCOL_DOWNLOAD_FILEHEADER_RS _DEF_PROTOCOL_BASE +12

#define _DEF_PROTOCOL_DOWNLOAD_FILEBLOCK_RQ _DEF_PROTOCOL_BASE +13
#define _DEF_PROTOCOL_DOWNLOAD_FILEBLOCK_RS _DEF_PROTOCOL_BASE +14



#define _DEF_PROTOCOL_SHARELINK_RQ _DEF_PROTOCOL_BASE +15
#define _DEF_PROTOCOL_SHARELINK_RS _DEF_PROTOCOL_BASE +16


#define _DEF_PROTOCOL_GETLINK_RQ _DEF_PROTOCOL_BASE +17
#define _DEF_PROTOCOL_GETLINK_RS _DEF_PROTOCOL_BASE +18

#define _DEF_PROTOCOL_ADDFRIEND_RQ _DEF_PROTOCOL_BASE +19
#define _DEF_PROTOCOL_ADDFRIEND_RS _DEF_PROTOCOL_BASE +20

#define _DEF_PROTOCOL_DELETEFRIEND_RQ _DEF_PROTOCOL_BASE +21
#define _DEF_PROTOCOL_DELETEFRIEND_RS _DEF_PROTOCOL_BASE +22


#define _DEF_PROTOCOL_DELETEFILE_RQ _DEF_PROTOCOL_BASE +23
#define _DEF_PROTOCOL_DELETEFILE_RS _DEF_PROTOCOL_BASE +24

#define _DEF_PROTOCOL_SEARCHFILE_RQ _DEF_PROTOCOL_BASE +25
#define _DEF_PROTOCOL_SEARCHFILE_RS _DEF_PROTOCOL_BASE +26

#define _DEF_PROTOCOL_PERSONLETTER_RQ _DEF_PROTOCOL_BASE +27
#define _DEF_PROTOCOL_PERSONLETTER_RS _DEF_PROTOCOL_BASE +28

#define _DEF_PROTOCOL_QUIT_RQ _DEF_PROTOCOL_BASE +29
#define _DEF_PROTOCOL_QUIT_RS _DEF_PROTOCOL_BASE + 30

//消息
#define UM_LOGINMSG   WM_USER +1
#define UM_FILELISTMSG  WM_USER + 2
#define UM_SHARE_LINK	WM_USER + 3



//结果值
#define _register_fail   0
#define _register_success  1

#define _login_fail   0
#define _login_success  1

#define _uploadfileheader_fail  0
#define _uploadfileheader_success 1
#define _uploadfileheader_continue 2
#define _uploadfileheader_transfer 3

#define _downloadfileheader_fail  0
#define _downloadfileheader_success	1

#define _downloadfileblock_fail  0
#define _downloadfileblock_success	1

#define _share_link_fail		0
#define _share_link_success		1

#define _getlink_fail		0
#define	_getlink_success	1
#define _link_getted		2
#define _link_myself		3

//边界值
#define _DEF_SIZE   45
#define _DEF_FILENUM  20

#define _DEF_FILECONTENTLEN   500

#define _DEF_SQLLEN      300

//协议包
typedef char PackType;
//注册、登录
typedef struct STRU_REGISTER_RQ
{
	PackType m_nType;
	INT64    m_UserId;
	char     m_szPassword[_DEF_SIZE];
	INT64    m_HostIp;
}STRU_LOGIN_RQ;

typedef struct STRU_REGISTER_RS
{
	PackType m_nType;
	char     m_szResult;
}STRU_LOGIN_RS;

//获取文件列表请求

struct STRU_GETFILELIST_RQ
{
	PackType m_nType;
	INT64    m_UserId;
};

struct FILEINFO
{
	 char m_fileName[_DEF_SIZE];
	 char m_fileType[_DEF_SIZE];
     char m_fileUpLoadTime[_DEF_SIZE];
	 INT64 m_fileSize;
};

struct STRU_GETFILELIST_RS
{
	PackType m_nType;
    FILEINFO m_aryFileInfo[_DEF_FILENUM];
	char     m_nNum;
};
//上传文件头
struct STRU_UPLOADFILEHEADER_RQ
{
	PackType m_nType;
    INT64    m_UserId;
	char     m_szFileName[_DEF_SIZE];
	char     m_szMD5[_DEF_SIZE];  //生成文件唯一标识-校验文件是否完全相同
	INT64    m_fileSize;
	char     m_szFileUpLoadTime[_DEF_SIZE];
	char     m_szFileType[_DEF_SIZE];

};

struct STRU_UPLOADFILEHEADER_RS
{
	PackType m_nType;
    INT64    m_UserId;
	INT64    m_fileid;
	char     m_szFileName[_DEF_SIZE];
	char     m_szMD5[_DEF_SIZE];  //生成文件唯一标识-校验文件是否完全相同
	char     m_szResult;
	INT64    m_nPos;   //断点续传
};

//上传文件块
typedef struct STRU_UPLOADFILEBLOCK_RQ
{
	PackType m_nType;
    INT64    m_UserId;
	INT64    m_fileid;
	char     m_szFileContent[_DEF_FILECONTENTLEN];
	INT32    m_fileNum;

}STRU_DOWNLOADFILEBLOCK_RQ;

typedef struct STRU_UPLOADFILEBLOCK_RS
{
	PackType m_nType;
	INT64    m_fileid;
	INT32    m_fileNum;
	char	 m_szResult ;

}STRU_DOWNLOADFILEBLOCK_RS;

//下载文件请求
struct STRU_DOWNLOADFILEHEADER_RQ
{
	PackType m_nType;
    INT64    m_UserId;
	char     m_szFileName[_DEF_SIZE];
	//char     m_szMD5[_DEF_SIZE];  //生成文件唯一标识-校验文件是否完全相同
};
struct STRU_DOWNLOADFILEHEADER_RS
{
	PackType m_nType;
	char     m_szResult;
	INT64	 m_fileid;
	char	m_szFileName[_DEF_SIZE];
	//int      m_nPos;
};


//分享连接
struct STRU_SHARELINK_RQ
{
	PackType m_nType;
    INT64    m_UserId;
	char     m_szFileName[_DEF_SIZE];
	//char     m_szMD5[_DEF_SIZE];  //生成文件唯一标识-校验文件是否完全相同
};

struct STRU_SHARELINK_RS
{
	PackType m_nType;
	char     m_szFileName[_DEF_SIZE];
	//char     m_szMD5[_DEF_SIZE];  //生成文件唯一标识-校验文件是否完全相同
	INT32    m_code;
	char	 m_szResult ; // 共享结果   fail 0 success 1
};

//提取连接
struct STRU_GETLINK_RQ
{
	PackType m_nType;
    INT64    m_UserId;
	INT32    m_code; 
};
struct STRU_GETLINK_RS
{
	PackType m_nType;
    char     m_szResult; 
};

//删除文件
struct STRU_DELETEFILE_RQ
{
	PackType m_nType;
	INT64    m_UserId;
	char     m_szFileName[_DEF_SIZE];
	char     m_szMD5[_DEF_SIZE];  //生成文件唯一标识-校验文件是否完全相同
};

struct STRU_DELETEFILE_RS
{
	PackType m_nType;
	char     m_szResult;
};
#endif