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


#define int_type1 int8_t
#define int_type2 uint8_t
#define int_type3 uint16_t
#define int_type4 uint32_t
#define int_type5 uint64_t
#define data_size 26
#define REQUEST 1
#define REPLY 2
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
    int_type5 timestamp_seconds;
    int_type5 timestamp_useconds;
    int_type1 *arr;
    struct node *next;
};
class post_queue{
    struct node *head;
    public:
        post_queue();
        bool isEmpty();
        void insert(int_type1*);
        int_type1* pop();
};

post_queue *queue_ = new post_queue();
int_type4 *Sno_arr = new int_type4[Server_nos];
bool *reply_received = new bool[Server_nos];
int_type5 server_sd;
int_type5 req_seconds;
int_type5 req_useconds;
bool access_granted;
bool interested_cs;
int_type5 millis(){
    struct timeval temp;
    gettimeofday(&temp,0);
    return temp.tv_usec;
}
int_type5 seconds(){
    struct timeval temp;
    gettimeofday(&temp,0);
    return temp.tv_sec;
}
void delay(int_type5 s, int_type5 us){
   struct timeval t;
    t.tv_sec=s,t.tv_usec=us;
    select(0,0,0,0,&t);
}
int_type1* Req_msg(int_type5 sd){
    int_type1 *arr=new int_type1[data_size];
    int_type5 sec=seconds(),usec=millis();
    int_type1 type=REQUEST;
    memcpy(arr+OFFSET_SEC,(void*)&sec,8),memcpy(arr+OFFSET_USEC,(void*)&usec,8),memcpy(arr+OFFSET_FD,(void*)&sd,8),memcpy(arr+OFFSET_MSG_TYPE,(void*)&type,1);
    return arr;
}
int_type1* Rep_msg(int_type5 sd){
    int_type1 *arr=new int_type1[data_size];
    int_type5 sec=seconds(),usec=millis();
    int_type1 type=REPLY;
    memcpy(arr+OFFSET_SEC,(void*)&sec,8),memcpy(arr+OFFSET_USEC,(void*)&usec,8),memcpy(arr+OFFSET_FD,(void*)&sd,8),memcpy(arr+OFFSET_MSG_TYPE,(void*)&type,1);
    return arr;
}
int_type5 get_fd_msg(int_type1 *arr){
    int_type5 sd;
    memcpy((void*)&sd,arr+OFFSET_FD,8);
    return sd;
}
int_type5 get_timestamp_seconds(int_type1 *arr){
    int_type5 time;
    memcpy((void*)&time,arr+OFFSET_SEC,8);
    return time;
}
int_type5 get_timestamp_useconds(int_type1 *arr){
    int_type5 time;
    memcpy((void*)&time,arr+OFFSET_USEC,8);
    return time;
}
int_type1 get_msg_type(int_type1* arr){
    int_type1 mtype;
    memcpy((void*)&mtype,arr+OFFSET_MSG_TYPE,1);
    return mtype;
}
post_queue::post_queue(){
    head = nullptr;
}
bool post_queue::isEmpty(){
    return head==nullptr;
}
void post_queue::insert(int_type1 *arr){
    struct node *new_node = new node;
    new_node->timestamp_seconds = get_timestamp_seconds(arr);
    new_node->timestamp_useconds = get_timestamp_useconds(arr);
    new_node->arr = new int_type1[data_size];
    memcpy(new_node->arr,arr,data_size);
    new_node->next = nullptr;
    if(head==nullptr){
        head = new_node;
        return;
    }
    struct node *temp = head;
    while(temp->next!=nullptr&&(temp->next->timestamp_seconds<new_node->timestamp_seconds||(temp->next->timestamp_seconds==new_node->timestamp_seconds&&temp->next->timestamp_useconds<new_node->timestamp_useconds))) temp = temp->next;
    new_node->next = temp->next;
    temp->next = new_node;
    return;
}
int_type1* post_queue::pop(){
    if(isEmpty()) return nullptr;
    int_type1 *temp=new int_type1[data_size];
    memcpy(temp,head->arr,data_size);
    struct node *rm=head;
    head=head->next;
    delete(rm);
    return temp;
}
void Reply_Recvr(int_type4 sd, int_type4 *in_conn){
    struct sockaddr_in remote_addr;
    if((*in_conn=accept(sd,(struct sockaddr *)&remote_addr,(socklen_t*)&remote_addr))<0) exit(EXIT_FAILURE);
    return;
}
void Req_Sender(int_type3 port, int_type4 *remote_sd){
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
void CS_Req(int_type5 sec,int_type3 server_port){
    int_type1 *req_msg;
    int_type1 *reply_msg;
    int_type5 reply_fd;
    int_type3 retry;
    while(true){
        interested_cs = false;
        delay(sec,0);
        interested_cs = true;
        cout<<"Started Requesting Critical Section.\n";
        req_msg=Req_msg(server_sd);
        req_seconds=get_timestamp_seconds(req_msg);
        req_useconds=get_timestamp_useconds(req_msg);
        for(int i=0;i<Server_nos;i++){
            reply_received[i]=false;
            send(Sno_arr[i],req_msg,data_size,0);
        }
        for(int i=0;i<Server_nos;i++){
            retry=1;
            while(!reply_received[i]){
                if(retry>6){
                    send(Sno_arr[i],req_msg,data_size,0);
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
        reply_msg = Rep_msg(server_sd);
        while(!queue_->isEmpty()){
            reply_fd = get_fd_msg(queue_->pop());
            send(reply_fd,reply_msg,data_size,0);
        }
    }
}
void Msg_Rec(int_type3 re){
    int_type1 *reply_msg,*received_msg = new int_type1[data_size];
    int_type3 msg_type;
    int_type5 recv_seconds,recv_useconds;
    while(true){
        cout<<"msg receiving has started.\n";
        read(Sno_arr[re],received_msg,data_size);
        msg_type=get_msg_type(received_msg);
        if(msg_type==(int_type3)REPLY){
            cout<<"A -Reply- msg has been received from "<<re<<"\n";
            reply_received[re] = true;
        }else if(msg_type==(int_type3)REQUEST){
            cout<<"A -Request- msg has been received from "<<re<<"\n";
            reply_msg=Rep_msg(server_sd);
            if(!interested_cs&&!access_granted){
                cout<<"Not Interested in Critical Section at the moment.\n";
                delay(0,100);
                send(Sno_arr[re],reply_msg,data_size,0); //cond1: reply if neither requesting nor executing the CS
                cout<<"Reply msg has been sent to: "<<re<<"\n";
            }else if(interested_cs && !access_granted){
                recv_seconds = get_timestamp_seconds(received_msg);
                recv_useconds = get_timestamp_useconds(received_msg);
                if(recv_seconds<req_seconds||(recv_seconds == req_seconds && recv_useconds < req_useconds)){
                    cout<<"Interested in Critical Section but have greater Time stamp than "<<re<<"\n";
                    send(Sno_arr[re],reply_msg,data_size,0); //cond2: reply if requesting but have higher timestamp
                    cout<<"Reply msg has been sent to: "<<re<<"\n";
                }else{
                    queue_->insert(received_msg); //cond3: inserting the request in the Deferred queue
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
    int_type3 server_port = stoi(args[1]); //string to integer
    int_type3 remote_port = stoi(args[2]);
    int_type5 sec = stoull(args[3]);	//Hexadecimal string to unsigned long long
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
