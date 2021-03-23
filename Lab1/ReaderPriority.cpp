//Reader priority: no reader should wait for other readers to finish simply because a writer is waiting.
#include<iostream> 
#include<semaphore.h>
#include<pthread.h>  
#include<string>
#include<fstream> 
using namespace std;

int rcnt  = 0; // To keep count of total readers 
sem_t wsem; // A binary semaphore that will be used both for mutual exclusion and signalling
sem_t x;  // Provides mutual exclusion when readcount is being modified

void* reader(void *rno){

    sem_wait(&x);
    //pre reader count critical section
    rcnt++;
    if(rcnt==1){
        sem_wait(&wsem);
    }
    sem_post(&x);
    
   //critical section started

    ifstream fin;
    fin.open("readpro.txt",ios::in); 
    string line;

     cout <<"\nReader"<<*((int *)rno)<<" is reading: \n";

    while(fin){
	getline(fin, line); // Reading a Line from File
        cout << line<<endl; 
    } 
 
    fin.close();
    //critical section ended

    sem_wait(&x); //post reader count critical section
    rcnt--;
    if(rcnt==0){
        sem_post(&wsem);
    }
    sem_post(&x);
}


void* writer(void* wno){

    sem_wait(&wsem);
     //critical section started

	string name;
	cout<<"Enter ur name to write "; //input from console
	getline(cin,name);
	//cout<<name;
	ofstream fout;
	fout.open("readpro.txt",ios::app); // appending to its existing contents.
	fout <<"Name= "<< name<< "\n";
	cout<<"\nwriter"<<*((int *)wno)<<" has written: "<<name<<endl;
    	fout.close();

    //critical section ended
    sem_post(&wsem); 
}

int main(){
    ofstream fout;
    fout.open("readpro.txt",ios::trunc); //deleting all conetent before write
    fout.close();

	pthread_t read[3], wrt[3]; //three threads for both reader and writer
	sem_init(&x, 0, 1); //initialisation of semaphore x,0: the semaphore is shared among all threads of a process, 1: initial value of semaphore=1
	sem_init(&wsem, 0, 1); //initialisation of semaphore wsem

    int a[3] = {1,2,3}; //Just used for numbering the Reader and Writer.
    
    for(int i = 0 ; i < 3 ; i++){
        pthread_create(&wrt[i],NULL,writer,(void *)&a[i]); //to create a new thread 
	pthread_create(&read[i],NULL,reader,(void *)&a[i]);
    }

    for(int i = 0 ; i < 3 ; i++){
        pthread_join(read[i],NULL); //to wait for the termination of a thread. 
	pthread_join(wrt[i],NULL); 
    }

    sem_destroy(&wsem); 
    sem_destroy(&x); 
}




//references: http://faculty.juniata.edu/rhodes/os/ch5d.htm
//http://faculty.cs.niu.edu/~hutchins/csci480/semaphor.htm
