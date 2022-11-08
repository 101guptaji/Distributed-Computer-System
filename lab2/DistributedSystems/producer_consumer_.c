#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<semaphore.h>
//#include<windows.h>

#define QUEUE_SIZE 5
#define array_size 20

pthread_mutex_t mutex;
sem_t full, empty;

pthread_t producer_tid;
pthread_t consumer1_tid;
pthread_t consumer2_tid;
pthread_attr_t attributes;

char for_consumer_one[array_size];
char for_consumer_two[array_size];
int counter_consumer_one = 0;
int counter_consumer_two = 0;
int counter_read;
int counter_write;
int flag = 0;
FILE * file_pointer;

struct Queue
{ 
	int front, rear, size; 
	unsigned int capacity; 
	char* array; 
}; 

struct Queue * createQueue(unsigned int capacity) 
{
	struct Queue* queue = (struct Queue*) malloc(sizeof(struct Queue)); 
	queue->capacity = capacity; 
	queue->front = queue->size = 0; 
	queue->rear = capacity - 1;
	queue->array = (char *) malloc(queue->capacity * sizeof(char)); 
	return queue; 
}

int isFull(struct Queue* queue)
{ return (queue->size == queue->capacity);} 

int isEmpty(struct Queue* queue) 
{ return (queue->size == 0); }

int enqueue(struct Queue* queue, char item) 
{ 
	if (isFull(queue)) 
		return -1; 
	queue->rear = (queue->rear + 1)%queue->capacity; 
	queue->array[queue->rear] = item; 
	queue->size = queue->size + 1; 
    return 0;
} 

int dequeue(struct Queue* queue, char * item) 
{ 
	if (isEmpty(queue)) 
		return -1; 
	* item = queue->array[queue->front]; 
	queue->front = (queue->front + 1)%queue->capacity; 
	queue->size = queue->size - 1; 
	return 0;
}

struct Queue* queue;

void setup()
{
    pthread_mutex_init(&mutex, NULL);
    sem_init(&full, 0, 0);
    sem_init(&empty, 0, QUEUE_SIZE);
    pthread_attr_init(&attributes);
}

int insert_item(char item)
{
    return enqueue(queue, item);
}

int remove_item(char * item)
{
    return dequeue(queue, item);
}

void *producer(void *param) {
    char item;

    file_pointer = fopen("input.txt", "r");
    while(TRUE) {
        Sleep(0);
        item = getc(file_pointer);
        if (item == EOF)
        {
            flag = 1;
            break;
        }
        sem_wait(&empty);
        pthread_mutex_lock(&mutex);
    
        if(insert_item(item)) {
            printf("Producer cannot write to the queue.\n");
        }
        else {
            counter_read++;
            printf("Producer writes: %c\n", item);
        }
        pthread_mutex_unlock(&mutex);
        sem_post(&full);
    }
    fclose(file_pointer);
}

void *consumer_one(void *param) {
    char item;
    while(TRUE) {
        Sleep(2);
        sem_wait(&full);
        pthread_mutex_lock(&mutex);
        if(remove_item(&item)) {
            printf("Could not red from queue.\n");
        }
        else {
            printf("Consumer_one stored: %c\n", item);
            counter_write++;
            for_consumer_one[counter_consumer_one++] = item;
        }
        pthread_mutex_unlock(&mutex);
        sem_post(&empty);
        if (counter_write == counter_write && flag)
        {
            break;
        }
   }
}

void *consumer_two(void *param) {
    char item;
    while(TRUE) {
        Sleep(10);
        sem_wait(&full);
        pthread_mutex_lock(&mutex);
        if(remove_item(&item)) {
            printf("Could not read from Queue.\n");
        }
        else {
            counter_write++;
            printf("Consumer_two stored: %c\n", item);
            for_consumer_two[counter_consumer_two++] = item;
        }
        
        pthread_mutex_unlock(&mutex);
        sem_post(&empty);
        if (counter_write == counter_read && flag)
        {
            break;
        }
   }
}

void print_array(char array[], int n)
{
    for (int i = 0; i < n; i++)
        printf("%c\t", array[i]);
    printf("\n");
}

int main()
{
    queue = createQueue(QUEUE_SIZE); 
    setup();
    
    pthread_create(&producer_tid, &attributes, producer, NULL);
    pthread_create(&consumer1_tid, &attributes, consumer_one, NULL);
    pthread_create(&consumer2_tid, &attributes, consumer_two, NULL);
    pthread_join(producer_tid, NULL);
    pthread_join(consumer1_tid, NULL);
    pthread_join(consumer2_tid, NULL);
    
    print_array(for_consumer_one, counter_consumer_one);
    print_array(for_consumer_two, counter_consumer_two);
    exit(0);
}
