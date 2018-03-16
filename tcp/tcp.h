#ifndef __tcp_h__
#define __tcp_h__

#include<sys/socket.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/un.h>
#include<cstdio>
#include<cstdlib>
#include<iostream>
#include<netinet/in.h>
#include<string>
#include<memory>
#include<list>
#include<arpa/inet.h>
#include<mutex>

#include"../../SuLog/logger.h"


/*some posix api has the same name with the function in the class tcp or tcpserver,
 so must encapsulate these api*/
inline int close_(int fd)
{
	return close(fd);
}

inline ssize_t write_(int fd,const void *buf,size_t count)
{
	return write(fd,buf,count);
}

inline ssize_t read_(int fd,void *buf,size_t len)
{
	return read(fd,buf,len);
}

inline int accept_(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	return accept(sockfd,addr,addrlen);
}

inline int connect_(int sockfd, struct sockaddr *serv_addr, socklen_t addrlen)
{
	return connect(sockfd,serv_addr,addrlen);
}



class tcp{
public:
	tcp(logger& _log):log(_log){
		other_status=0;
		sockfd=socket(AF_INET,SOCK_STREAM,0);
		if(sockfd<0){
        	       	log.error("Socket file descriptor get fail:");
			connect_status=0;
			return;
        	}
	}
	tcp(const char* addr,unsigned short int port,logger& _log);
	tcp(const int& sockfd_,const sockaddr_in& address_,
		int connect_status_,logger& _log):
		address(address_),sockfd(sockfd_),connect_status(connect_status_),
		other_status(0),log(_log){
			address_len=sizeof(address_);
	}
	tcp(in_addr_t addr,unsigned short int port,logger& _log);
	virtual ~tcp();
	virtual void close();
	tcp()=delete;
	tcp(const tcp&)=delete;
	tcp(tcp&&)=delete;
	tcp operator=(const tcp&)=delete;
	tcp operator=(tcp&&)=delete;
	void in(char* buff,size_t len);
	void out(char* buff,size_t len);
	void connect(const char* addr,unsigned short int port);
	inline int is_connected(){
		return connect_status.load();
	}
protected:
	struct sockaddr_in address;						//一般表示对方地址
	int sockfd;
	socklen_t address_len;
	std::atomic<int> connect_status;					//用于指示tcp连接的状况，1表示已连接，0表示未连接
	logger log;
public:
	std::atomic<int> other_status;						//用于给使用者的tcp标志位，除初始化外tcp类不参与该项的设置
};


tcp::tcp(in_addr_t addr,unsigned short int port,logger& _log):
	other_status(0),log(_log){

	sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(sockfd<0){
               	log.error("Socket file descriptor get fail:");
		connect_status=0;
		return;
        }

        address.sin_family=AF_INET;
        address.sin_addr.s_addr=htonl(addr);
        address.sin_port=htons(port);                                            
        address_len=sizeof(address);

	int ret=bind(sockfd,(struct sockaddr*)&address,address_len);
	if(ret<0){
		log.error("bind fail!");
	}
}


tcp::tcp(const char* addr,unsigned short int port,logger& _log):
	other_status(0),log(_log){
        sockfd=socket(AF_INET,SOCK_STREAM,0);

	if(sockfd<0){
                log.error("Socket file descriptor get fail:");
		connect_status=0;
		return;
        }

        address.sin_family=AF_INET;
        address.sin_addr.s_addr=inet_addr(addr);
        address.sin_port=htons(port);
        address_len=sizeof(address);

	int ret=bind(sockfd,(struct sockaddr*)&address,address_len);
	if(ret<0){
		log.error("Bind fail:");
	}

}

tcp::~tcp(){
	if(connect_status.load()!=0){	
		if(close_(sockfd)==-1){
			log.error("Close error:");
		}
		else
			connect_status.store(0);
	}
}

void tcp::close(){
	if(connect_status.load()!=0){	
		if(close_(sockfd)==-1){
			log.error("Close error:");
		}
		else
			connect_status.store(0);
	}
}

void tcp::in(char *buff,size_t len){
	ssize_t ret;
	if(len==0 || buff==NULL)
		return; 
	while(len!=0){
		ret=recv(sockfd,buff,len,0);
		if(ret==0){
			close();
		}
		if(ret==-1){
			if(errno==EINTR)
				continue;
			if(errno==ENOTCONN || errno==ENOTSOCK)
				close();
			log.error("Receieve fail:");
			return;
		}
		len-=ret;
		buff+=ret;
	}
}

void tcp::connect(const char* addr,unsigned short int port){
        sockaddr_in other;
	socklen_t other_len;
	int return_val;

	//only if tcp is not on,we can use the connect function
	if(connect_status.load()==0){
        	other.sin_family=AF_INET;
        	other.sin_addr.s_addr=inet_addr(addr);
        	other.sin_port=htons(port);
        	other_len=sizeof(other);
							
		return_val=connect_(sockfd,(sockaddr*)&other,other_len);
		if(return_val==-1)
			log.error("Connect fail!");
		else
			connect_status.store(1);
	}
}

void tcp::out(char* buff,size_t len){
	ssize_t ret;
	while(len!=0&&(ret=write(sockfd,buff,len))!=0){
		if(ret==-1){
			if(errno==EINTR)
				continue;
			log.error("Tcp send error:");
                        break;
        	}
		len-=ret;
		buff+=ret;
	}
}


class tcp_server{
public:
	tcp_server()=delete;
	tcp_server(const tcp_server&)=delete;
	tcp_server(tcp_server &&)=delete;
	tcp_server operator=(const tcp_server&)=delete;
	tcp_server operator=(tcp_server&&)=delete;

	tcp_server(unsigned short int port,logger& _log):
		log(_log),accept_flag(1){

		sockfd=socket(AF_INET,SOCK_STREAM,0);
		if(sockfd<0){
                	log.error("Socket file descriptor get fail:");
			listen_flag=0;
			return;
        	}

		address.sin_family=AF_INET;
	        address.sin_addr.s_addr=htonl(INADDR_ANY);
	        address.sin_port=htons(port);                                            
	        address_len=sizeof(address);
		
		int ret=bind(sockfd,(struct sockaddr*)&address,address_len);		
	        if(ret<0){
                	log.error("bind fail!");
        	}
		else
			listen_flag=1;
	}
	tcp_server(in_addr_t addr,unsigned short int port,logger& _log):
		log(_log),accept_flag(1){

		sockfd=socket(AF_INET,SOCK_STREAM,0);
		if(sockfd<0){
                	log.error("Socket file descriptor get fail:");
			listen_flag=0;
			return;
        	}

        	address.sin_family=AF_INET;
        	address.sin_addr.s_addr=htonl(addr);
        	address.sin_port=htons(port);                                            
        	address_len=sizeof(address);

		int ret=bind(sockfd,(struct sockaddr*)&address,address_len);
	        if(ret<0){
                	log.error("bind fail!");
        	}
		else
			listen_flag=1;
	}
	tcp_server(char* addr,unsigned short int port,logger& _log):
		log(_log),accept_flag(1){
			
		sockfd=socket(AF_INET,SOCK_STREAM,0);
		if(sockfd<0){
                	log.error("Socket file descriptor get fail:");
			listen_flag=0;
			return;
        	}
		
		address.sin_family=AF_INET;
        	address.sin_addr.s_addr=inet_addr(addr);
        	address.sin_port=htons(port);
       		address_len=sizeof(address);

		int ret=bind(sockfd,(struct sockaddr*)&address,address_len);
	        if(ret<0){
                	log.error("bind fail!");
        	}
		else
			listen_flag=1;
	}
	virtual ~tcp_server(){
		if((listen_flag==1) && (close_(sockfd)==-1)){
			log.error("close");
		}	
		listen_flag=0;
	}
 	virtual void accept(){
		int client_sockfd;
		struct sockaddr_in client_address;
		
		/*警告！这里backlog设为0会导致难以移植和拒绝服务攻击*/
		int ret=listen(sockfd,0);
		if(ret<0){
			log.error("Listen fail");
			listen_flag.store(0);
			return;
		}

		while(listen_flag.load()==1 && accept_flag.load()==1){
			

			socklen_t len=sizeof(client_address);
			client_sockfd=accept_(sockfd,(sockaddr*)&client_address,&len);
			if(client_sockfd==-1){
				log.error("Accept fail");
			}
			else{
				std::shared_ptr<class tcp> client(new tcp(client_sockfd,client_address,1,log));

				accept_queue_mutex.lock();		
				accept_queue.push_back(client);
				accept_queue_mutex.unlock();
			}
		}
	}


	/*must use this function in the different thread with accept*/
	inline void stopaccept(){
		accept_flag.store(0);
	}

	void close();
	size_t queue_size(){
		std::lock_guard<std::mutex> lock(accept_queue_mutex);
		return accept_queue.size();
	}
public:
	std::list<std::shared_ptr<class tcp>> accept_queue;			//用于存储tcp连接的队列
	std::mutex accept_queue_mutex;
							
protected:
	std::atomic<int> accept_flag;						//用于指示是否继续接受tcp连接
	std::atomic<int> listen_flag;				
//是否可用于侦听
	struct sockaddr_in address;						//表示本地用于侦听的地址
	int sockfd;
	socklen_t address_len;
	logger log;
};


void tcp_server::close(){
	if(listen_flag!=0)	{	
		if(close_(sockfd)==-1){
			log.error("close");
		}
		else
			listen_flag=0;
	}
}


#endif







