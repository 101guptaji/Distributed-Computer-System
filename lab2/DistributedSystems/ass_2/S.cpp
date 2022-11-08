#include<iostream>
#include<cstring>
#include<thread>
#include<sys/socket.h>
#include<sys/time.h>
#include<netinet/in.h>
#include<cstdlib>
#include<unistd.h>
using namespace std;
char SIZE='s',DATA='d',ACK='a',END='e';
int server_id, new_socket, client_id;
struct sockaddr_in address, client_addr;
int addrlen = sizeof(address);
socklen_t len = sizeof(client_id);
void binaryConverter(char Char_arr, char *bin){
	int dec=Char_arr,i=7;
	while(dec>0){
		*(bin+i)=(char)(dec%2+48);
		dec/=2;
		i--;
	}
}
void CHECKSUM_ALGO(char *arr, char *arr_CKS, char *carry){
	for(int i=7;i>=0;i--){
		if(*(arr+i)=='0'&&*(arr_CKS+i)=='0'&&*carry=='0'){
			*(arr_CKS+i)='0';
			*carry='0';
		}
		if(*(arr+i)=='0'&&*(arr_CKS+i)=='0'&&*carry=='1'){
			*(arr_CKS+i)='1';
			*carry='0';
		}
		if(*(arr+i)=='1'&&*(arr_CKS+i)=='1'&&*carry=='0'){
			*(arr_CKS+i)='0';
			*carry='1';
		}
		if(*(arr+i)=='1'&&*(arr_CKS+i)=='1'&&*carry=='1'){
			*(arr_CKS+i)='1';
			*carry='1';
		}
		if(((*(arr+i)=='1'&&*(arr_CKS+i)=='0')||*(arr+i)=='0'&&*(arr_CKS+i)=='1')&&*carry=='0'){
			*(arr_CKS+i)='1';
			*carry='0';
		}
		if(((*(arr+i)=='1'&&*(arr_CKS+i)=='0')||*(arr+i)=='0'&&*(arr_CKS+i)=='1')&&*carry=='1'){
			*(arr_CKS+i)='0';
			*carry='1';
		}
	}
}
void ack_receiver(bool *Ack_){
	char frame_rec[512];
	while (true){
		recvfrom(server_id,frame_rec,512,0,(struct sockaddr*)&client_addr,&len);
		if(frame_rec[0]==ACK) *Ack_ = true;
		else if(frame_rec[0]==END){
			*Ack_=true;
			break;
		}
	}
}
int main(){
    address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(8080);
	FILE *fp_x;
	fp_x=fopen("file1.c","r");
	char dummy[2],frame_buf[100],frame_rec[100],frame_[100];
	int cnt_D=1,mul=10,sz=0,buf_window=4;
	bool Ack_=false;
	while(fgets(dummy,2,fp_x)){
		sz++;
		if (sz%mul==0){
			cnt_D++;
			mul*= 10;
		}
	}
	fclose(fp_x);
	if ((server_id=socket(AF_INET, SOCK_DGRAM,0))==0){
		perror("Socket Error");
		exit(EXIT_FAILURE);
	}
	if (bind(server_id, (struct sockaddr* )&address, sizeof(address))<0){
		perror("Bind Error");
		exit(EXIT_FAILURE);
	}
	printf("Server is Running\n");
	while(true){
		printf("\nConnecting\n");
		recvfrom(server_id, frame_rec,100,0,(struct sockaddr*)&client_addr,&len);
		printf("Client has requested for file.\n");
		printf("File Size is %d\n", sz);
		memset(frame_buf,0,sizeof(frame_buf));
		frame_buf[0]=SIZE;
		while(cnt_D){
			frame_buf[cnt_D--]=char(sz%10+48);
			sz/=10;
		}
		sendto(server_id, frame_buf,100,0,(struct sockaddr*)&client_addr, len);
		fp_x=fopen("file1.c", "r");
		thread receiving(&ack_receiver, &Ack_);
		while(fgets(frame_buf,buf_window,fp_x)){
			Ack_=false;
			int cnt_retry=0;
			char *bin,*arr_CKS;
			char carry='0';
			bin=(char*)malloc(10*sizeof(char));
			arr_CKS=(char*)malloc(10*sizeof(arr_CKS));
			int i=0;
			for(i=0;i<8;i++) *(arr_CKS+i)='0';
			i=0;
			while(frame_buf[i] != '\0'){
				memset(bin,0, sizeof(bin));
				for(int j=0;j<8;j++) *(bin+j)='0';
				binaryConverter(frame_buf[i], bin);
				CHECKSUM_ALGO(bin, arr_CKS, &carry);
				i++;
			}
			for(int i=0;i<8;i++){
				if(*(arr_CKS+i)=='0') *(arr_CKS+i)='1';
				else *(arr_CKS+i)='0';
			}
			frame_[0]=DATA,frame_[1]=carry;
			for (int i=0;i<8;i++){
                frame_[2+i]=*(arr_CKS+i);
			}
			strcat(frame_, frame_buf);
			while(!Ack_){
				char *sending_frame;
				sending_frame = (char*)malloc(100*sizeof(sending_frame));
				memset(sending_frame,0,sizeof(sending_frame));
				strcpy(sending_frame,frame_);
				if (rand()%10==0){
					if(sending_frame[rand()%5+1]=='0') sending_frame[rand()%5+1]='1';
					else sending_frame[rand()%5+1]='0';
				}
				printf("Buffer(Sent):- \"%s\"\n", frame_buf);
				sendto(server_id, sending_frame,100,0,(struct sockaddr*)&client_addr, len);
				timeval t1;
				timeval t2;
				gettimeofday(&t1, NULL);
				gettimeofday(&t2, NULL);
				while (t2.tv_usec-t1.tv_usec<700){
					gettimeofday(&t2, NULL);
					if (Ack_){
						printf("\nRetry count: %d\n", cnt_retry+1);
						printf("Ack Received");
						break;
					}
				}
				cnt_retry++;
			}
			memset(frame_,0,sizeof(frame_));
			memset(frame_buf,0,sizeof(frame_buf));
		}
		Ack_=false;
		memset(frame_, 0, sizeof(frame_));
		frame_[0]=END;
		while (!Ack_){
			sendto(server_id, frame_,100,0,(struct sockaddr*)&client_addr, len);
			timeval t1;
			timeval t2;
			gettimeofday(&t1, NULL);
			gettimeofday(&t2, NULL);
			while(t2.tv_usec-t1.tv_usec<1000){
				gettimeofday(&t2, NULL);
				if(Ack_) break;
			}
		}
		printf("\nFile Is Successfully Transfered!\n");
		receiving.join();
		fclose(fp_x);
	}
	exit(0);
}
