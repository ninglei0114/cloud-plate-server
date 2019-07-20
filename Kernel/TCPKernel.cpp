#include "TCPKernel.h"
#include "IOCPServer.h"
TCPKernel::TCPKernel()
{
	m_pNet = new IOCPServer(this);
	strcpy_s(m_szSystemPath,MAX_PATH,"d:/disk/");
}

TCPKernel::~TCPKernel()
{
	delete m_pNet;
	m_pNet = NULL;
}

ProtocolMap m_protocolEntries[]=
{    
	{_DEF_PROTOCOL_REGISTER_RQ,&TCPKernel::RegisterRq},
	{_DEF_PROTOCOL_LOGIN_RQ,&TCPKernel::LoginRq},
	{_DEF_PROTOCOL_GETFILELIST_RQ,&TCPKernel::GetFileListRq},
	{_DEF_PROTOCOL_UPLOAD_FILEHEADER_RQ,&TCPKernel::UpLoadFileHeaderRq},
	{_DEF_PROTOCOL_UPLOAD_FILEBLOCK_RQ,&TCPKernel::UpLoadFileBlockRq},
	{_DEF_PROTOCOL_DOWNLOAD_FILEHEADER_RQ,&TCPKernel::DownLoadFileHeaderRq},
	{_DEF_PROTOCOL_DOWNLOAD_FILEBLOCK_RS,&TCPKernel::DownLoadFileBlockRs},
	{_DEF_PROTOCOL_SHARELINK_RQ,&TCPKernel::ShareLinkRq},
	{_DEF_PROTOCOL_GETLINK_RQ,&TCPKernel::GetLinkRq},
	{0,0}

};


void TCPKernel::UpLoadFileBlockRq(MySocket *pSock,char *szbuf)
{
	STRU_UPLOADFILEBLOCK_RQ *psur = (STRU_UPLOADFILEBLOCK_RQ*)szbuf;
	//向文件中写入内容
	STRU_UPLOADFILEBLOCK_RS sur;
	sur.m_fileid = psur->m_fileid;
	sur.m_nType = _DEF_PROTOCOL_UPLOAD_FILEBLOCK_RS;
	
	UpLoadFileInfo *pInfo =  m_mapFileIdToFileInfo[psur->m_fileid];

	sur.m_fileNum =  fwrite(psur->m_szFileContent,sizeof(char),psur->m_fileNum,pInfo->m_pfile);

	pInfo->m_nPos +=sur.m_fileNum;

	if(pInfo->m_nPos == pInfo->m_filesize)
	{
		fclose(pInfo->m_pfile);
		auto ite = m_mapFileIdToFileInfo.find(psur->m_fileid);
		delete pInfo;
		pInfo = NULL;
		m_mapFileIdToFileInfo.erase(ite);
		//更新数据库
		//return;
	}

	m_pNet->SendData(pSock,(char*)&sur,sizeof(sur));


}

void TCPKernel::UpLoadFileHeaderRq(MySocket *pSock,char *szbuf)
{
	STRU_UPLOADFILEHEADER_RQ *psur = (STRU_UPLOADFILEHEADER_RQ*)szbuf;
	char szsql[_DEF_SQLLEN] = {0};
	char szPath[MAX_PATH] = {0};
	list<string> lststr;
	STRU_UPLOADFILEHEADER_RS  sur;
	sur.m_nPos  = 0;
	sur.m_nType = _DEF_PROTOCOL_UPLOAD_FILEHEADER_RS;
	strcpy_s(sur.m_szFileName,sizeof(sur.m_szFileName),psur->m_szFileName);
	strcpy_s(sur.m_szMD5,sizeof(sur.m_szMD5),psur->m_szMD5);
	sur.m_szResult = _uploadfileheader_fail;
	sur.m_UserId = psur->m_UserId;

	//校验文件信息
	//遍历MAP
	//判断是不是断点续传
	auto  ite = m_mapFileIdToFileInfo.begin();
	while( ite != m_mapFileIdToFileInfo.end())
	{
		if(0 == strcmp(ite->second->m_szFilemd5,psur->m_szMD5) &&
			0 == strcmp(ite->second->m_szFileName,psur->m_szFileName)  &&
			ite->second->m_userid == psur->m_UserId
			)
		{
			sur.m_nPos = ite->second->m_nPos;
			sur.m_fileid = ite->second->m_fileid;
			sur.m_szResult = _uploadfileheader_continue;
			m_pNet->SendData(pSock,(char*)&sur,sizeof(sur));
			return;
		}
		ite++;
	}
	
		//判断服务器中是否存在，如果存在，则秒传成功
	sprintf_s(szsql,"select f_id,f_count from file where f_md5 = '%s' and f_name = '%s' ",psur->m_szMD5,psur->m_szFileName);
	
	m_sql.SelectMySql(szsql,2,lststr);
	if(lststr.size()  >0)
	{
		//
		string strFileid = lststr.front();
		lststr.pop_front();
		string strFileCount = lststr.front();
		lststr.pop_front();

		sprintf_s(szsql,"insert into user_file(u_id,f_id) values(%lld,%lld);",psur->m_UserId,_atoi64(strFileid.c_str()));
		m_sql.UpdateMySql(szsql);

		INT64 filecount = _atoi64(strFileCount.c_str());

		sprintf_s(szsql,"update file set f_count = %lld where f_id =%lld ",++filecount,_atoi64(strFileid.c_str()));
		m_sql.UpdateMySql(szsql);

		sur.m_szResult   =_uploadfileheader_transfer;

		m_pNet->SendData(pSock,(char*)&sur,sizeof(sur));

		return;
	}
	//将文件信息存储到数据库中
	
	sprintf_s(szPath,"%s%lld/%s",m_szSystemPath,psur->m_UserId,psur->m_szFileName);
	sprintf_s(szsql,"insert into file(f_name,f_uploadtime,f_size,f_md5,f_count,f_type,f_path) values('%s','%s',%lld,'%s',1,'%s','%s')",
		psur->m_szFileName,psur->m_szFileUpLoadTime,psur->m_fileSize,psur->m_szMD5,psur->m_szFileType,szPath);

	if(m_sql.UpdateMySql(szsql))
	{
		list<string> lststr;
		//获取文件ID  ？？ 
		INT64 fileid;
		sprintf_s(szsql,"select f_id  from file where f_name = '%s' and f_md5 ='%s'",psur->m_szFileName,psur->m_szMD5);
		m_sql.SelectMySql(szsql,1,lststr);
		if(lststr.size() >0)
		{
			string strFileid  = lststr.front();
			lststr.pop_front();

			fileid = _atoi64(strFileid.c_str());

			sur.m_fileid =  fileid;
			sur.m_szResult = _uploadfileheader_success;
		}
		//将用户与文件映射
		sprintf_s(szsql,"insert into user_file(u_id,f_id) values(%lld,%lld)",psur->m_UserId,fileid);
		m_sql.UpdateMySql(szsql);
     	//在当前用户的目录下 ，创建文件
		FILE *pFile =  fopen(szPath,"wb");

		// 文件id   -- 文件指针  文件名 文件MD5 文件大小 文件位置 用户id 
		UpLoadFileInfo *pInfo = new UpLoadFileInfo;
		pInfo->m_fileid = fileid;
		pInfo->m_filesize = psur->m_fileSize;
		pInfo->m_nPos = 0;
		pInfo->m_pfile = pFile;
		strcpy_s(pInfo->m_szFilemd5,_DEF_SIZE,psur->m_szMD5);
		strcpy_s(pInfo->m_szFileName,_DEF_SIZE,psur->m_szFileName);
		pInfo->m_userid = psur->m_UserId;

		m_mapFileIdToFileInfo[pInfo->m_fileid] = pInfo;

	}
	
	//发送回复
	m_pNet->SendData(pSock,(char*)&sur,sizeof(sur));

}

void TCPKernel::DownLoadFileHeaderRq(MySocket *pSock,char *szbuf)
{
	// 1. 拆包
	STRU_DOWNLOADFILEHEADER_RQ *pSdfr =  (STRU_DOWNLOADFILEHEADER_RQ *)szbuf;

	STRU_DOWNLOADFILEHEADER_RS sdfr ;
	list<string> lstInfo;

	DownLoadFileInfo * pNewInfo  = 0 ;

	sdfr.m_nType = _DEF_PROTOCOL_DOWNLOAD_FILEHEADER_RS;
	strcpy_s ( sdfr.m_szFileName ,_DEF_SIZE ,pSdfr->m_szFileName);

	 sdfr.m_fileid = 0 ;
	 sdfr.m_szResult = _downloadfileheader_fail;

	// 2. 根据文件名查找数据库 , 结果 0 ,1   查某人 某文件的.... 文件id  文件路径 文件大小
	 char szSql[_DEF_SQLLEN] = ""; 

	 //select f_id , f_size , f_path from file inner join user_file on file.f_id = user_file.f_id 
//and user_file.u_id = xx and file.f_name = xx;
 	 sprintf_s( szSql , "select user_file.f_id , f_size , f_path , f_md5 from file inner join user_file on file.f_id = user_file.f_id \
						and user_file.u_id = %lld and file.f_name = '%s';" , pSdfr->m_UserId ,pSdfr->m_szFileName );

	if( m_sql .SelectMySql( szSql , 4 , lstInfo) )
	{
		if( lstInfo .size() > 0)
		{
			string strFileMD5 = lstInfo.back();
			lstInfo.pop_back();
			string strPath = lstInfo.back();
			lstInfo.pop_back();
			string strSize = lstInfo.back();
			lstInfo.pop_back();
			string strFileId = lstInfo.back();
			lstInfo.pop_back();

			sdfr.m_szResult = _downloadfileheader_success ;
			
			// 3. 保存信息
			pNewInfo->m_userid = pSdfr->m_UserId;
			strcpy_s ( pNewInfo->m_szFileName ,_DEF_SIZE , pSdfr->m_szFileName );
			strcpy_s ( pNewInfo->m_szFilemd5  ,_DEF_SIZE , strFileMD5.c_str() );

			pNewInfo->m_nPos = 0;
			pNewInfo->m_filesize = _atoi64( strSize.c_str());
			pNewInfo->m_fileid =  _atoi64( strFileId.c_str());
			pNewInfo->m_pfile = 0; 

			sdfr.m_fileid = pNewInfo->m_fileid ;

			fopen_s( &pNewInfo->m_pfile, strPath.c_str() , "rb");

			m_mapFileIdToDownLoadFileInfo[pNewInfo->m_fileid] = pNewInfo ;
		}
	}

	// 4. 发送 回复包
	m_pNet->SendData( pSock , (char*)&sdfr , sizeof(sdfr) );

	if(sdfr.m_szResult == _downloadfileheader_success)
	{ //成功, 发送文件第一帧
		DownLoadFileBlockRq(pSock , (char*)pNewInfo);
	}

}

void  TCPKernel::DownLoadFileBlockRq(MySocket *pSock,char *szbuf)
{
	DownLoadFileInfo * pInfo  = (DownLoadFileInfo * )szbuf;
	//发送要下载的文件块
	STRU_DOWNLOADFILEBLOCK_RQ rq;
	rq.m_fileid = pInfo->m_fileid;
	rq.m_nType = _DEF_PROTOCOL_DOWNLOAD_FILEBLOCK_RQ;
	rq.m_UserId = pInfo->m_userid;
	
	rq.m_fileNum = fread(rq.m_szFileContent , sizeof(char) , _DEF_FILECONTENTLEN , pInfo->m_pfile);
	// pInfo->m_nPos =   考虑只有回复成功, 才能移动位置, 否则移动了失败了, 就回不去了

	m_pNet->SendData(pSock ,(char*)&rq , sizeof(rq) );

}


 void TCPKernel::DownLoadFileBlockRs(MySocket *pSock,char *szbuf)
 {
	 //拆包 得到rs的结果
	 STRU_DOWNLOADFILEBLOCK_RS *rs = ( STRU_DOWNLOADFILEBLOCK_RS *) szbuf;
	 //rs->m_szResult
	 //rs->m_fileNum
	 //rs->m_fileid

	 STRU_DOWNLOADFILEBLOCK_RQ rq;
	 rq.m_nType = _DEF_PROTOCOL_DOWNLOAD_FILEBLOCK_RQ;
	 rq.m_fileNum = 0 ; // todo
	 //rq.m_szFileContent  //todo
	 rq.m_fileid = rs->m_fileid;

	 auto ite =  m_mapFileIdToDownLoadFileInfo.find( rs->m_fileid );
	 if( ite == m_mapFileIdToDownLoadFileInfo.end() ) return;
	 
	 DownLoadFileInfo *pInfo  = ite ->second ;

	 rq.m_UserId = pInfo->m_userid;

	 if( rs->m_szResult == _downloadfileblock_success )
	 {//结果是真 pos移动 
		 pInfo->m_nPos += rs->m_fileNum;
		 if( pInfo->m_nPos >= pInfo->m_filesize ) //到达文件尾 , 根据id 找到info  关闭文件指针, 删除信息节点
		 {
			 fclose(pInfo->m_pfile);
			 delete ite->second; ite->second = 0;
			 m_mapFileIdToDownLoadFileInfo.erase(ite);
			 return;
		 }
		 else //没有到尾端 , 读文件 写rq
		 { 
			  rq.m_fileNum = fread( rq.m_szFileContent , sizeof(char) , _DEF_FILECONTENTLEN , pInfo->m_pfile );
		 }
	 }else//结果是假 , 回到上一次的位置 , 读文件写rq
	 {
		 _fseeki64( pInfo->m_pfile , pInfo->m_nPos , SEEK_SET); //从头偏移 pos

		 rq.m_fileNum = fread( rq.m_szFileContent , sizeof(char) , _DEF_FILECONTENTLEN , pInfo->m_pfile );
	 }

	 //发送 下载文件块请求
	 m_pNet->SendData(pSock, (char*)&rq , sizeof(rq));
 }


 void TCPKernel::ShareLinkRq(MySocket *pSock,char *szbuf)
 {
	 //拆包 结合 u_id  f_name 查找数据库 , 查某人某文件 文件id 
	 STRU_SHARELINK_RQ *rq = (STRU_SHARELINK_RQ*) szbuf;
	// rq->m_UserId
	// rq->m_szFileName 
	 STRU_SHARELINK_RS rs;
	 rs.m_nType = _DEF_PROTOCOL_SHARELINK_RS ;
	 rs.m_code = 0 ;  // todo
	 rs.m_szResult = _share_link_fail ;  //todo 
	 strcpy_s ( rs.m_szFileName , _DEF_SIZE , rq->m_szFileName);
	 list<string> lststr;
	 char szSql[_DEF_SQLLEN] = "";
	 sprintf_s( szSql , "select file.f_id from file inner join user_file on file.f_id = user_file.f_id \
						and user_file.u_id = %lld and file.f_name = '%s'" ,rq->m_UserId ,rq->m_szFileName );
	 if( m_sql .SelectMySql( szSql , 1 ,lststr ) )
	 {
		 if( lststr .size() > 0 )
		 {//有文件
			 rs.m_szResult = _share_link_success ;

			 string strFileID = lststr.front();
			 lststr.pop_front();
			//有文件  分享过 , 没有分享过   查user_share_file 表 查code
			 ZeroMemory(szSql , _DEF_SQLLEN);
			 sprintf_s ( szSql , "select code from user_share_file where u_id = %lld and f_id = %lld", rq->m_UserId , _atoi64(strFileID.c_str() ) );
			 list<string > lstCode;
			 if( m_sql.SelectMySql ( szSql , 1 , lstCode ))
			 {
				  if( lstCode.size() == 0 )
				  { //没有分享过//没分享过  获取分享码 , 然后插入数据库
					  rs.m_code = TCPKernel::GetShareLinkCode( rq->m_UserId );
					  ZeroMemory(szSql , _DEF_SQLLEN);
					  sprintf_s ( szSql , "insert into user_share_file ( u_id , f_id , code ) values( %lld , %lld ,%lld)",rq->m_UserId,_atoi64(strFileID.c_str() ), rs.m_code   );
					  m_sql.UpdateMySql( szSql );
				  }
				  else{ //分享过  直接查表获得分享码 
					  rs.m_code = _atoi64( lstCode.front().c_str() );
					  lstCode.pop_front();
				  }
			 }
		 }else//没有文件 失败 
		 {
			  rs.m_szResult = _share_link_fail ; 
		 }
	 }

	 //写rs 发送客户端
	 m_pNet->SendData ( pSock , (char *)&rs , sizeof(rs));
 }

void TCPKernel::GetLinkRq(MySocket *pSock,char *szbuf)
{
	//拆包 根据rq的code  查数据库 user_share_file 查 u_id f_id
	STRU_GETLINK_RQ* rq = (STRU_GETLINK_RQ*)szbuf;
	//rq->m_UserId
	//rq->m_code
	char szSql[_DEF_SQLLEN] = "";
	list<string > lstID;
	STRU_GETLINK_RS rs;
	rs.m_nType = _DEF_PROTOCOL_GETLINK_RS ;
	rs.m_szResult = _getlink_fail ;
	string strFileID;
	sprintf_s(szSql , "select u_id , f_id from user_share_file where code = %lld" ,rq->m_code );
	if( m_sql .SelectMySql ( szSql , 1 , lstID))
	{
		if( lstID.size() >0 )
		{// 如果有code 对应的 
			//u_id 是不是自己  是自己返回  _link_myself
			string strUserID = lstID.front();
			lstID.pop_front();
			strFileID = lstID.front();
			lstID.pop_front();

			if( _atoi64( strUserID.c_str() ) == rq->m_UserId  )
			{
				rs.m_szResult = _link_myself ;
			}else
			{//不是自己 看是否是提取过   提取过 _link_getted  查 user_file
				ZeroMemory(szSql , _DEF_SQLLEN);
				sprintf_s(szSql , "select f_id from user_file where u_id = %lld and f_id = %lld" ,rq->m_UserId ,_atoi64(strFileID.c_str())  );
				list<string> lstFID;
				if(  m_sql.SelectMySql(szSql , 1 , lstFID))
				{
					if( lstFID .size() > 0 )//提取过
					{
						rs.m_szResult = _link_getted;
					}else
					{//没提取过  插入数据库, 然后文件引用计数+1 
						ZeroMemory(szSql , _DEF_SQLLEN);
						sprintf_s(szSql , "insert into user_file( u_id , f_id) values ( %lld , %lld ) " ,rq->m_UserId ,_atoi64(strFileID.c_str())  );
						m_sql.UpdateMySql(szSql);
						rs.m_szResult = _getlink_success ;

						list<string > lstCount;
						//文件引用计数+1
						ZeroMemory(szSql , _DEF_SQLLEN);
						sprintf_s(szSql , "select f_count from file where f_id = %lld" ,_atoi64(strFileID.c_str())  );
						m_sql.SelectMySql(szSql , 1,lstCount );
						INT64 nCount = _atoi64( lstCount.front().c_str());
						nCount+= 1;
						ZeroMemory(szSql , _DEF_SQLLEN);
						sprintf_s(szSql , "update user_file set f_count = %lld  where f_id = %lld" ,nCount ,_atoi64(strFileID.c_str())  );
						m_sql.UpdateMySql(szSql);
					}
				}
			}
		}
		//没有 对应的  _getlink_fail
	}
	//写 rs  发送
	m_pNet->SendData(pSock , (char*)&rs , sizeof(rs) );

	if( rs.m_szResult == _getlink_success)
	{
		//发送 提取文件的信息
		GetSelectedFileInfoRq(pSock, rq->m_UserId  ,_atoi64(strFileID.c_str())  );
	}
}
 void TCPKernel::GetSelectedFileInfoRq(MySocket *pSock,INT64 u_id , INT64 f_id)
{
	char szsql[_DEF_SQLLEN]= {0};
	STRU_GETFILELIST_RS sgr;
	list<string> lststr;
	sgr.m_nType = _DEF_PROTOCOL_GETFILELIST_RS;
	sprintf(szsql,"select f_name,f_uploadtime,f_size,f_type from myview where u_id = %lld and f_id = %lld",u_id , f_id);
	m_sql.SelectMySql(szsql,4,lststr);
	int i = 0;
	while(lststr.size() >0)
	{
		string strfilename = lststr.front();
		lststr.pop_front();
		string strfileuploadtime = lststr.front();
		lststr.pop_front();
		string strfilesize = lststr.front();
		lststr.pop_front();
		string strfiletype = lststr.front();
		lststr.pop_front();

		strcpy_s(sgr.m_aryFileInfo[i].m_fileName,_DEF_SIZE,strfilename.c_str());
		strcpy_s(sgr.m_aryFileInfo[i].m_fileUpLoadTime,_DEF_SIZE,strfileuploadtime.c_str());
		strcpy_s(sgr.m_aryFileInfo[i].m_fileType,_DEF_SIZE,strfiletype.c_str());
		
		sgr.m_aryFileInfo[i].m_fileSize = _atoi64(strfilesize.c_str());
		i++;

		if(lststr.size() == 0 || i == _DEF_FILENUM)
		{
			sgr.m_nNum = i;
			m_pNet->SendData(pSock,(char*)&sgr,sizeof(sgr));
			i=0;
		}
	}
}

void TCPKernel::GetFileListRq(MySocket *pSock,char *szbuf)
{
	STRU_GETFILELIST_RQ *psgr = (STRU_GETFILELIST_RQ*)szbuf;
	char szsql[_DEF_SQLLEN]= {0};
	STRU_GETFILELIST_RS sgr;
	list<string> lststr;
	sgr.m_nType = _DEF_PROTOCOL_GETFILELIST_RS;
	sprintf(szsql,"select f_name,f_uploadtime,f_size,f_type from myview where u_id = %lld",psgr->m_UserId);
	m_sql.SelectMySql(szsql,4,lststr);
	int i = 0;
	while(lststr.size() >0)
	{
		string strfilename = lststr.front();
		lststr.pop_front();
		string strfileuploadtime = lststr.front();
		lststr.pop_front();
		string strfilesize = lststr.front();
		lststr.pop_front();
		string strfiletype = lststr.front();
		lststr.pop_front();

		strcpy_s(sgr.m_aryFileInfo[i].m_fileName,_DEF_SIZE,strfilename.c_str());
		strcpy_s(sgr.m_aryFileInfo[i].m_fileUpLoadTime,_DEF_SIZE,strfileuploadtime.c_str());
		strcpy_s(sgr.m_aryFileInfo[i].m_fileType,_DEF_SIZE,strfiletype.c_str());
		
		sgr.m_aryFileInfo[i].m_fileSize = _atoi64(strfilesize.c_str());
		i++;

		if(lststr.size() == 0 || i == _DEF_FILENUM)
		{
			sgr.m_nNum = i;
			m_pNet->SendData(pSock,(char*)&sgr,sizeof(sgr));
			i=0;
		}


	}
	//查找当前人上传过的文件

}

void TCPKernel::RegisterRq(MySocket *pSock,char *szbuf)
{
	 STRU_REGISTER_RQ *psrr = (STRU_REGISTER_RQ*)szbuf;
	 char szsql[_DEF_SQLLEN] = {0};
	 char szpath[MAX_PATH]= {0};
	 STRU_REGISTER_RS srr;
	 srr.m_nType = _DEF_PROTOCOL_REGISTER_RS;
	 srr.m_szResult = _register_fail;
	 //将客户端信息写入数据库中--
	 sprintf(szsql,"insert into user values(%lld,'%s')",psrr->m_UserId,psrr->m_szPassword);

	if( m_sql.UpdateMySql(szsql))
	{
		 srr.m_szResult =  _register_success;
		 //在指定目录下创建文件f:/disk/15046691259
		 sprintf_s(szpath,"%s%lld",m_szSystemPath,psrr->m_UserId);
		 CreateDirectory(szpath,NULL);
	}
	
	m_pNet->SendData(pSock,(char*)&srr,sizeof(srr));
	


}

void TCPKernel::LoginRq(MySocket *pSock,char *szbuf)
{
	   STRU_LOGIN_RQ *pslr = (STRU_LOGIN_RQ*)szbuf;
	   char szsql[_DEF_SQLLEN] = {0};
	   STRU_LOGIN_RS slr;
	   slr.m_nType = _DEF_PROTOCOL_LOGIN_RS;
	   slr.m_szResult = _login_fail;
	   list<string> lststr;
	   sprintf_s(szsql,"select u_password from user where u_id = %lld",pslr->m_UserId);

	   m_sql.SelectMySql(szsql,1,lststr);
	   if(lststr.size() >0)
	   {
		   string strPassword = lststr.front();
		   lststr.pop_front();

		   if(0 == strcmp(pslr->m_szPassword,strPassword.c_str()))
		   {
			   slr.m_szResult = _login_success;
		   }
	   }
	   //校验信息

	   m_pNet->SendData(pSock,(char*)&slr,sizeof(slr));
}
	

bool TCPKernel::Open()
{
	if(!m_sql.ConnectMySql("localhost","root","123456","0626disk"))
		return false;

	if(!m_pNet->InitNetWork())
		return false;
	return true;
}
void TCPKernel::Close()
{
	m_sql.DisConnect();
	m_pNet->UnInitNetWork();
}

bool TCPKernel::DealData(MySocket *pSock,char *szbuf)
{
	//校验包类型
	PackType *pType = (PackType *)szbuf;
	int i =0;
	while(1)
	{
		if(*pType == m_protocolEntries[i].m_nType)
		{
			(this->*m_protocolEntries[i].m_pfun)(pSock,szbuf);
			break;
		}
		else if( m_protocolEntries[i].m_nType  == 0&&m_protocolEntries[i].m_pfun ==0)
			break;
		i++;
	}


	
	//遍历结构体数组
	//switch (*pType)
	//{
	//case _DEF_PROTOCOL_REGISTER_RQ://
	//	RegisterRq();
	//	break;
	//case _DEF_PROTOCOL_LOGIN_RQ://
	//	break;
	//default:
	//	break;
	//}

	//处理

	return true;
}