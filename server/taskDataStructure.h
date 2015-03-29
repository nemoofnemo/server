#ifndef TASK_H
#define TASK_H
#define DEV_DEBUG

/********************************************************************************
**
**Function:数据结构定义文件
**Detail  :定义任务的存储方式，操作。并对相关方法进行实现。
**
**Author  :袁羿
**Date    :2015/3/29
**
********************************************************************************/

#include <list>
#include <utility>
#include <string>
#include <iostream>
#include <cstdlib>
#include <Winsock2.h>
#pragma comment(lib, "ws2_32.lib") 

#define SRV_ERROR -1
#define SRV_MAX_TASK_QUEUE_LENGTH 20
#define SRV_MAX_TASK_QUEUE_NUM 6

#define S_TASK_VISITING 2
#define S_TASK_DONE 1
#define S_TASK_STANDBY 0

using std::pair;
using std::string;
using std::list;
using std::iterator;
using std::cout;
using std::endl;

#ifdef DEV_DEBUG

#endif

/********************************************************************************
**
**Function:任务存储结构定义及实现
**Detail  :定义任务的存储结构。
**         
**         status为当前任务状态：
**         S_TASK_VISITING 2 正在访问
**         S_TASK_DONE 1 任务结束可移出任务队列
**         S_TASK_STANDBY 0 任务正在被执行。需等待当前操作结束
**         
**         data 指向任务需要的数据
**         
**         length为数据长度
**
**Author  :袁羿
**Date    :2015/3/28
**
********************************************************************************/
class Task{
public:
	int status;	//1已执行，0未执行，2访问中（需等待）
	char * data;	//在TaskQueue的Push方法中分配内存，在pop内释放
	int length;

public:
	Task( char * src , int length , int status = S_TASK_VISITING){
		data = src;
		this->length = length;
		this->status = status;
	}

	Task(){
		status = S_TASK_VISITING;
		data = NULL;
		length = 0;
	}
};

/********************************************************************************
**
**Function:任务队列定义及实现
**Detail  :实现任务队列的操作。
**         curTaskNumber当前队列内任务数
**         maxTaskNumber最大任务数
**         key标识当前任务队列。通过该字段确定两台设备的连接。pair第一个字段为控制端mac地址，第二个字段为被控端mac地址。
**         curWardIP保存当前控制端的ip
**         curTargetIP保存当前被控端ip
**         isVisitng记录当前队列的访问状态
**
**Author  :袁羿
**Date    :2015/3/29
**
********************************************************************************/
class TaskQueue{
private:
	int curTaskNumber;
	int maxTaskNumber;
	pair<string,string> key;
	list<Task> taskList;

	string curWardIP;
	string curTargetIP;

	bool isVisting;
public:
	TaskQueue(){
		curTaskNumber = 0;
		maxTaskNumber = SRV_MAX_TASK_QUEUE_LENGTH;
		key.first = "NULL";
		key.second = "NULL";
		curWardIP = "NULL";
		curTargetIP = "NULL";
		isVisting = false;
	}

	TaskQueue(string wardMac , string targetMac , int maxNum = SRV_MAX_TASK_QUEUE_LENGTH){
		curTaskNumber = 0;
		maxTaskNumber = SRV_MAX_TASK_QUEUE_LENGTH;
		key.first = wardMac;
		key.second = targetMac;
		curWardIP = "NULL";
		curTargetIP = "NULL";
		isVisting = false;
	}

	~TaskQueue(){
		while( !taskList.empty() ){
			free(taskList.front().data);
			taskList.pop_front();
		}
	}

	bool getStatus( void ){
		return isVisting;
	}

	void setStatus( bool f ){
		isVisting = f;
	}

	bool push_back( char * src , int length , string ward = "NULL" , string target =  "NULL" , int status = S_TASK_VISITING){
		isVisting = true;
		if( length < 0 || src == NULL ){
			printf("error in taskQueue.pushBack\n");
			isVisting = false;
			return false;
		}

		if( curTaskNumber >= SRV_MAX_TASK_QUEUE_LENGTH ){
			printf("error in taskQueue.pushBack:queue is full\n");
			isVisting = false;
			return false;
		}

		if( ward != curWardIP ){		
			curWardIP = ward;
		}

		if( target != curTargetIP ){
			curTargetIP = target;
		}

		//在析构函数或pop_front函数中释放
		char * st = (char*)malloc(sizeof(char)*length + 32 );	

		memcpy( st , src , length );
		Task task = Task( st , length , status);
		taskList.push_back( task );
		++ curTaskNumber;
		taskList.back().status = S_TASK_STANDBY;

		isVisting = false;
		return true;
	}

	void pop_front(){
		isVisting = true;
		if( curTaskNumber > 0 ){
			if( taskList.front().status == S_TASK_VISITING ){
				isVisting = false;
				return;
			}

			taskList.front().status = S_TASK_VISITING;
			-- curTaskNumber;
			free( taskList.front().data );
			taskList.pop_front();	
			isVisting = false;
			return;
		}
		else{
			printf("error in taskQueue.pop_front:queue is empty\n");
			isVisting = false;
			return;
		}
	}

	string getCurWardIP( void ){
		return curWardIP;
	}

	string getCurTargetIP( void ){
		return curTargetIP;
	}

	pair<string,string> getKey( void ){
		return key;
	}

	void setKey( pair<string,string> k ){
		key.first = k.first;
		key.second = k.second;
	}

	bool macExist( string mac ){
		if( mac == key.first || mac == key.second ){
			return true;
		}
		return false;
	}

	int getTaskNumber( void ){
		return curTaskNumber;
	}

	bool isFull( void ){
		return (curTaskNumber == SRV_MAX_TASK_QUEUE_LENGTH);
	}

	Task * getCurTask( void ){
		return &(taskList.front());
	}

	void printQueueInfo( void ){
		//list<Task>::iterator it = taskList.begin();
		
		cout << "[taskQueue]"<< "ward=" << key.first << ' ' << ",target=" << key.second << endl
			 << "[taskQueue]"<< "task num=" << curTaskNumber << endl
			 << "[taskQueue]"<< "ward ip=" << curWardIP << ",target ip=" << curTargetIP << endl;
		
		/*for( int i = 0 ; i < curTaskNumber ; ++ i ){
			if( it == taskList.end() )
				break;

			cout << '[' << i << ']' << "status=" << it->status << ',' << "length=" << it->length << endl << "data=";
			for( int d = 0 ;d < it->length;d ++){
				putchar( it->data[d]);
			}
			putchar('\n');
			it ++;
		}*/
	}
};

class TaskManager{
private:
	int curQueueNumber;
	TaskQueue taskQueues[SRV_MAX_TASK_QUEUE_NUM + 1];
public:
	TaskManager(){
		curQueueNumber = 0;
	}

	~TaskManager(){
		WSACleanup();
	}

	bool addTask(int index , char * src , int length , string ward = "NULL" , string target = "NULL" , int status = S_TASK_VISITING){
		if( index < 0 ){
			return false;
		}

		return taskQueues[index].push_back( src , length , ward , target , status );
	}

	bool removeFirst( int index ){
		if( index < 0 )
			return false;

		if( taskQueues[index].getTaskNumber() == 0 ){
			return false;
		}

		taskQueues[index].pop_front();
		return true;
	}

	int findByMac( string mac ){
		for( int i = 0 ; i < curQueueNumber ; ++ i ){
			if( taskQueues[i].macExist( mac ) ){
				return i;
			}
		}
		return -1;	//cannot find
	}

	bool registerQueue( string wardMac ,string targetMac){
		if( curQueueNumber == SRV_MAX_TASK_QUEUE_NUM )
			return false;

		for( int i = 0 ; i < curQueueNumber ; ++ i ){
			if( taskQueues[i].macExist(wardMac) || taskQueues[i].macExist( targetMac) ){
				return true;
			}
		}

		taskQueues[curQueueNumber].setKey( pair<string,string>(wardMac,targetMac) );
		curQueueNumber ++;
		return true;
	}

	int getTaskNum( void ){
		int count = 0;
		for( int i = 0 ; i < curQueueNumber ; ++ i ){
			count += taskQueues[i].getTaskNumber();
		}
		return count;
	}

	int getTaskQueueNum( void ){
		return curQueueNumber;
	}

	int getSpecifyQueueTaskNum( int index ){
		if(index >= 0 && index < curQueueNumber ){
			return taskQueues[index].getTaskNumber();
		}
		else{
			return 0;
		}
	}

	TaskQueue * getSpecifyQueue( int index ){
		if(index >= 0 && index < curQueueNumber ){
			return (taskQueues + index);
		}
		else{
			return NULL;
		}
	}

	bool isSpecifyQueueFull( int index ){
		return taskQueues[index].isFull();
	}

	int getMaxQueueNum(void ){
		return SRV_MAX_TASK_QUEUE_NUM;
	}

	void printAll( void ){
		cout << "[taskManager]task queue number:" << curQueueNumber << endl;
		for( int i = 0 ; i < curQueueNumber ; ++ i ){
			cout  << "[taskQueue " << i << ']' <<endl;
			taskQueues[i].printQueueInfo();
		}
	}
};

#endif // !TASK_H
