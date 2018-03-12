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



class tcp
{
public:
	tcp();
	tcp(char* addr,unsigned short int port);
	tcp(const int& sockfd_,const sockaddr_in& address_,int connect_status_):
		address(address_),sockfd(sockfd_),connect_status(connect_status_)
		{
			address_len=sizeof(address_);
		}
	tcp(in_addr_t addr,unsigned short int port);
	~tcp();
	void close();
	tcp(const tcp&)=delete;
	tcp(tcp&&)=delete;
	tcp operator=(const tcp&)=delete;
	tcp operator=(tcp&&)=delete;
	void in(char* buff,size_t len);
	void in(int fd);
	void out(char* buff,size_t len);
	void connect(char* addr,unsigned short int port);
	void check();
protected:
	struct sockaddr_in address;						//一般表示对方地址
	int sockfd;
	socklen_t address_len;
public:
	volatile int connect_status=0;						//用于指示tcp连接的状况，1表示已连接，0表示未连接，2表示已连接并
										//被某个程序接管，不应当被处理
};
tcp::tcp()
{
	sockfd=socket(AF_INET,SOCK_STREAM,0);
}

tcp::tcp(in_addr_t addr,unsigned short int port)
{
	sockfd=socket(AF_INET,SOCK_STREAM,0);

        address.sin_family=AF_INET;
        address.sin_addr.s_addr=htonl(addr);
        address.sin_port=htons(port);                                            
        address_len=sizeof(address);

	bind(sockfd,(struct sockaddr*)&address,address_len);
	if(errno<0)
	{
		perror("bind fail!");
	}
}
void tcp::check()
{
	char ch;
	if(connect_status!=0)
	{	
		while(1)
		{
			if(recv(sockfd,&ch,1,0)==0)
			{
				close();
				break;
			}
		}
	}
}

tcp::tcp(char* addr,unsigned short int port)
{
        sockfd=socket(AF_INET,SOCK_STREAM,0);

        address.sin_family=AF_INET;
        address.sin_addr.s_addr=inet_addr(addr);
        address.sin_port=htons(port);
        address_len=sizeof(address);

	bind(sockfd,(struct sockaddr*)&address,address_len);
	if(errno<0)
	{
		perror("bind fail!");
	}
}
tcp::~tcp()
{
	if(connect_status!=0)
	{	
		if(close_(sockfd)==-1)
		{
			perror("close");
		}
		else
			connect_status=0;
	}

}
void tcp::close()
{
	if(connect_status!=0)
	{	
		if(close_(sockfd)==-1)
		{
			perror("close");
		}
		else
			connect_status=0;
	}
}

void tcp::in(char *buff,size_t len)
{
	ssize_t ret;
	while(len!=0&&(ret=read(sockfd,buff,len))!=0)
	{
		if(ret==-1)
		{
			if(errno==EINTR)
				continue;
			perror("tcp read fail!");
			break;
		}
		len-=ret;
		buff+=ret;
	}
}
void tcp::in(int fd)
{
	char ch;
	int ret;
	if(connect_status==1)
	{		
		while(recv(sockfd,&ch,1,0)>0)
		{
			ret=write(fd,&ch,1);
			if(ret<0)
			{
				perror("file write error");
			}
		}
		if(recv(sockfd,&ch,1,0)==0)
		{
			close();
		}
	}
}
void tcp::connect(char* addr,unsigned short int port)
{
        sockaddr_in other;
	socklen_t other_len;
	int return_val;

        other.sin_family=AF_INET;
        other.sin_addr.s_addr=inet_addr(addr);
        other.sin_port=htons(port);
        other_len=sizeof(other);
	if(connect_status==0)							//only if tcp is not on,we can use the connect function
	{
		return_val=connect_(sockfd,(sockaddr*)&other,other_len);
		if(return_val==-1)
			perror("connect fail!");
		else
		{
			connect_status=1;
		}
	}
}
void tcp::out(char* buff,size_t len)
{
	ssize_t ret;
	while(len!=0&&(ret=write(sockfd,buff,len))!=0)
	{
		if(ret==-1)
		{
			if(errno==EINTR)
				continue;
			perror("tcp send");
                        break;
        	}
		len-=ret;
		buff+=ret;
	}
}


class tcp_server
{
public:
	tcp_server()=delete;
	tcp_server(unsigned short int port)
	{
		sockfd=socket(AF_INET,SOCK_STREAM,0);
		address.sin_family=AF_INET;
	        address.sin_addr.s_addr=htonl(INADDR_ANY);
	        address.sin_port=htons(port);                                            
	        address_len=sizeof(address);
		
		bind(sockfd,(struct sockaddr*)&address,address_len);		
	        if(errno<0)
        	{
                	perror("bind fail!");
        	}
		else
			listen_flag=1;
	}
	tcp_server(in_addr_t addr,unsigned short int port)
	{
		sockfd=socket(AF_INET,SOCK_STREAM,0);

        	address.sin_family=AF_INET;
        	address.sin_addr.s_addr=htonl(addr);
        	address.sin_port=htons(port);                                            
        	address_len=sizeof(address);

		bind(sockfd,(struct sockaddr*)&address,address_len);
	        if(errno<0)
        	{
                	perror("bind fail!");
        	}
		else
			listen_flag=1;
	}
	tcp_server(char* addr,unsigned short int port)
	{	
		sockfd=socket(AF_INET,SOCK_STREAM,0);
		
		address.sin_family=AF_INET;
        	address.sin_addr.s_addr=inet_addr(addr);
        	address.sin_port=htons(port);
       		address_len=sizeof(address);

		bind(sockfd,(struct sockaddr*)&address,address_len);
	        if(errno<0)
        	{
                	perror("bind fail!");
        	}
		else
			listen_flag=1;
	}
	tcp_server(const tcp_server&)=delete;
	tcp_server(tcp_server&&)=delete;
	tcp_server operator=(const tcp_server&)=delete;
	tcp_server operator=(tcp_server&&)=delete;
	~tcp_server()
	{
		if(close_(sockfd)==-1)
		{
			perror("close");
		}	
		listen_flag=0;
	}
 	void accept()
	{
		int client_sockfd;
		struct sockaddr_in client_address;
		/*警告！这里backlog设为0会导致难以移植和拒绝服务攻击*/
		listen(sockfd,0);
		while(listen_flag==1)
		{
			socklen_t len=sizeof(client_address);
			client_sockfd=accept_(sockfd,(sockaddr*)&client_address,&len);
			if(client_sockfd==-1)
			{
				perror("accept fail");
			}
			else
			{
				std::shared_ptr<class tcp> client(new tcp(client_sockfd,client_address,1));

				accept_queue_mutex.lock();		
				accept_queue.push_back(client);
				accept_queue_mutex.unlock();
			}
		}
	}
	void close();
	size_t queue_size()
	{
		return accept_queue.size();
	}
public:
	std::list<std::shared_ptr<class tcp>> accept_queue;			//用于存储tcp连接的队列
	std::mutex accept_queue_mutex;
							
private:
	volatile int accept_flag=0;						//用于指示是否继续接受tcp连接
	volatile int listen_flag=0;						//是否可用于侦听
	struct sockaddr_in address;						//表示本地用于侦听的地址
	int sockfd;
	socklen_t address_len;
};

void tcp_server::close()
{
	if(listen_flag!=0)
	{	
		if(close_(sockfd)==-1)
		{
			perror("close");
		}
		else
			listen_flag=0;
	}
}










