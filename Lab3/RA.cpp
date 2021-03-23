/*Ricart-Agarwala's distributed mutual exclusion algorithm for a single machine and having 3 ports/nodes on that machine.
*/

#include<iostream>
#include<inttypes.h>
#include<cstring>
#include<string.h>
#include<thread>
#include<mutex>
#include<vector>
#include<sys/time.h>
#include<unistd.h>
#include<stdio.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdlib.h>
#include <queue>

#define dtype1 int8_t
#define dtype2 uint8_t
#define dtype3 uint16_t
#define dtype4 uint32_t
#define dtype5 uint64_t
#define Data_sz 26
#define REQUEST (dtype1)64
#define REPLY (dtype1)32
#define OFFSET_SEC 0
#define OFFSET_USEC 8
#define OFFSET_FD 16
#define OFFSET_MSG_TYPE 24
#define OFFSET_EOM 25
#define Server_nos 2
#define Rec_node_ 1
#define Send_node_ 0

using namespace std;

struct node{
    dtype5 timestamp_seconds;
    dtype5 timestamp_useconds;
    dtype1 *arr;
    struct node *next;
};

dtype4 *Sno_arr = new dtype4[Server_nos];
bool *reply_received = new bool[Server_nos];
queue <dtype1*> queue_;
dtype5 server_sd;
dtype5 req_seconds;
dtype5 req_useconds;
bool access_granted;
bool interested_cs;
dtype5 millis(){
    struct timeval temp;
    gettimeofday(&temp,0);
    return temp.tv_usec;
}
dtype5 seconds(){
    struct timeval temp;
    gettimeofday(&temp,0);
    return temp.tv_sec;
}
void delay(dtype5 s, dtype5 us){
   struct timeval t;
    t.tv_sec=s,t.tv_usec=us;
    select(0,0,0,0,&t);
}
dtype1* Req_msg_(dtype5 sd){
    dtype1 *arr=new dtype1[Data_sz];
    dtype5 sec=seconds(),usec=millis();
    dtype1 type=REQUEST;
    memcpy(arr+OFFSET_SEC,(void*)&sec,8),memcpy(arr+OFFSET_USEC,(void*)&usec,8),memcpy(arr+OFFSET_FD,(void*)&sd,8),memcpy(arr+OFFSET_MSG_TYPE,(void*)&type,1);
    return arr;
}
dtype1* Rep_msg_(dtype5 sd){
    dtype1 *arr=new dtype1[Data_sz];
    dtype5 sec=seconds(),usec=millis();
    dtype1 type=REPLY;
    memcpy(arr+OFFSET_SEC,(void*)&sec,8),memcpy(arr+OFFSET_USEC,(void*)&usec,8),memcpy(arr+OFFSET_FD,(void*)&sd,8),memcpy(arr+OFFSET_MSG_TYPE,(void*)&type,1);
    return arr;
}
dtype5 get_fd_msg(dtype1 *arr){
    dtype5 sd;
    memcpy((void*)&sd,arr+OFFSET_FD,8);
    return sd;
}
dtype5 get_timestamp_seconds(dtype1 *arr){
    dtype5 time;
    memcpy((void*)&time,arr+OFFSET_SEC,8);
    return time;
}
dtype5 get_timestamp_useconds(dtype1 *arr){
    dtype5 time;
    memcpy((void*)&time,arr+OFFSET_USEC,8);
    return time;
}
dtype1 get_msg_type(dtype1* arr){
    dtype1 mtype;
    memcpy((void*)&mtype,arr+OFFSET_MSG_TYPE,1);
    return mtype;
}

void Reply_Recvr(dtype4 sd, dtype4 *in_conn){
    struct sockaddr_in remote_addr;
    if((*in_conn=accept(sd,(struct sockaddr *)&remote_addr,(socklen_t*)&remote_addr))<0) exit(EXIT_FAILURE);
    return;
}
void Req_Sender(dtype3 port, dtype4 *remote_sd){
    struct sockaddr_in remote_addr;
    *remote_sd=0;
    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family=AF_INET;
    remote_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    remote_addr.sin_port = htons(port);
    if((*remote_sd=socket(AF_INET, SOCK_STREAM, 0))<0) return;
    while(connect(*remote_sd, (struct sockaddr *)&remote_addr, sizeof(remote_addr))<0) perror("Error in establishing connection Retrying...");
    return;
}
void CS_Req(dtype5 sec,dtype3 server_port){
    dtype1 *req_msg;
    dtype1 *reply_msg;
    dtype5 reply_fd;
    dtype3 retry;
    while(true){
        interested_cs = false;
        delay(sec,0);
        interested_cs = true;
        cout<<"Started Requesting Critical Section.\n";
        req_msg=Req_msg_(server_sd);
        req_seconds=get_timestamp_seconds(req_msg);
        req_useconds=get_timestamp_useconds(req_msg);
        for(int i=0;i<Server_nos;i++){
            reply_received[i]=false;
            send(Sno_arr[i],req_msg,Data_sz,0);
        }
        for(int i=0;i<Server_nos;i++){
            retry=1;
            while(!reply_received[i]){
                if(retry>6){
                    send(Sno_arr[i],req_msg,Data_sz,0);
                    retry = 1;
                }
                delay(0,500);
                retry++;
            }
        }
        cout<<"Critical Section has been Acquired.\n";
        access_granted=true;
        FILE *file;
        file=fopen("CS_File.txt","a");
        fprintf(file,"Inside CS, Port: %ld at %2ld\n",server_port,seconds());
        delay(1,0);
        fprintf(file,"Exiting CS, Port: %ld at %2ld\n\n",server_port,seconds());
        fclose(file);
        access_granted = false;
        cout<<"Released CS and Replying to Nodes Request of the Deferred Queue.\n";
        reply_msg = Rep_msg_(server_sd);
        while(!queue_.empty()){
            reply_fd = get_fd_msg(queue_.pop());
            send(reply_fd,reply_msg,Data_sz,0);
        }
    }
}
void Msg_Rec(dtype3 re){
    dtype1 *reply_msg,*received_msg = new dtype1[Data_sz];
    dtype3 msg_type;
    dtype5 recv_seconds,recv_useconds;
    while(true){
        cout<<"msg receiving has started.\n";
        read(Sno_arr[re],received_msg,Data_sz);
        msg_type=get_msg_type(received_msg);
        if(msg_type==(dtype3)REPLY){
            cout<<"A -Reply- msg has been received from "<<re<<"\n";
            reply_received[re] = true;
        }else if(msg_type==(dtype3)REQUEST){
            cout<<"A -Request- msg has been received from "<<re<<"\n";
            reply_msg=Rep_msg_(server_sd);
            if(!interested_cs&&!access_granted){
                cout<<"Not Interested in Critical Section at the moment.\n";
                delay(0,100);
                send(Sno_arr[re],reply_msg,Data_sz,0); //cond1: reply if neither requesting nor executing the CS
                cout<<"Reply msg has been sent to: "<<re<<"\n";
            }else if(interested_cs && !access_granted){
                recv_seconds = get_timestamp_seconds(received_msg);
                recv_useconds = get_timestamp_useconds(received_msg);
                if(recv_seconds<req_seconds||(recv_seconds == req_seconds && recv_useconds < req_useconds)){
                    cout<<"Interested in Critical Section but have greater Time stamp than "<<re<<"\n";
                    send(Sno_arr[re],reply_msg,Data_sz,0); //cond2: reply if requesting but have higher timestamp
                    cout<<"Reply msg has been sent to: "<<re<<"\n";
                }else{
                    queue_.push(received_msg); //cond3: inserting the request in the Deferred queue
                    cout<<"Interested in Critical Section and have lesser Time stamp than "<<re<<", So pushed request in Queue.\n";
                }
            }
        }
    }
}
void Req_Listener(){
    vector<thread> threads;
    for(int i=0;i<Server_nos;i++) threads.push_back(thread(Msg_Rec,i));
    for(vector<thread>::iterator itr = threads.begin(); itr != threads.end() ; itr++) itr->join(); 
}
int main(int argc, char *args[]){
    if(argc<4) exit(EXIT_FAILURE);
    dtype3 server_port = stoi(args[1]); //string to integer
    dtype3 remote_port = stoi(args[2]);
    dtype5 sec = stoull(args[3]);	//Hexadecimal string to unsigned long long
    int32_t sd;
    struct sockaddr_in addr;
    sd=socket(AF_INET,SOCK_STREAM,0); 
    if(sd==0) exit(EXIT_FAILURE);
    int opt=1;
    if(setsockopt(sd,SOL_SOCKET,SO_REUSEADDR|SO_REUSEPORT,&opt,sizeof(opt))) exit(EXIT_FAILURE);  //https://www.cct.lsu.edu/~sidhanti/tutorials/network_programming/setsockoptman.html
    memset(&addr,0,sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //connecting on same machine
    addr.sin_port = htons(server_port);		  //converts the unsigned short integer hostshort from host byte order to network byte order.
						//https://www.gta.ufrj.br/ensino/eel878/sockets/htonsman.html
    if(bind(sd,(struct sockaddr *)&addr,sizeof(addr))<0) exit(EXIT_FAILURE);
    cout<<"binded to server"<<endl;
    if(listen(sd, 3)<0) exit(EXIT_FAILURE);
	cout<<"listening to server"<<endl;
    server_sd=sd;
    cout<<"Server has started on port no: "<<server_port<<"\n";
    thread t1(Req_Sender,remote_port,&Sno_arr[Send_node_]),t2(Reply_Recvr,server_sd,&Sno_arr[Rec_node_]); //for connection establishment
    t1.join(),t2.join();
    //system("clear");
    cout<<"Node has established the Connection.\n";
    thread t3(CS_Req,sec,server_port),t4(Req_Listener);
    cout<<"**************************************\n";
    t3.join(),t4.join();
}
