#include"tcp.h"


int main(){
	logger mainlog("ForTcpTest");
	mainlog.print("Start write log!");


	for(int i=0;i<100;i++){
		tcp test(mainlog);
		test.connect("127.0.0.1",3638);
		if(test.is_connected()==1){
			mainlog.print("A new connection,this is the %dth!",i);
			test.out((char *)&i,4);
		}
	}

	return 0;




}
