 //RICART AGRAWALA ALGORITHM FOR MUTUAL EXCLUSION  
 #include <stdio.h>  
 #include <stdlib.h>  
 #include <pthread.h>  
 #include <unistd.h>  
 #include <errno.h>  
 #include <string.h>  
 #include <sys/types.h>  
 #include <sys/times.h>  
 #include <sys/socket.h>  
 #include <netinet/in.h>  
 #include <arpa/inet.h>  
 #include <sys/wait.h>  
 #include <semaphore.h>  
 #include <sys/shm.h>  
 #include <sys/ipc.h>  
 #include <signal.h>  
 #include <netdb.h>  
 #include <math.h>  
 #include <time.h>  
 #define BACKLOG 150      //Number of pending connections queue will hold  
 #define MAXDATASIZE 100 //Maximum number of bytes we can get at once  
 #define MAXLINE 750  
 #define TRUE 1  
 #define FALSE 0  
 #define noproc 4     //Total number of sites in the system  
 pthread_t tid1,tid2,tid3;  
 pthread_t proc1[5];  
 int argc1;  
 char argv1[50];  
 char argv[50];  
 int i;  
 int listenPort;     //The process port on which it is recieving the messages  
 int count[25];  
 int serverFlag = 0; //flag to check if all servers/sites are ready  
 int requesttime[5];     //times at which the request message is sent  
 struct host          //Structure to maintain the Id, Server name and Port number  
 {  
      int id;  
      char name[50];  
      int port;  
 };  
 struct host hs[20];  
 typedef struct myinfo1 //Structure to maintain my information  
 {  
      int id;  
      int portno;  
      char mac[50]; //machine or host name eg. net06  
 } myinfo;  
 myinfo my;  
 struct message     //Structure that comtains the message exchanged  
 {  
      int id;               //site ID  
      int procid;          //Process ID  
      char type[10];     //Type of message sent  
      int seq_no;          //sequence number of the process  
      int clock;          //clock at which the message is sent  
 };  
 static int rfront=-1,rrear=-1; //The pointers for REQ_QUEUE  
 static int dfront=-1,drear=-1; //The pointers for the DEFER_QUEUe  
 static int pfront=-1,prear=-1; //The pointers for the PROCESS_QUEUE  
 struct message REQ_QUEUE[200];  //The REQUEST QUEUE  
 struct message DEFER_QUEUE[200]; //The DEFER QUEUE  
 int PROCESS_QUEUE[200];                    //The PROCESS QUEUE  
 sem_t proc[5];  
 sem_t site;  
 //Mutex varialbes used to lock variuos globally shared variables  
 pthread_mutex_t sequence;  
 pthread_mutex_t inCS;  
 pthread_mutex_t reqCS;  
 pthread_mutex_t ccounter;  
 pthread_mutex_t replycnt;  
 pthread_mutex_t signals;  
 pthread_mutex_t     types;  
 pthread_mutex_t     clk;  
 pthread_mutex_t     sending_mutex;  
 pthread_mutex_t sema;  
 pthread_mutex_t pqueue;  
 pthread_mutex_t processthd;  
 pthread_mutex_t counts;  
 pthread_mutex_t requestq;  
 pthread_mutex_t deferq;  
 pthread_mutex_t refront;  
 pthread_mutex_t rerear;  
 pthread_mutex_t defront;  
 pthread_mutex_t derear;  
 //The threads used in this program  
 void * recv_reply_thread ( void *);  
 void * recv_request_thread ( void *);  
 void * process_thread (void *);  
 void * processes (void *);  
 void send_reply(struct message *msg);     //Function to send reply messages  
 void rinsert(struct message);     //Request queue functions  
 void rdisplay(void);  
 struct message rdelete(void);  
 void dinsert(struct message);     //Defer queue functions  
 void ddisplay(void);  
 struct message ddelete(void);  
 void pinsert(int);                    //Process queue functions  
 void pdisplay(void);  
 int pdelete();  
 void sigchld_handler(int s)     // reap all dead processes  
 {  
   while(wait(NULL) > 0);  
 }  
 int me;                         //my id number  
 int our_seq_number=0;     // My sequence number  
 int outstanding_reply_count = noproc-1;     //outstanding reply count..Initially N-1  
 int counter=0;          // counter for clock  
 int clockvalue=1;  
 int highest_sequence_number=0;  
 int counting=0;  
 int req_CS=0; // Request for the Critical section: initially FALSE  
 int in_CS=0; //Inside the Critical Section: initially FALSE  
 int SIGNAL;  
 int in;               //to read if in CS  
 int req;          // to read if req CS  
 int seqno;          // to read seq no  
 int sendcount;  
 int recvcount;  
 int replycount; //to read current outstanding_reply_count  
 // SAVE CONNECTION - RECV  
 void saveconn(int sockfdr, int id, int counter)  
 {  
      int n;  
      FILE *file;  
   char line[MAXLINE];  
   struct message * msg, m;  
      pthread_mutex_lock(&sending_mutex);  
   msg = (struct message *)malloc(sizeof(struct message));  
      n = recv(sockfdr,(void *)msg,sizeof(struct message),0);  
      pthread_mutex_unlock(&sending_mutex);  
      m = *((struct message *)msg);  
   if(n == 0)  
           return;  
   else if(n < 0)  
        printf("saveconn(): read error\n");  
   else  
             printf("Site %d receiving %s from site %d with clock %d. \n",me,m.type,m.id,m.clock);  
           if(highest_sequence_number < m.seq_no)  
           {  
                highest_sequence_number = m.seq_no;  
           }  
           else  
           highest_sequence_number = highest_sequence_number;  
        pthread_mutex_lock(&clk);  
             clockvalue++;  
                if(clockvalue < (m.clock+1))  
                {  
                     clockvalue = (m.clock+1);  
                }  
           pthread_mutex_unlock(&clk);  
        if(strcmp(m.type,"REQUEST") == 0)  
      {  
           printf("Recieving REQUEST message from site %d\n",m.id);  
           pthread_mutex_lock(&requestq);  
                     rinsert(m);  
                    rdisplay();  
        pthread_mutex_unlock(&requestq);  
           SIGNAL=1; //Process wakeup  
      }  
      else if(strcmp(m.type,"REPLY") == 0)  
      {  
           printf("Recieving REPLY message from site %d\n",m.id);  
           pthread_mutex_lock(&replycnt);  
                replycount++;  
           printf("CURRENT REPLYCOUNT : %d\n",replycount);  
           pthread_mutex_unlock(&replycnt);  
      }  
      else  
      {  
           printf("Improper message : message not received properly\n");  
           rdisplay();  
      }  
 }  
 // CLIENT CONNECTION - SEND  
 void cliconn(FILE *file,int sockfds, char *mac, int portno, int id,struct message *messg,int counter)  
 {  
      int n,i;  
      char sendline[400],recvline[MAXLINE + 1];  
      portno = my.portno;  
      pthread_mutex_lock(&clk);  
           messg->clock = clockvalue;  
      pthread_mutex_unlock(&clk);  
      if(send(sockfds,messg,sizeof(struct message),0) != sizeof(struct message))  
           printf("cliconn(): write error on socket\n");  
      printf("Site %d has sent the request message..\n",me);  
 }  
 // MAIN FUNCTION  
 int main(int argc, char **argv)  
 {  
      struct message *msg;  
      int s;  
      pthread_mutex_init(&sequence,NULL);  
      pthread_mutex_init(&inCS,NULL);  
      pthread_mutex_init(&reqCS,NULL);  
      pthread_mutex_init(&ccounter,NULL);  
      pthread_mutex_init(&replycnt,NULL);  
      pthread_mutex_init(&signals,NULL);  
      pthread_mutex_init(&types,NULL);  
      pthread_mutex_init(&clk,NULL);  
      pthread_mutex_init(&sending_mutex,NULL);  
      pthread_mutex_init(&sema,NULL);  
      pthread_mutex_init(&processthd,NULL);  
      pthread_mutex_init(&pqueue,NULL);  
      pthread_mutex_init(&counts,NULL);  
      pthread_mutex_init(&requestq,NULL);  
      pthread_mutex_init(&deferq,NULL);  
      pthread_mutex_init(&refront,NULL);  
      pthread_mutex_init(&rerear,NULL);  
      pthread_mutex_init(&defront,NULL);  
      pthread_mutex_init(&derear,NULL);  
      FILE *file;  
      file = fopen("config.txt", "r"); //Open the configuration file  
        if(file==NULL)  
      {  
           printf("Error: can't open file.\n");  
          return 1;  
      }  
      else  
           printf("File opened successfully.\n");  
      for(i=1;i<=noproc;i++)  
      {  
           fscanf(file,"%d",&hs[i].id);//Reading host info from config file  
           fscanf(file,"%s",hs[i].name);  
           fscanf(file,"%d",&hs[i].port);  
      }  
      argc1 = argc;  
      printf("%d %d",argc1,argc);  
      my.id = atoi(argv[1]);  
      me = my.id;  
      strcpy(my.mac,argv[2]);  
      char t[9];  
      strcpy(t,argv[3]);  
      my.portno = atoi(t);  
      listenPort = atoi(t);  
      printf("My ID is : %s My Port : %s and My IP %s\n",argv[1],argv[3],argv[2]);  
      printf("Configuration File\n"); //Printing the configuration file details  
      for(i=1;i<=noproc;i++)  
      {  
           printf("%d %s %d\n",hs[i].id,hs[i].name,hs[i].port);  
      }  
      fclose(file);  
      for(s=0;s<5;s++)  
      {  
           sem_init( &proc[s],0,0);  
      }  
      for(s=0;s<5;s++)  
      {  
           pthread_create( &proc1[s], NULL, &processes, (void *)s);     //Creating processes in site  
      }  
      pthread_create( &tid3, NULL, &recv_request_thread, &msg);     //Creating send thread  
      pthread_create( &tid2, NULL, &recv_reply_thread, &msg);     //Creating recieve thread  
      pthread_create( &tid1, NULL, &process_thread, &msg);     //Creating process thread  
      pthread_join( tid1, NULL );                     //Join all process threads  
      pthread_join( tid2, NULL );                     //Join all recieve reply threads  
      pthread_join( tid3, NULL );                         //Join all recieve request threads  
      for(s=0;s<5;s++)  
      {  
           pthread_join( proc1[s], NULL);               //Join all processes in the site  
      }  
 }  
 //     RECIEVE REQUESTS THREAD  
 void * recv_request_thread(void *msg)  
 {  
        struct sockaddr_in their_addr; // Connector's address information  
        struct hostent *h;  
        int sockfds;  
        int pid;  
      int j;  
      int check, procid;  
      struct message m;  
      struct message tm;  
      m = *((struct message *)msg);  
      for(j=0;j<noproc; j++)  
      {  
           count[j];  
      }  
        if (argc1 != 4)      //The command line should have the output file,machine name and  
                                    //my port address as the runtime parameters  
        {                          //Error check the command line  
           fprintf(stderr,"usage: getip address\n");  
           exit(1);  
   }  
      int liveServers = 1;     //Initialising number of live processes counting for itself  
      while(liveServers <= noproc)  
                     //Checks for number of processes that are alive before sending  
                     //the messages. It is similar to the initialization message sent  
                     //to all the proceses  
      {  
           liveServers = 1;  
           int j;  
           for(j=1;j<=noproc;j++)  
           {  
                if ((sockfds = socket(AF_INET, SOCK_STREAM, 0)) == -1)  
                     //Opens a connection to check for the live processes  
                  {  
                         perror("socket");  
                         exit(1);  
                  }  
                  if ((h=gethostbyname(hs[j].name)) == NULL)  
                  {  
                     perror("gethostbyname");  
                     exit(1);  
                  }  
                 their_addr.sin_family = AF_INET;  
                 their_addr.sin_port = htons(hs[j].port);  
                  their_addr.sin_addr = *((struct in_addr *)h->h_addr);  
                    memset(&(their_addr.sin_zero), '\0', 8);  
                 if (connect(sockfds, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1)                //Connects to the process  
                {  
                }  
                else  
                {  
                     liveServers++;  
                     //if connection is setup increments liveserver count by one  
                     //everytime it extablishes a connection with a process  
                }  
                close(sockfds); //Connection closed after checking is done  
           }  
      }  
      serverFlag = 1;           //When all processes are alive sets serverFlag to 1.  
      printf("\nALL SERVERS ARE READY!!! \n");     //Processes are ready to listen now.  
 /*REQUESTING ENTRY TO THE CRITICAL SECTION*/  
      while(1)  
      {  
           if(pfront==-1)  
           {  
                check = 0;  
                break;  
           }  
           else  
                check = 1;  
      if(check)  
      {  
      pthread_mutex_lock(&processthd);  
           procid = pdelete();  
           printf("SITE %d REQUESTING FOR CS..\n",me);  
           pthread_mutex_lock(&reqCS);  
                req_CS = 1;  
                req = req_CS;  
           pthread_mutex_unlock(&reqCS);  
      //preparing the structure for sending  
           m.id = me;  
      pthread_mutex_lock(&types);  
           strcpy(m.type,"REQUEST");  
      pthread_mutex_unlock(&types);  
      pthread_mutex_lock(&sequence);  
           our_seq_number = highest_sequence_number+1;  
           m.seq_no = our_seq_number;  
      pthread_mutex_unlock(&sequence);  
      for(i=1; i<=noproc; i++)  
      {  
                if(i == me)               //Checking request not sending to myself  
                {  
                     continue;  
                }  
                if ((h=gethostbyname(hs[i].name)) == NULL)  
             {  
                     perror("gethostbyname");  
                     exit(1);  
             }  
                if ((sockfds = socket(AF_INET, SOCK_STREAM, 0)) == -1)  
                               //Opens socket to send messages  
              {  
                    perror("socket");  
                    exit(1);  
              }  
                their_addr.sin_family = AF_INET;         // Host byte order  
               their_addr.sin_port = htons(hs[i].port); // Short,networbyteorder  
                 their_addr.sin_addr = *((struct in_addr *)h->h_addr);  
                   memset(&(their_addr.sin_zero), '\0', 8); // Zero the rest of the struct  
                  sleep(1);  
                 if (connect(sockfds, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1)  
                   {  
                     perror("connect in send thread\n");  
                    exit(1);  
                  }  
                printf("Clock value is updated to %d\n",clockvalue);  
                clockvalue=clockvalue+1;  
                m.clock=clockvalue;  
                requesttime[i]=m.clock;  
                counter = counter + 1;  
                requesttime[i]=m.clock;  
                printf("Site %d sending REQUEST for process %d to site %d with timestamp %d\n",me,procid,i,m.clock);  
                cliconn(stdin, sockfds,my.mac,my.portno,my.id,&m,counter);  
                close(sockfds);  
      }  
      printf("Waiting for reply from other sites...");  
      while(1)  
      {  
           if (replycount == outstanding_reply_count)  
           {  
                pthread_mutex_lock(&replycnt);  
                replycount=0;  
                pthread_mutex_unlock(&replycnt);  
                break;  
           }  
           else  
                sleep(2);  
      }  
 /*ENTERING THE CRITICAL SECTION*/  
      //Is entering inside the CS  
      pthread_mutex_lock(&inCS);  
           in_CS = 1;  
           in = in_CS;  
      pthread_mutex_unlock(&inCS);  
      //Is not requesting for CS again  
      pthread_mutex_lock(&reqCS);  
           req_CS = 0;  
           req = req_CS;  
      pthread_mutex_unlock(&reqCS);  
      sem_post(&proc[procid]);  
      sem_wait(&site);  
      //Entering CS  
      pthread_mutex_lock(&inCS);  
           in_CS = 0;  
           in = in_CS;  
      pthread_mutex_unlock(&inCS);  
 /*RELEASING THE CRITICAL SECTION*/  
      sendcount = 0;  
      // Pop from the defer queue  
           while(drear!=-1)  
      {  
        pthread_mutex_lock(&types);  
                strcpy(m.type,"REPLY"); //copy my node id and the message type  
           pthread_mutex_unlock(&types);  
           m.id = me;  
           pthread_mutex_lock(&deferq);  
                tm = ddelete(); //tm is the buffer in which the values are stored in message  
                pid = tm.id;  
           pthread_mutex_unlock(&deferq);  
           sendcount++;  
           printf("Send Reply Message count: %d\n",sendcount);  
           if ((h=gethostbyname(hs[pid].name)) == NULL)  
        {  
                perror("gethostbyname");  
                exit(1);  
        }  
           if ((sockfds = socket(AF_INET, SOCK_STREAM, 0)) == -1)  
                               //Opens socket to send messages  
         {  
                    perror("socket");  
                     exit(1);  
         }  
            their_addr.sin_family = AF_INET;         // Host byte order  
           their_addr.sin_port = htons(hs[pid].port); // Short,networbyteorder  
            their_addr.sin_addr = *((struct in_addr *)h->h_addr);  
               memset(&(their_addr.sin_zero), '\0', 8); // Zero the rest of the struct  
      sleep(1);  
            if (connect(sockfds, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1)  
              {  
                     perror("connect in send thread\n");  
                      //exit(1);  
             }  
           cliconn(stdin, sockfds,my.mac,my.portno,my.id,&m,0);  
           close(sockfds);  
      }  
           pthread_mutex_unlock(&processthd);  
      }  
 }  
 }  
 //      RECIEVE REPLYS THREAD  
 void * recv_reply_thread(void *msg)  
 {  
      int sockfdr, new_fd;                  // Listen on sock_fd, new connection on new_fd  
   struct sockaddr_in my_addr;             // My address information  
        struct sockaddr_in their_addr;           // Connector's address information  
        int sin_size;  
      int yes=1;  
      FILE *file;  
      struct message m;  
      m = *((struct message *)msg);  
   if ((sockfdr = socket(AF_INET, SOCK_STREAM, 0)) == -1) //Opening socket connection  
   {  
     perror("socket");                // Checking for any in case if connection failed  
     exit(1);  
   }  
   if (setsockopt(sockfdr, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)  
   {  
     perror("setsockopt");  
     exit(1);  
   }  
   my_addr.sin_family = AF_INET;         // Host byte order  
   my_addr.sin_port = htons(listenPort);        // Short, network byteorder  
   my_addr.sin_addr.s_addr = (INADDR_ANY);      // Automatically fill with myIP  
   memset(&(my_addr.sin_zero), '\0', 8);      // Zero the rest of the struct  
   if (bind(sockfdr, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)  
   {                              // Bind to my address  
     perror("bind");                    // Check for errors  
     exit(1);  
   }  
   if (listen(sockfdr, BACKLOG) == -1)      // Listening from the other processes  
   {  
     perror("listen");               // Checking for errors  
     exit(1);  
   }  
      for(;;)  
      {  
           int numbytes;  
           char buf[MAXDATASIZE];  
           sin_size = sizeof(struct sockaddr_in);  
           if ((new_fd = accept(sockfdr, (struct sockaddr*)&their_addr, (socklen_t*)&sin_size)) == -1)  
           {  
          perror("In server accept");  
          continue;  
           }  
           else  
           {  
                saveconn(new_fd,my.id,counter);  
                close(new_fd);  
           }  
      }  
 }  
 //     THE SITE CONTROLLER THREAD  
 void * processes(void *msg)  
 {  
      int pid,mycount;  
      pid = (int)msg;  
      for(mycount=1; mycount<=20 ; mycount++)  
      {  
                pthread_mutex_lock(&pqueue);  
                     pinsert(pid);  
            counting++;  
                 pdisplay();  
          pthread_mutex_unlock(&pqueue);  
                sem_wait(&proc[pid]);  
                //entering crictical section  
                printf("Starting CS execution at time : %ld\n",time(NULL));  
                printf("*********SITE %d PROCESS %d ENTERING THE CS*********\n",me,pid);  
          printf("*********INSIDE THE CS*********\n");  
                      sleep(1);  
          printf("*********SITE %d PROCESS %d EXITING THE CS*********\n",me,pid);  
          printf("Exiting CS at time : %ld\n",time(NULL));  
                sem_post(&site);  
                printf("\nProcess %d is in CS for %d times\n",pid,mycount);  
      }  
      printf("*** Total Message count: %d ***\n",counting);  
 }  
 //     THE PROCESSING THREAD  
 void * process_thread(void *msg)  
 {  
      int nodeseq;  
      int pid; //use it for ripping the process to b sent to  
      struct message m;  
      m = *((struct message *)msg);  
      while(1)  
      {  
      printf("Process thread starting to process requests..\n ");  
      sleep(3);  
      while(SIGNAL == 1)  
      {  
      pthread_mutex_lock(&refront);  
      while(1)  
       {  
                //pop data from the request queue  
           if(rfront!=-1)  
           {  
                rdisplay();  
                pthread_mutex_lock(&inCS);  
                     in = in_CS;  
                pthread_mutex_unlock(&inCS);  
                pthread_mutex_lock(&reqCS);  
                     req = req_CS;  
                pthread_mutex_unlock(&reqCS);  
             pid=REQ_QUEUE[rfront].id;  
                nodeseq = REQ_QUEUE[rfront].seq_no;  
                pthread_mutex_lock(&requestq);  
                     m = rdelete();  
                pthread_mutex_unlock(&requestq);  
                if (in == 1)  
                {  
                     printf("PROCESS ALREADY IN CS..So putting in defer queue.!!!\n");  
                     pthread_mutex_lock(&deferq);  
                          dinsert(m);  
                          pthread_mutex_unlock(&deferq);  
                }  
             else if( in == 0)  
             {  
                if (req == 1)  
                {  
                     printf("SITE %d REQUESTING FOR CS AS WELL..\nSo tie break..between %d and %d..!!\n",pid,me,pid);  
                     pthread_mutex_lock(&sequence);  
                          seqno = our_seq_number;  
                     pthread_mutex_unlock(&sequence);  
                     printf("Their seqno: %d My seqno: %d Their ID: %d My ID: %d\n",nodeseq,seqno,pid,me);  
                     if ((nodeseq < seqno) || (nodeseq == seqno && pid < me))  
                     {  
                          m.id = pid;  
                          m.seq_no = 0;  
                          strcpy(m.type, "REPLY");  
                          send_reply(&m); //send reply to that node with my structure (node id and type)  
                     }  
                     else  
                     {  
                          printf("@@@@@ I WIN @@@@@ \nNode %d get the priority...So put %d in the defer queue..!\n",me,pid);  
                          pthread_mutex_lock(&deferq);  
                               dinsert(m);  
                          pthread_mutex_unlock(&deferq);  
                     }  
                 }  
                  else  
                {  
                     m.id = pid;  
                     m.seq_no = 0;  
                     strcpy(m.type, "REPLY");  
                     send_reply(&m); //send reply to that node with my structure (node id and type)  
                }  
               }  
                else  
           {  
                sleep(5);  
           }  
           }  
       }  
       }  
      pthread_mutex_unlock(&refront);  
      pthread_mutex_lock(&signals);  
           SIGNAL = 0;  
      pthread_mutex_unlock(&signals);  
      }  
 }  
 // SEND REPLYS FUNCTION  
 void send_reply(struct message *msg)  
 {  
   struct sockaddr_in their_addr; // Connector's address information  
   struct hostent *h;  
   int sockfds;  
      int pid;  
      struct message m;  
      m = *((struct message *)msg);  
      pid =m.id;  
      pthread_mutex_lock(&types);  
           strcpy(m.type,"REPLY");  
      pthread_mutex_unlock(&types);  
      m.id = me;  
      if ((h=gethostbyname(hs[pid].name)) == NULL)  
        {  
                perror("gethostbyname");  
                exit(1);  
      }  
      if ((sockfds = socket(AF_INET, SOCK_STREAM, 0)) == -1)  
           //Opens socket to send messages  
        {  
                perror("socket");  
                exit(1);  
        }  
      their_addr.sin_family = AF_INET;         // Host byte order  
   their_addr.sin_port = htons(hs[pid].port); // Short,networbyteorder  
      their_addr.sin_addr = *((struct in_addr *)h->h_addr);  
        memset(&(their_addr.sin_zero), '\0', 8); // Zero the rest of the struct  
   sleep(1);  
      if (connect(sockfds, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1)  
        {  
           perror("connect in send thread\n");  
         exit(1);  
       }  
      cliconn(stdin, sockfds,my.mac,my.portno,my.id,&m,0);  
      printf("Sending REPLY message from site %d to site %d\n",me,pid);  
      close(sockfds);     //Socket closed after sending the message to the process  
 }  
 // THE REQUEST QUEUE  
 //REQUEST QUEUE INSERT  
 void rinsert(struct message temp)  
 {  
      printf("Inside the request queue insert..\n");  
      if(rfront==rrear)  
   {  
           rfront = 0;  
           rrear = 0;  
   }  
      printf("Inserting values in request queue..\n");  
      REQ_QUEUE[rrear].id = temp.id;  
   strcpy(REQ_QUEUE[rrear].type,temp.type);  
   REQ_QUEUE[rrear].seq_no = temp.seq_no;  
   REQ_QUEUE[rrear].clock = temp.clock;  
      rrear++;  
 }  
 //REQUEST QUEUE DISPLAY  
 void rdisplay()  
 {  
   int i;  
   if(rfront==-1)  
   printf("CAUTION: Request Queue is Empty..!!\n");  
   for(i=rfront;i<rrear;i++)  
   printf("%d %s %d %d\n",REQ_QUEUE[i].id,REQ_QUEUE[i].type,REQ_QUEUE[i].seq_no,REQ_QUEUE[i].clock);  
 }  
 //REQUEST QUEUE DELETE  
 struct message rdelete()  
 {  
   struct message tempvar;  
   printf("Inside request queue delete.. \n");  
   rdisplay();  
   if(rfront==-1)  
   {  
        printf("CAUTION: Request Queue Underflow !!\n");  
           exit(1);  
   }  
   else if(rfront==rrear-1)  
   {  
   tempvar.id = REQ_QUEUE[rfront].id;  
   strcpy(tempvar.type,REQ_QUEUE[rfront].type);  
   tempvar.seq_no = REQ_QUEUE[rfront].seq_no;  
   tempvar.clock = REQ_QUEUE[rfront].clock;  
      rfront = -1;  
      rrear = -1;  
      }  
   else  
   {  
   tempvar.id = REQ_QUEUE[rfront].id;  
   strcpy(tempvar.type,REQ_QUEUE[rfront].type);  
   tempvar.seq_no = REQ_QUEUE[rrear].seq_no;  
   tempvar.clock = REQ_QUEUE[rfront].clock;  
      rfront++;  
   }  
   return tempvar;  
 }  
 // THE DEFER QUEUE   
 //DEFER QUEUE INSERT  
 void dinsert(struct message temp)  
 {  
      printf("Inside the defer queue insert..\n");  
      if(dfront==drear)  
   {  
           dfront = 0;  
           drear = 0;  
      }  
   DEFER_QUEUE[drear].id = temp.id;  
   strcpy(REQ_QUEUE[drear].type,temp.type);  
   DEFER_QUEUE[drear].seq_no = temp.seq_no;  
   DEFER_QUEUE[drear].clock = temp.clock;  
      drear++;  
 }  
 //DEFER QUEUE DISPLAY  
 void ddisplay()  
 {  
   int i;  
   if(dfront==-1)  
   printf("Defer Queue is Empty..!!\n");  
   for(i=dfront;i<drear;i++)  
   printf("%d %s %d %d\n",DEFER_QUEUE[i].id,DEFER_QUEUE[i].type,DEFER_QUEUE[i].seq_no,DEFER_QUEUE[i].clock);  
 }  
 //DEFER QUEUE DELETE  
 struct message ddelete()  
 {  
   struct message tempvar;  
   printf("Inside the defer queue delete..\n");  
   ddisplay();  
   if(dfront==-1)  
   {  
           printf("CAUTION: Defer queue Underflow !!\n");  
           exit(1);  
   }  
   else if(dfront==drear-1)  
   {  
   tempvar.id = DEFER_QUEUE[dfront].id;  
   strcpy(tempvar.type,DEFER_QUEUE[dfront].type);  
   tempvar.seq_no = DEFER_QUEUE[drear].seq_no;  
   tempvar.clock = DEFER_QUEUE[dfront].clock;  
      dfront = -1;  
      drear = -1;  
   }  
   else  
   {  
   tempvar.id = DEFER_QUEUE[dfront].id;  
   strcpy(tempvar.type, DEFER_QUEUE[dfront].type);  
   tempvar.seq_no = DEFER_QUEUE[drear].seq_no;  
   tempvar.clock = DEFER_QUEUE[dfront].clock;  
      dfront++;  
   }  
   return tempvar;  
 }  
 // THE PROCESS QUEUE   
 //PROCESS QUEUE INSERT  
 void pinsert(int temp)  
 {  
      printf("Inside the process queue insert..\n");  
      if(pfront==prear)  
      {  
           pfront = 0;  
           prear = 0;  
      }  
      PROCESS_QUEUE[prear] = temp;  
      prear++;  
 }  
 //PROCESS QUEUE DISPLAY  
 void pdisplay()  
 {  
      int i;  
      if(pfront==-1)  
      printf("Process Queue is Empty\n");  
      for(i=pfront;i<prear;i++)  
      printf("%d \n",PROCESS_QUEUE[i]);  
 }  
 //PROCESS QUEUE DELETE  
 int pdelete()  
 {  
      int tempvar;  
      printf("Inside process queue delete..\n");  
            pdisplay();  
      if(pfront==-1)  
      {  
        printf("CAUTION: Process Queue Underflow !!\n");  
        exit(1);  
      }  
      else if(pfront==prear-1)  
      {  
      tempvar = PROCESS_QUEUE[pfront];  
           pfront = -1;  
           prear = -1;  
      }  
      else  
      {  
      tempvar = PROCESS_QUEUE[pfront];  
      pfront++;  
      }  
      return tempvar;  
 }  
