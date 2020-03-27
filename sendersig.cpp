/*AUTHORS
--------------------------------------
Janelle Estabillo
estabillojanelle@csu.fullerton.edu
CPSC 351-02

Nidhi Shah
nidhi989@csu.fullerton.edu
CPSC 351-02*/
//Sender attach with shared memory segment.
//Sender open the file, read the file data, store in shared memory buffer,
//save the data size in shared memory stuct variable - msgsize,
//changed the variable status to READY and send the receiver the signal of SIGUSR2
//and wait for receiver signal - which will be SIGUSR1,
//the handler will handle the signal and sender will set the STATUS variable as DONE,
//to let receiver that is not sending more message.with setting msgsize =0
//deallocate the shared memory.
//close the file.
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

using namespace std;

/* The size of the shared memory chunk */
#define SHARED_MEMORY_CHUNK_SIZE 1000
#define READY 1
#define RECIEVED 2
#define DONE -1
/* struct of sharedmemory*/
struct memory{
  char buff[SHARED_MEMORY_CHUNK_SIZE];
  int pids,pidr,msgsize,status;
};
/* The ids for the shared memory segment and the message queue */
int shmid;
int r =0;
/* The pointer to the shared memory */
struct memory *sharedMemPtr;
/* handler for sender.. received the signal SIGUSR1
   from receiver, than set variable r =1 to allow sender deallocate memory */
 void handlersend(int signum)
 {
   	if(signum == SIGUSR1)
   	{
        if(sharedMemPtr->status == RECIEVED)
        {
          r = 1;
        }
   	}
 }

 /**
  * Sets up the shared memory segment
  * @param shmid - the id of the allocated shared memory
  */
void init(int& shmid, struct memory*& sharedMemPtr)
{
    	/* TODO:
            1. Create a file called keyfile.txt containing string "Hello world" (you may do
     		    so manually or from the code).
    	    2. Use ftok("keyfile.txt", 'a') in order to generate the key.
    		3. Use the key in the TODO's below. Use the same key for the
          shared memory segment. This also serves to illustrate the difference
    		    between the key and the id used in shared memory. The id
    		    for any System V objest (i.e. message queues, shared memory, and sempahores)
    		    is unique system-wide among all SYstem V objects. Two objects, on the other hand,
    		    may have the same key.
    	 */

            //generating key for keyfile.txt for further use
          key_t key = ftok("keyfilesig.txt",'a');
          //checking error
    			if(key == -1)
    	 	 {
    	 	    perror("EROR:: generating key");
    	 			exit(1);
    	 	 }


      	/* TODO: Get the id of the shared memory segment. The size of the segment must be SHARED_MEMORY_CHUNK_SIZE */
      	/* TODO: Attach to the shared memory */
      	/* Store the IDs and the pointer to the shared memory region in the corresponding parameters */

         //get id of segment
      	shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE,0666);
        //checking error
      	if(shmid == -1)
      	{
      		perror("ERROR:: shared segment ");

      		exit(1);
      	}

      	//attach with segment with reading and writing permission
        sharedMemPtr = (struct memory*)shmat(shmid,NULL,0);
        //checking error
      	if(sharedMemPtr == (memory*)-1)
      	{
      		perror("ERROR:: not attached with segment");
      		exit(1);
      	}

        //getting the sender process id
        //no need to check error cause getpid() never throw error
         sharedMemPtr->pids = getpid();
}

/**
 * Performs the cleanup functions
 * @param sharedMemPtr - the pointer to the shared memory
 * @param shmid - the id of the shared memory segment
 */

void cleanUp(const int& shmid, struct memory* sharedMemPtr)
{
  	/* TODO: Detach from shared memory */
  	// Detaching from our Shared Memory
  	if (shmdt(sharedMemPtr) == -1)
    {
  		perror("shmdt"); // If Shared memory failed to detach
  		exit(-1);
  	}
}


// /**
//  * The main send function
//  * @param fileName - the name of the file
//  */
void send(const char* fileName)
{

	/* Open the file for reading */
	FILE* fp = fopen(fileName, "r");

	/* Was the file open? */
	if(!fp)
	{
		perror("fopen");
		exit(-1);
	}

	/* Read the whole file */
	while(!feof(fp))
	{

    		/* Read at most SHARED_MEMORY_CHUNK_SIZE from the file and store them in shared memory.
     		 * fread will return how many bytes it has actually read (since the last chunk may be less
     		 * than SHARED_MEMORY_CHUNK_SIZE).
     		 */
    		if((sharedMemPtr->msgsize = fread(sharedMemPtr, sizeof(char), SHARED_MEMORY_CHUNK_SIZE, fp)) < 0)
    		{
          //checking error
    			perror("fread");
    			exit(-1);
    		}

        //now change the status variable in memory for reciever and call kill
        //status - READY - means -data is ready on shared memory
        sharedMemPtr->status = READY;
        //sending signal to RECEIVER -SIGUSR2
        kill(sharedMemPtr->pidr,SIGUSR2);
  }

  //will wait for handler to ser r=1, as Sender receive signal from Receiver
  while(r!=1)
       {
         //no message from recevier ..wait..
         continue;
       }

 //if r=1 , means set by handler, means reciever's signal came.
 //now do not want to send message, so change status to DONE, to let recevier know that,
 // no more sharing data coming can detach the shared memory segment
 // and for that send signal to receiver
  if(r==1)
       {
         //change the status to DONE
         sharedMemPtr->status = DONE;
         //set messagesize 0
         sharedMemPtr->msgsize =0;
         //send the signal to receiver
         kill(sharedMemPtr->pidr,SIGUSR2);
       }


  /* Close the file */
  fclose(fp);
}

// main function
int main(int argc, char** argv)
{

  //settign if the signal SIGUSR1 recieved, handle by handlersend
  signal(SIGUSR1,handlersend);
	/* Check the command line arguments */
	 if(argc < 2)
	 {
      	fprintf(stderr, "USAGE: %s <FILE NAME>\n", argv[0]);
	 	exit(-1);
	 }

	/* Connect to shared memory and the message queue */
	init(shmid,sharedMemPtr);


	// /* Send the file */
	send(argv[1]);

	/* Cleanup */
  cleanUp(shmid,sharedMemPtr);

	return 0;
}
