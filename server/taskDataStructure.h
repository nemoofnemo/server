#ifndef TASK_H
#define TASK_H

#include <list>
#include <utility>
#include <vector>
#include <string>
#include <iostream>
#include <cstdlib>
#include <process.h>
#include <Winsock2.h>
#include <Windows.h>

#pragma comment(lib, "ws2_32.lib") 

#define SRV_ERROR -1
#define SRV_NOT_FOUND -1
#define SRV_MAX_TASK_QUEUE_LENGTH 30
#define SRV_MAX_TASK_QUEUE_NUM 40

using std::pair;
using std::vector;
using std::string;
using std::list;
using std::iterator;
using std::cout;
using std::endl;

class Task{
public:
	char * data;	//在TaskQueue的Push方法中分配内存，在pop内释放
	int length;
public:
	Task( char * src , int length ){
		data = src;
		this->length = length;
	}

	Task(){
		data = NULL;
		length = 0;
	}
};

template <typename keyType>
class TaskQueue{
private:
	int curTaskNumber;
	int maxTaskNumber;
	list<keyType> keys;
	list<Task> taskList;
public:
	TaskQueue(){
		curTaskNumber = 0;
		maxTaskNumber = SRV_MAX_TASK_QUEUE_LENGTH;
	}

	TaskQueue(list<keyType> keys , int maxNum = SRV_MAX_TASK_QUEUE_LENGTH){
		curTaskNumber = 0;
		maxTaskNumber = SRV_MAX_TASK_QUEUE_LENGTH;
		this.keys = keys;
	}

	~TaskQueue(){
		while( !taskList.empty() ){
			free(taskList.front().data);
			taskList.pop_front();
		}
	}

	void clear( void ){
		while( !taskList.empty() ){
			free(taskList.front().data);
			taskList.pop_front();
		}
	}

	bool push_back( Task arg  ){
		if( curTaskNumber >= SRV_MAX_TASK_QUEUE_LENGTH ){
			printf("[taskQueue]:error in taskQueue.pushBack:queue is full\n");
			return false;
		}

		taskList.push_back( arg );
		++ curTaskNumber;
		return true;
	}

	bool push_back( char * src , int length ){
		if( length < 0 || src == NULL ){
			printf("error in taskQueue.pushBack\n");
			return false;
		}

		if( curTaskNumber >= SRV_MAX_TASK_QUEUE_LENGTH ){
			printf("error in taskQueue.pushBack:queue is full\n");
			return false;
		}

		//在析构函数或pop_front函数中释放
		char * st = (char*)malloc(sizeof(char)*length + 32 );	
		memcpy( st , src , length );

		taskList.push_back( Task( st , length ) );
		++ curTaskNumber;
		return true;
	}

	bool pop_front( void ){
		if( curTaskNumber > 0 ){
			-- curTaskNumber;
			free( taskList.front().data );
			taskList.pop_front();	
			return true;
		}
		else{
			printf("[taskQueue]:error in taskQueue.pop_front:queue is empty\n");
			return false;
		}
	}

	list<keyType> getKey( void ){
		return this.keys;
	}

	void setKeys( list<keyType> keys){
		this.keys = keys;
	}

	int isKeyExist( const keyType & key ){
		int ret = SRV_NOT_FOUND;
		int limit = keys.size();
		for( int i = 0 ; i < limit ; ++ i ){
			if( keys[i] == key ){
				ret = i;
				break;
			}
		}
		return ret;
	}

	int size( void ){
		return curTaskNumber;
	}

	Task * getCurTask( void ){
		if( curTaskNumber == 0 ){
			return NULL;
		}
		else{
			return &(taskList.front());
		}
	}

	bool isFull( void ){
		return (curTaskNumber == SRV_MAX_TASK_QUEUE_LENGTH);
	}

	//need test
	void printInfo( void ){
		using std::iterator;
		list<Task>::iterator it = taskList.begin();
		
		cout << "[taskQueue]"<< "ward=" << key.first << ' ' << ",target=" << key.second << endl
			 << "[taskQueue]"<< "task num=" << curTaskNumber << endl
			 << "[taskQueue]"<< "ward ip=" << curWardIP << ",target ip=" << curTargetIP << endl;
		
		for( int i = 0 ; i < curTaskNumber ; ++ i ){
			if( it == taskList.end() )
				break;

			cout << '[' << i << ']' << "length=" << it->length << endl << "data=";
			for( int d = 0 ;d < it->length;d ++){
				putchar( it->data[d]);
			}
			putchar('\n');
			it ++;
		}
	}
};

template <typename keyType>
class TaskManager{
private:
	int curQueueNumber;
	vector< pair<TaskQueue<keyType>,CRITICAL_SECTION> > taskQueues;
	CRITICAL_SECTION taskManagerCriticalSection;
public:
	TaskManager(){
		curQueueNumber = 0;
		if( InitializeCriticalSectionAndSpinCount( &taskManagerCriticalSection , 4096) != TRUE){
			puts("[taskManager]:create global cs failed.");
			exit(EXIT_FAILURE);
		}
	}

	~TaskManager(){
		DeleteCriticalSection( &taskManagerCriticalSection );
		for( int i = 0 ; i < curQueueNumber ; ++ i ){
			DeleteCriticalSection( &(taskQueues[i].second ) );
		}
		WSACleanup();
	}

	//need test
	bool addQueue( list<keyType> keys ){
		int i = 0;
		int count = -1;
		CRITICAL_SECTION cs;
		
		if( curQueueNumber == SRV_MAX_TASK_QUEUE_NUM ){
			return false;
		}
		
		for( i = 0; i < curQueueNumber ; ++ i ){
			EnterCriticalSection( &(taskQueues[i].second) );
		}

		taskQueues.push_back( pair<TaskQueue<keyType>,CRITICAL_SECTION>(TaskQueue<keyType>( keys ) , cs) );
		InitializeCriticalSectionAndSpinCount( &( taskQueues.back().second ) , 4096 );
		++ curQueueNumber ;

		for( i = 0 ; i < curQueueNumber ; ++ i ){
			LeaveCriticalSection( &(taskQueues[i].second) );
		}

		return true;
	}

	//need test
	bool removeQueue(string mac){
		if( curQueueNumber == 0 ){
			return false;
		}

		int i = 0;
		int pos = -1;
		CRITICAL_SECTION cs;

		for( i = 0; i < curQueueNumber ; ++ i ){
			EnterCriticalSection( &(taskQueues[i].second) );
		}

		for( i = 0 ; i < curQueueNumber ; ++ i ){
			if( pos == -1 && taskQueues[i].first.macExist(mac) ){ 
				pos = i;
			}
			LeaveCriticalSection( &(taskQueues[i].second) );
		}

		if( pos >= 0 && pos < curQueueNumber ){
			cs = taskQueues[pos].second;
			EnterCriticalSection( &cs );
			taskQueues[pos].first.clear();
			taskQueues.erase( taskQueues.begin() + pos );
			LeaveCriticalSection( &cs );
			DeleteCriticalSection( &cs );
			curQueueNumber --;
			return true;
		}

		return false;
	}

	int findByKey( keyType key ){
		int ret = SRV_NOT_FOUND;
		for( int i = 0 ; i < curQueueNumber ; ++ i ){
			if( taskQueues[i].first.isKeyExist(key) ){
				ret = i;
				break;
			}
		}
		return ret;	//cannot find -1 (SRV_NOT_FOUND).
	}

	int getTaskQueueNum( void ){
		int ret = curQueueNumber;
		return ret;
	}

	TaskQueue * getSpecifyQueue( int index ){
		TaskQueue * ret = NULL;
		if(index >= 0 && index < curQueueNumber ){
			ret = &(taskQueues[index].first);
		}
		return ret;
	}

	void EnterSpecifyCriticalSection( int index ){
		if( index < curQueueNumber && index >= 0 )
			EnterCriticalSection( &(taskQueues[index].second) );
	}

	void LeaveSpecifyCritialSection( int index ){
		if( index < curQueueNumber && index >= 0 )
			LeaveCriticalSection( &(taskQueues[index].second) );
	}

	static int getMaxQueueNum(void ){
		return SRV_MAX_TASK_QUEUE_NUM;
	}

};


#endif // !TASK_H
