

#include "MyLock.h"

template<class T>
class MyQueue
{
public:
	MyQueue():m_lReadPos(0),m_lWritePos(0),m_nLen(0)
	{}
	~MyQueue()
	{}

	//1.初始化队列
	bool InitQueue(int nlen)
	{
		if(nlen <=0)return false;

		m_queue = new T[nlen];
		m_nLen = nlen;
		for(int i = 0;i <nlen;i++)
		{
			m_queue[i] = 0;
		}

		//ZeroMemory();

		return true;
	}
	
	//2.销毁队列
	void UnInitQueue()
	{
		for(int i = 0; i < m_nLen;i++)
		{
			if(m_queue[i])
			{
				delete m_queue[i];
				m_queue[i] = NULL;
			}
		}


		delete []m_queue;
		m_queue = NULL;
	}

	//3.push
	bool push(T node)
	{
		m_WriteLock.Lock();
		if(m_queue[m_lWritePos] != NULL)
		{
			  return false;
		}

		m_queue[m_lWritePos] = node;
		m_lWritePos = (m_lWritePos+1)%m_nLen;
		m_WriteLock.UnLock();
		return true;
	}


	//4.pop
	void pop(T& node )
	{
		m_ReadLock.Lock();
		if(m_queue[m_lReadPos] == NULL)
		{
			  node = NULL;
			  return ;
		}
		node = m_queue[m_lReadPos];
		m_queue[m_lReadPos] = NULL;
		m_lReadPos = (m_lReadPos+1)%m_nLen;
		m_ReadLock.UnLock();
	}


	//5.判断是否为空
	bool empty()
	{
		if(m_lWritePos == m_lReadPos && m_queue[m_lWritePos] == NULL)
		{
			return true;
		}

		return false;
	}
 
private:
	T* m_queue;
	long m_lReadPos;
	long m_lWritePos;
	CMyLock m_ReadLock;
	CMyLock m_WriteLock;
	int  m_nLen;
};

