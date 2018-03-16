#include"tcp.h"
#include<thread>

void recv_msg(std::shared_ptr<class tcp>& client){
	char buffer[4];
	client->in(buffer,4);
	printf("%d",*((int*)buffer));
	return;
}

int main(){
	logger mainlog("ForTcpTest");
	static tcp_server main_server(3638,mainlog);
	
	auto server_=&main_server;

	std::thread accept([server_](){		server_->accept();	});
	accept.detach();

	while(1){
		/*为防止优化过甚，加入sleep(0)，如为-g可不加*/ 	
		sleep(0);
		
		for(auto it=main_server.accept_queue.begin();it!=main_server.accept_queue.end();it++){	
			if((*it)->is_connected()==1 && 
				(*it)->other_status.load()==0){
				std::cout<<"Recv a accquest!\n";
				(*it)->other_status.store(1);
				std::thread client(std::bind(recv_msg,*it));
					client.detach();
			}else if((*it)->is_connected()==0){
				auto temp=it;
				temp++;
				main_server.accept_queue_mutex.lock();
				main_server.accept_queue.erase(it);
				main_server.accept_queue_mutex.unlock();
				
				it=temp;
			}
		}
		
		
	}
	return 0;
}
