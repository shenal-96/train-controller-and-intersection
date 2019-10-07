#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/iofunc.h>
#include <sys/netmgr.h>

#define BUF_SIZE 100
//dfgfg
typedef struct
{
	struct _pulse hdr; // Our real data comes after this header
	int ClientID; // our data (unique id from client)
    char data[5];     // our data
} my_data;

typedef struct
{
	struct _pulse hdr; // Our real data comes after this header
    char buf[BUF_SIZE];// Message we send back to clients to tell them the messages was processed correctly.
} my_reply;

typedef struct
{
	int data1;//serverPID
	int data2;//serverCHID
} input_data;


/*** Client code ***/
void *client(void *Data)
{
	input_data *tdc = (input_data*) Data;
    my_data msg;
    my_reply reply;

    msg.ClientID = 500;

    int server_coid;
    int index = 0;

	printf("   --> Trying to connect (server) process which has a PID: %d\n",   tdc->data1);
	printf("   --> on channel: %d\n\n", tdc->data2);

	// set up message passing channel
    server_coid = ConnectAttach(ND_LOCAL_NODE, tdc->data1, tdc->data2, _NTO_SIDE_CHANNEL, 0);
	if (server_coid == -1)
	{
        printf("\n    ERROR, could not connect to server!\n\n");
        pthread_exit( EXIT_FAILURE );
	}
    printf("Connection established to process with PID:%d, Ch:%d\n", tdc->data1, tdc->data2);

	int rcvid=0, msgnum=0;  	// no message received yet
	int Stay_alive=0, living=0;	// server stays running (ignores _PULSE_CODE_DISCONNECT request)

	my_reply replymsg; 			// replymsg structure for sending back to client
	replymsg.hdr.type = 0x01;
	replymsg.hdr.subtype = 0x00;

	living = 1;
	while (living)
	{
	   // Do your MsgReceive's here now with the chid
	   rcvid = MsgReceive(server_coid, &msg, sizeof(msg), NULL);

	   while (rcvid == -1)  // Error condition, exit
	   {
		   //printf("\nFailed to MsgReceive\n");
		   //break;
	   }

	   // did we receive a Pulse or message?
	   // for Pulses:
	   if (rcvid == 0)  //  Pulse received, work out what type
	   {
		   switch (msg.hdr.code)
		   {
			   case _PULSE_CODE_DISCONNECT:
					// A client disconnected all its connections by running
					// name_close() for each name_open()  or terminated
				   if( Stay_alive == 0)
				   {
					   ConnectDetach(msg.hdr.scoid);
					   printf("\nServer was told to Detach from ClientID:%d ...\n", msg.ClientID);
					   living = 0; // kill while loop
					   continue;
				   }
				   else
				   {
					   printf("\nServer received Detach pulse from ClientID:%d but rejected it ...\n", msg.ClientID);
				   }
				   break;

			   case _PULSE_CODE_UNBLOCK:
					// REPLY blocked client wants to unblock (was hit by a signal
					// or timed out).  It's up to you if you reply now or later.
				   printf("\nServer got _PULSE_CODE_UNBLOCK after %d, msgnum\n", msgnum);
				   break;

			   case _PULSE_CODE_COIDDEATH:  // from the kernel
				   printf("\nServer got _PULSE_CODE_COIDDEATH after %d, msgnum\n", msgnum);
				   break;

			   case _PULSE_CODE_THREADDEATH: // from the kernel
				   printf("\nServer got _PULSE_CODE_THREADDEATH after %d, msgnum\n", msgnum);
				   break;

			   default:
				   // Some other pulse sent by one of your processes or the kernel
				   printf("\nServer got some other pulse after %d, msgnum\n", msgnum);
				   break;

		   }
		   continue;// go back to top of while loop
	   }

	   // for messages:
	   if(rcvid > 0) // if true then A message was received
	   {
		   msgnum++;

		   // If the Global Name Service (gns) is running, name_open() sends a connect message. The server must EOK it.
		   if (msg.hdr.type == _IO_CONNECT )
		   {
			   MsgReply( rcvid, EOK, NULL, 0 );
			   printf("\n gns service is running....");
			   continue;	// go back to top of while loop
		   }

		   // Some other I/O message was received; reject it
		   if (msg.hdr.type > _IO_BASE && msg.hdr.type <= _IO_MAX )
		   {
			   MsgError( rcvid, ENOSYS );
			   printf("\n Server received and IO message and rejected it....");
			   continue;	// go back to top of while loop
		   }

		   // A message (presumably ours) received


		   	if(strcmp(msg.data,"BF")==0)
			{
				printf("Got message to close boom gate\n");
			}
		   	else
		   	{
		   		printf("Got the message ---> %s\n", msg.data);
		   	}




		   // put your message handling code here and assemble a reply message
		   sprintf(replymsg.buf, "Message %d received", msgnum);
		  // printf("Server received data packet with value of '%d' from client (ID:%d), ", msg.data, msg.ClientID);
			   fflush(stdout);

		sleep(1); // Delay the reply by a second (just for demonstration purposes)
		   MsgReply(rcvid, EOK, &replymsg, sizeof(replymsg));
	   }
	   else
	   {
		   printf("\nERROR: Server received something, but could not handle it correctly\n");
	   }

	}

	//printf("\nServer received Destroy command\n");
	// destroyed channel before exiting
	//ChannelDestroy(chid);


	pthread_exit( EXIT_FAILURE );

}




int main(int argc, char *argv[])
{
	FILE *fptr;
	printf("Client running\n");
	int serverPID;
	int serverCHID;

	   if ((fptr = fopen("/tmp/file/file.info","r")) == NULL)
	   {
		   printf("Error! opening file");

		   // Program exits if the file pointer returns NULL.
		   exit(1);
	   }

	   fscanf(fptr,"%d\n", &serverPID);
	   fscanf(fptr,"%d", &serverCHID);
	   fclose(fptr);
	   input_data message_thread_data;
	   message_thread_data.data1= serverPID;
	   message_thread_data.data2 = serverCHID;

	   pthread_t  th1;
	   void *retval;

		// create the producer and consumer threads
		pthread_create(&th1,NULL, client, &message_thread_data);

		pthread_join (th1, &retval);
		printf("Main Terminating...");
		return EXIT_SUCCESS;
}
