#ifndef SERVER_H_
#define SERVER_H_

#include "taskDataStructure.h"
#include <process.h>
#include <Windows.h>
#include "cmdFlag.h"

#define BUF_SIZE 1024000

class transferModule{
private:
	//socket
	WSADATA wsaData;
	SOCKET sockSrv;
	SOCKET sockConn;
	SOCKADDR_IN addrSrv;
	SOCKADDR_IN  addrClient;

public:
	transferModule(){
		if ( WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) != 0 ) {
			exit(EXIT_FAILURE);
		}

		if ( LOBYTE( wsaData.wVersion ) != 2 ||	HIBYTE( wsaData.wVersion ) != 2 ){
			WSACleanup( );
			exit(EXIT_FAILURE);
		}

		sockSrv = socket(AF_INET, SOCK_STREAM, 0);
		// 将INADDR_ANY转换为网络字节序，调用 htonl(long型)或htons(整型)
		addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY); 
		addrSrv.sin_family = AF_INET;
		addrSrv.sin_port = htons(6000);
		
	}

	transferModule(const int & port ){
		if ( WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) != 0 ) {
			exit(EXIT_FAILURE);
		}

		if ( LOBYTE( wsaData.wVersion ) != 2 ||	HIBYTE( wsaData.wVersion ) != 2 ){
			WSACleanup( );
			exit(EXIT_FAILURE);
		}

		sockSrv = socket(AF_INET, SOCK_STREAM, 0);
		// 将INADDR_ANY转换为网络字节序，调用 htonl(long型)或htons(整型)
		addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY); 
		addrSrv.sin_family = AF_INET;
		addrSrv.sin_port = htons(port);
		
	}

	~transferModule(){
		closesocket( sockSrv );
		closesocket( sockConn );
	}

	static bool checkLength( const char * data , const int length ){
		char temp[16] = {0};
		for( int i = 2 ; i < 10 ; ++ i ){
			temp[i-2] = data[i];
		}
		int val = atoi( temp );
		return val == length;
	}

	//waring:this function will release all socket connections
	void releaseAllSocket(){
		WSACleanup();
	}

	int startBind( void ){
		return bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));
	}

	/*SOCKET prepareServerSocket( void ){
		return (this->sockSrv = socket(AF_INET, SOCK_STREAM, 0));
	}*/

	int prepareListen(const int num = 15){
		return listen( sockSrv , num );
	}

	SOCKET getServerSocket( void ){
		return this->sockSrv;
	}

	SOCKET acceptClient( void ){
		int len = sizeof(SOCKADDR);
		sockConn = accept(sockSrv, (SOCKADDR*)&addrClient, &len);
		return sockConn ;
	}

	SOCKET getClientSocket( void ){
		return sockConn;
	}

	char * getClientIp( void ){
		return inet_ntoa(addrClient.sin_addr);
	}

	int closeClient( void ){
		return closesocket( sockConn );
	}

	//send的第一个参数是客户端的socket
	//向客户端发送
	int sendData( char * data , int length){
		return send( sockConn , data , length , 0 );
	}

	//need test
	int sendDataToListener( char * data , int length , string ip , int port){
		SOCKADDR_IN addr;
		addr.sin_addr.S_un.S_addr = inet_addr(ip.c_str());		
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
		connect(sock, (SOCKADDR*)&addr, sizeof(SOCKADDR));
		int status = send( sock , data , length , 0 );
		closesocket(sock);
		return status;
	}

	//recv函数的第一个参数是发送数据的socket
	int recvData( char * dest , int maxLength ){
		int count = 0;
		int num = 0;
		char * pData = dest;

		char * buffer = ( char *)malloc( sizeof(char) * BUF_SIZE );
		while( (num = recv( sockConn , buffer , BUF_SIZE , 0 ) ) > 0 ){
			count += num;
			if( count < maxLength ){
				memcpy( pData , buffer , num );
				pData += num;
			}
			else{
				return SRV_ERROR;
			}
			num = 0;
		}

		free( buffer );
		return count;
	}
};

//==================================================

//全局变量taksManager，负责管理任务
TaskManager taskManager;
//全局变量listener，负责监听
transferModule listener;

//实现运行逻辑的线程
unsigned int __stdcall taskThread( LPVOID lpArg ){
	TaskQueue * curQueue = (TaskQueue *)lpArg;
	Task curTask =*( curQueue->getCurTask());
	
	if( curTask.status == S_TASK_VISITING ){
		return 0;
	}

	curQueue->getCurTask()->status = S_TASK_VISITING;
	
	int length = curTask.length - 32;//!!
	char * data = curTask.data;
	
	char stMac[8] = {0};
	for( int i = 10 ; i < 16 ; ++ i ){
		stMac[i - 10] = data[i];
	}

	char cmdFlag = *data;
	char formatFlag = *(data+1);
	string taskMac( stMac );

	switch(cmdFlag){//need to change status filed in Task
	case WARD_REGISTER:
		puts("r");
		taskManager.getSpecifyQueue( 0)->getCurTask()->status = S_TASK_DONE;
		taskManager.removeFirst( 0 );
		break;
	case WARD_SENDTEXT:
		puts("[server]:msg");
		listener.sendDataToListener( "a" ,2,taskManager.getSpecifyQueue(0)->getCurTargetIP() , 6002);
		taskManager.getSpecifyQueue( 0)->getCurTask()->status = S_TASK_DONE;
		taskManager.removeFirst( 0 );
		break;
	case SRV_QUIT:
		puts("q");
		taskManager.getSpecifyQueue( 0)->getCurTask()->status = S_TASK_DONE;
		taskManager.removeFirst( 0 );
		puts("[server]:quit.");
		exit(0);
		break;
	default:
		puts("ok");
		taskManager.getSpecifyQueue( 0)->getCurTask()->status = S_TASK_DONE;
		taskManager.removeFirst( 0 );
		break;
	}

	curQueue->setStatus( false );
	return 0;
}

//每隔一段时间检查taskManager中有没有需要执行的任务 10ms
unsigned int __stdcall processTasks( LPVOID lpArg ){
	int taskNum = 0;
	int taskQueueNum =0;
	const int handleArrLen = taskManager.getMaxQueueNum();
	HANDLE * handleArr = (HANDLE *)malloc( sizeof(HANDLE) * handleArrLen);

	while( true ){
		taskNum = taskManager.getTaskNum();
		taskQueueNum = taskManager.getTaskQueueNum();

		if( taskNum > 0 ){
			for( int i = 0 ; i < taskQueueNum ; ++ i ){
				if( (taskManager.getSpecifyQueueTaskNum( i ) > 0 && taskManager.getSpecifyQueue(i)->getStatus() == false) ){
					taskManager.getSpecifyQueue(i)->setStatus(true);
					handleArr[i] = (HANDLE)_beginthreadex( NULL , 0 , taskThread , taskManager.getSpecifyQueue(i) , 0 ,NULL);
				}
			}
			WaitForMultipleObjects( taskQueueNum , handleArr , true , INFINITE );
		}
		//wait 10 ms
		Sleep( 3 );
	}
	return 0;
}

//===============================================

//
struct recvDataArg{
	bool isVisting;	//if arg is being visited.wait until done
	SOCKET sock;	//socket that need to call function recv
	char ip[16];	//ip of socket
};

//非阻塞接收数据
unsigned int __stdcall recvData( LPVOID lpArg ){
	SOCKET sock = ((recvDataArg*)lpArg)->sock;
	string ip(((recvDataArg*)lpArg)->ip);
	
	int count = 0;
	int num = 0;
	int limit = 10 * BUF_SIZE;

	char * data =  ( char *)malloc( sizeof(char) * limit);
	char * pData = data;
	char * buffer = ( char *)malloc( sizeof(char) * BUF_SIZE );

	while( (num = recv( sock , buffer , BUF_SIZE , 0 ) ) > 0 ){
		count += num;
		if( count < limit ){
			memcpy( pData , buffer , num );
			pData += num;
		}
		else{
			return SRV_ERROR;
		}
		num = 0;
	}

	closesocket(sock);
	((recvDataArg*)lpArg)->isVisting = false;

	//TODO : need find by mac.check length.ip
	puts("[server]:recv data");
	for( int d = 0 ;d < count;d ++){
		putchar( data[d]);
	}
	printf("[server]:recv done.size = %d\n" , count );

	while( taskManager.getSpecifyQueue(0)->getStatus() || taskManager.isSpecifyQueueFull( 0 ) ){
		Sleep(1);
	}
	
	taskManager.getSpecifyQueue(0)->setStatus(true);
	taskManager.addTask(0,data,count,ip,ip);
	taskManager.getSpecifyQueue(0)->setStatus(false);
	taskManager.printAll();

	//release
	free( data );
	free( buffer );
	
	return 0;
}

//============================================

class Server{
public:
	
public:
	Server(){
		
	}

	~Server(){
		WSACleanup();
	}

	bool registerMac( string ward , string target ){
		return taskManager.registerQueue(ward,target );
	}

	void startListening( void ){
		//char * buf = ( char * )malloc(sizeof(char)*BUF_SIZE);
		recvDataArg arg = {false,INVALID_SOCKET};
		int b = listener.startBind();
		int a = listener.prepareListen();
		
		puts("[server]:start.");
		//监听的主循环
		while( true ){
			//当参数被访问时等待当前访问结束
			while( arg.isVisting ){
				//puts("[server]:waiting");
				Sleep(1);
			}

			//监听
			puts("[server]:listening.");
			arg.sock = listener.acceptClient();
			memcpy( arg.ip , listener.getClientIp() , 16 );
			arg.isVisting = true;
			puts("[server]:connection accepted.");
			
			/*int c = listener.recvData( buf , BUF_SIZE );
			for( int i = 0 ; i < c ; ++ i ){
				putchar(buf[i] );
			}*/

			//start new thread
			if( arg.sock != INVALID_SOCKET ){
				_beginthreadex( NULL , 0 , recvData , &arg , 0 , NULL );
				puts("[server]:A new thread has been created.");
			}
		}

		puts("[server]:stop.");
	}

};

#endif // !SERVER_H_
