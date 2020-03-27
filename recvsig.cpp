/*AUTHORS
--------------------------------------
Janelle Estabillo
estabillojanelle@csu.fullerton.edu
CPSC 351-02

Nidhi Shah
nidhi989@csu.fullerton.edu
CPSC 351-02*/
//Receiver create the shared memory segment with shared memory struct size,coneect it,
//Reciever open the file,
//and wait for sender signal, signal hnadler set the variables by checking the status variables
//when it receives the SIGUSR2 signal, Receiver copied the data from the shared memory to recv file.
//After gettign all data, the receiver set the status as RECEIVED and send signal of SIGUSR1 to Sender
//to notify sender , that it received the data, is done with reading shared memory,
//will wait for sender signal, if the Receiver get signal and status variable is DONE, than
//handler allow receiver to deallocate the shared memory and close the file
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
/* struct of sharedmemory*/
#define READY 1
#define RECIEVED 2
#define DONE -1
struct memory{
  char buff[SHARED_MEMORY_CHUNK_SIZE];
  int pids,pidr,msgsize,status;
};
/* The ids for the shared memory segment and the message queue */
int shmid;

//for receiver set by handler
int set=0;
/* The pointer to the shared memory */
struct memory *sharedMemPtr;

/* The name of the received file */
const char recvFileName[] = "recvfile";
/* THe handler set variable set as 1 and 2
    If the status is READY than set =1
    If the status is DONE than set =2
   */
void handler(int signum)
{
 if(signum == SIGUSR2)
 {
    if(sharedMemPtr->status == READY)
    {
        set = 1;
    }

    if(sharedMemPtr->status == DONE)
    {
        set = 2;
    }
 }


}

/**
 * Sets up the shared memory segment and message queue
 * @param shmid - the id of the allocated shared memory
 * @param sharedMemPtr - the pointer to the shared memory
 */

void init(int& shmid, struct memory*& sharedMemPtr)
{

    	/* TODO: 1. Create a file called keyfile.txt containing string "Hello world" (you may do
     		    so manually or from the code).
    	         2. Use ftok("keyfile.txt", 'a') in order to generate the key.
    		 3. Use the key in the TODO's below. Use the same key for the
           shared memory segment. This also serves to illustrate the difference
    		    between the key and the id used in message queues and shared memory. The id
    		    for any System V object (i.e. message queues, shared memory, and sempahores)
    		    is unique system-wide among all System V objects. Two objects, on the other hand,
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

    	/* TODO: Allocate a piece of shared memory. The size of the segment must be SHARED_MEMORY_CHUNK_SIZE. */
    	/* TODO: Attach to the shared memory */
    	/* Store the IDs and the pointer to the shared memory region in the corresponding parameters */

       //get shmid from shmget function.. with the new creation of shared memory, will have id related to the key. if the segment is already exit,
    	 //will recieve -1 as error
    	 shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, IPC_CREAT|0666);
    	 //if it already exit, than get the id related to that key
    	 if(shmid == -1)
    	 {
    		 perror("ERROR:: shared segment already exist for this key");
    		 shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE,0666);
    		 exit(1);
    	 }

    	 //connecting with the shared memory segment
    	sharedMemPtr = (struct memory*)shmat(shmid,NULL,0);
      //checking error
      if(sharedMemPtr == (memory*)-1)
      {
        perror("ERROR:: not attached with segment");
        exit(1);
      }

      //get id for receiver process
      //no need to check error cause getpid() never throw error
      sharedMemPtr->pidr = getpid();

}


/**
 * The main loop
 */
  void mainLoop()
 {
       	cout<< "in main loop"<<endl;

        //setting variable to set receiver idle for a while
        sharedMemPtr->msgsize =1;

    // 	/* Open the file for writing */
      	FILE* fp = fopen(recvFileName, "w");
    //
    // 	/* Error checks */
      	if(!fp)
    	{
    		perror("fopen");
    		exit(-1);
    	}


    // //
    //     /* TODO: Receive the signal and get the message size. The message will
    //      * contain regular information.  If the size field
    //      * of the message is not 0, then we copy that many bytes from the shared
    //      * memory region to the file. Otherwise, if 0, then we close the file and
    //      * exit.
    //      *
    //      * NOTE: the received file will always be saved into the file called
    //      * "recvfile"
    //      */
    //
    // 	/* Keep receiving until the sender set the size to 0, indicating that
    //  	 * there is no more data to send
    //  	 */
    //
    //
    	while(set != 2 && sharedMemPtr->msgsize !=0)
    	{
        //will wait for sender to give signal
        while(set==0)
        {
          //wait for sender... be idle
          continue;
        }
        sleep(1);

        //get the signal from the sender and handler set the set variable to 1
        //to work on shared memory data. as receiving data and storing in file
    		if(set == 1)
    		{
      			/* Save the shared memory to file */
      			if(fwrite(sharedMemPtr->buff, sizeof(char), sharedMemPtr->msgsize, fp) < 0)
      			{
              //check error
      				perror("fwrite");
      			}

      			/* TODO: Tell the sender that we are ready for the next file chunk.
       			 * I.e. send a signal by setting the status as RECEIVED
               and sending signal to sender by SIGUSR1
       			 */
            sharedMemPtr->status = RECIEVED;
      			 //now sending the message that we recieved first one and we are ready for the next one
             //by SIGUSR1 signal
              kill(sharedMemPtr->pids,SIGUSR1);
              //puting set variable 0 again to wait for sender signal
              set = 0;
    		}
    	}
    	/* We are done */
      if(set == 2)
    		{
    		  	/* Close the file */
    			  fclose(fp);
    		}

 }

/**
 * Perfoms the cleanup functions
 * @param sharedMemPtr - the pointer to the shared memory
 * @param shmid - the id of the shared memory segment
 */

void cleanUp(const int& shmid, struct memory* sharedMemPtr)
{
	/* TODO: Detach from shared memory */
	if (shmdt(sharedMemPtr) == -1) {
 	 perror("shmdt"); // If Shared memory failed to detach
 	 exit(-1);
  }
	/* TODO: Deallocate the shared memory chunk */
	if (shmctl(shmid, IPC_RMID, NULL) == -1) {
		perror("shmctl"); //If Shared memory isn't destroyed
		exit(-1); // Exits Program
	}

}

// /**
//  * Handles the exit signal
//  * @param signal - the signal type
//  */
//
void ctrlCSignal(int signal)
{
	/* Free system V resources */
	cleanUp(shmid,sharedMemPtr);
}

//main functions
int main(int argc, char** argv)
{

	/* TODO: Install a singnal handler (see signaldemo.cpp sample file).
 	 * In a case user presses Ctrl-c your program should delete message
 	 * queues and shared memory before exiting. You may add the cleaning functionality
 	 * in ctrlCSignal().
 	 */
  signal(SIGINT, ctrlCSignal);

  //install signal handler for SIGUSR2 to handle the signal for shared memory segment
  signal(SIGUSR2, handler);
	/* Initialize */
	init(shmid,sharedMemPtr);

	/* Go to the main loop */
	mainLoop();

	/** TODO: Detach from shared memory segment, and deallocate shared memory and message queue (i.e. call cleanup) **/
	ctrlCSignal(0);

	return 0;
}
