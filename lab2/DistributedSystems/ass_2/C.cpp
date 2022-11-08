#include<iostream>
#include<cstring>
#include<cstdlib>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/time.h>
using namespace std;
int server_id, new_socket, valread, client_id;
struct sockaddr_in address;
struct sockaddr_in client_addr;
int addrlen=sizeof(address);
char SIZE='s',DATA='d',ACK='a',END='e';
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
void binaryConverter(char Char_arr, char *bin){
	int dec=Char_arr,i=7;
	while(dec>0){
		*(bin+i)=(char)(dec%2+48);
		dec/=2;
		i--;
	}
}
int main(){
	char *frame_buf=(char*)malloc(100*sizeof(char));
	char *ack_End_arr=(char*)malloc(100*sizeof(char));
	strcpy(ack_End_arr,"Send me file");
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(8090);
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
	client_addr.sin_port=htons(8080);
	if((server_id=socket(AF_INET,SOCK_DGRAM,0)) == 0){
		perror("Socket failed");
		exit(EXIT_FAILURE);
	}
	if(bind(server_id, (struct sockaddr* )&address, sizeof(address))<0){
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	socklen_t len=sizeof(client_addr);
	sendto(server_id,ack_End_arr,100,0,(struct sockaddr*)&client_addr, len);
	recvfrom(server_id,frame_buf,100,0,(struct sockaddr*)&client_addr, &len);
	int sz=0;
	if (frame_buf[0] == SIZE){
		int i = 1;
		while(frame_buf[i]){
			sz=sz*10+(int(frame_buf[i])-48);
			i++;
		}
	}
	printf("File is of size: %d\n", sz);
	FILE* fp_x;
	fp_x=fopen("file2.c", "w");
	while(true){
		memset(frame_buf,0,sizeof(frame_buf));
		recvfrom(server_id, frame_buf, 100, 0, (struct sockaddr*)&client_addr, &len);
		bool flg = true;
		char frame_rec[512];
		for(int i=0;i<sizeof(frame_buf)/sizeof(char);i++) frame_rec[i]=frame_buf[10+i];
		char carry_recv=frame_buf[1],carry='0';
		char *arr_CKS, *rec_CKS, *bin;
		rec_CKS = (char*)malloc(10*sizeof(char));
		bin=(char*)malloc(10*sizeof(char));
		arr_CKS=(char*)malloc(10*sizeof(char));
		for(int i=0;i<8;i++) *(arr_CKS+i)=frame_buf[2+i];
		for(int i=0;i<8;i++) *(rec_CKS+i)='0';
		int i=0;
		while(frame_rec[i]!='\0'){
			memset(bin,0,sizeof(bin));
			for(int j=0;j<8;j++) *(bin+i)='0';
			binaryConverter(frame_rec[i], bin);
			CHECKSUM_ALGO(bin, rec_CKS, &carry);
			i++;
		}
		for(int i=0;i<8;i++){
			if(*(rec_CKS+i)=='0') *(rec_CKS+i)='1';
			else *(rec_CKS+i)='0';
		}
		if(carry!=carry_recv) flg=false;
		for(int i=0;i<8;i++){
			if(*(arr_CKS+i)!=*(rec_CKS+i)) flg=false;
		}
		if(frame_buf[0]!=END&&!flg) printf("Error in Data\n");
		if(frame_buf[0]==END) flg=true;
		if(rand()%2==0&&flg){
			printf("Frame Recieved:- \"%s\"\n",frame_rec);
			fputs(frame_rec, fp_x);
			memset(ack_End_arr, 0, sizeof(ack_End_arr));
			ack_End_arr[0]=ACK;
			sendto(server_id,ack_End_arr,100,0,(struct sockaddr*)&client_addr, len);
			if(frame_buf[0]==END) break;
		}
		else printf("\nMissed Ack\n");
	}
	memset(ack_End_arr,0,sizeof(ack_End_arr));
	ack_End_arr[0]=END;
	sendto(server_id, ack_End_arr,100,0,(struct sockaddr*)&client_addr, len);
	printf("\n\tFile Received Sucessfully!\n");
	fclose(fp_x);
	return 0;
}
