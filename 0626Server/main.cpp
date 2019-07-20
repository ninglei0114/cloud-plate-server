#include <iostream>
#include "TCPKernel.h"
using namespace std;

int main()
{
	IKernel *pKernel = new TCPKernel;
	if(pKernel->Open())
		cout<<"server  is running "<<endl;


	system("pause");
	return 0;
}