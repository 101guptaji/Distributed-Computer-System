#include<iostream>
#include<semaphore.h>
#include<pthread.h>  
#include<string>
#include<fstream> 
using namespace std;


int shareV = 1, rcnt = 0, wcnt = 0;
sem_t wsem;
sem_t x,y;
sem_t rsem;

void* reader(void *rno){
    sem_wait(&rsem);
	sem_wait(&x);

    	rcnt++;
    	if(rcnt==1){
        	sem_wait(&wsem);
    	}
	sem_post(&x);
    sem_post(&rsem);

    //critical section started

    ifstream fin;
    fin.open("writepro.txt"); 
    string line;

    cout <<"\nReader"<<*((int *)rno)<<" is reading: \n";
    
    while(fin){ 
	getline(fin, line);  // Reading a Line from File
        cout << line << endl;  
    } 
   
    fin.close();
    //critical section ended

    sem_wait(&x);
    rcnt--;
    if(rcnt==0){
        sem_post(&wsem);
    }
    sem_post(&x);

}


void* writer(void* wno){

    sem_wait(&y); 
    wcnt++;
    if(wcnt == 1){
        sem_wait(&rsem);
    }
    sem_post(&y);

    //critical section started
    sem_wait(&wsem);

	shareV*=2;
	ofstream fout;
	fout.open("writepro.txt",ios::trunc); // ios::trunc mode delete all conetent before open 
	fout << "share variable= " << shareV << "\n";
	cout<<"\nwriter"<<*((int *)wno)<<" has modified share variable to "<<shareV<<endl; 
	fout.close();

    sem_post(&wsem);
    //critical section ended

    sem_wait(&y); 
    wcnt--;
    if(wcnt == 0){
        sem_post(&rsem);
    }
    sem_post(&y);
}

int main(){

    pthread_t read[5], wrt[5]; //five threads for both reader and writer
    sem_init(&wsem, 0, 1);  //initialisation of semaphore wsem
    sem_init(&rsem, 0, 1);  //initialisation of semaphore rsem
    sem_init(&x, 0, 1);   //initialisation of semaphore x
    sem_init(&y, 0, 1);   //initialisation of semaphore y

    
    int a[5] = {1,2,3,4,5};
    for(int i = 0 ; i < 5 ; i++){ 
	pthread_create(&wrt[i],NULL,writer,(void *)&a[i]);
	pthread_create(&read[i],NULL,reader,(void *)&a[i]);
    }

    for(int i = 0 ; i < 5 ; i++){
        pthread_join(read[i],NULL); 
        pthread_join(wrt[i],NULL);
    }

    sem_destroy(&wsem); 
    sem_destroy(&rsem);
    sem_destroy(&x);
    sem_destroy(&y);  

}


