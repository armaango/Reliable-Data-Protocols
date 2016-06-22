#include "../include/simulator.h"
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
int h;


struct packetData {
    struct pkt packet;
    int sentBit;
    int canBeSentBit;
};

struct packetData bufferToStore[8000];

int seqNumforB=1;

struct pkt bufferedAck;
struct pkt ackPacket;
float timeOut;
int windowSize;

int nextSeqNumber;
int baseSeqNumber;
int currentPositioninBuffer;

int calcCheckSum(struct pkt packet)
{
    int checkSum=0;
    int size=sizeof(packet.payload);
    checkSum=packet.acknum+packet.seqnum;
    for(int i=0;i<size;i++)
    {
        checkSum=checkSum+packet.payload[i];
    }
    return checkSum;
}
bool isCorruptPacket(struct pkt packet)
{
    int result=calcCheckSum(packet);
    if(result==packet.checksum)
    {
        return false;
    }
    else
    {
        return true;
    }
}
void A_init()
{
    nextSeqNumber=1;
    baseSeqNumber=1;
    currentPositioninBuffer=1;
    windowSize=getwinsize();
    
    if (windowSize>=10 && windowSize<50)
    {
        timeOut=20.0;
    }
    else if (windowSize>=50 && windowSize<100)
    {
        timeOut=22.0;
    }
    else if (windowSize>=100 && windowSize<200)
    {
        timeOut=24.0;
    }
    else if (windowSize>=200 && windowSize<500)
    {
        timeOut=26.0;
    }
    else if (windowSize>=500 && windowSize<1000)
    {
        timeOut=28.0;
    }
    
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
    
    if(currentPositioninBuffer < (baseSeqNumber+windowSize))
    {
    pkt packet;
    packet.acknum=0;
    strncpy(packet.payload,message.data,sizeof(packet.payload));
    packet.seqnum=currentPositioninBuffer;
    packet.checksum=calcCheckSum(packet);
    bufferToStore[currentPositioninBuffer].packet=packet;
    bufferToStore[currentPositioninBuffer].sentBit=1;
    bufferToStore[currentPositioninBuffer].canBeSentBit=1;
    currentPositioninBuffer++;
    tolayer3(0,packet);
    if(baseSeqNumber==nextSeqNumber)
        {
            starttimer(0,timeOut);
        }
        nextSeqNumber++;
    }
    else{
        pkt packet;
  
        packet.acknum=0;
    	strncpy(packet.payload,message.data,sizeof(packet.payload));
        packet.seqnum=currentPositioninBuffer;
        packet.checksum=calcCheckSum(packet);
        bufferToStore[currentPositioninBuffer].packet=packet;
        bufferToStore[currentPositioninBuffer].sentBit=0;
        bufferToStore[currentPositioninBuffer].canBeSentBit=1;
        currentPositioninBuffer++;
        printf("buffered data\n");
    }
}
    
    
    

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
    if(isCorruptPacket(packet)==true)
    {
        printf("Corrupt ack recieved\n");
        return;
    }
    if(packet.acknum>=baseSeqNumber)
    {
    
        baseSeqNumber=packet.acknum+1;
        if (nextSeqNumber==baseSeqNumber)
        {
            stoptimer(0);
        }
        if(nextSeqNumber>baseSeqNumber)
        {
            stoptimer(0);
            starttimer(0,timeOut);
        }
        for (int i = nextSeqNumber; i < baseSeqNumber+windowSize; i++)
        {
            if(bufferToStore[i].sentBit==0 && bufferToStore[i].canBeSentBit==1)
            {
                tolayer3(0,bufferToStore[i].packet);
                bufferToStore[i].sentBit=1;
                if(i==baseSeqNumber)
                {
                    starttimer(0,timeOut);
                }
                nextSeqNumber++;
            }
        }
    }
    else
    {

            printf("Invalid ack \n");
    }
}


/* called when A's timer goes off */
void A_timerinterrupt()
{
    printf("timer interrupted\n");
    
    for (int i=baseSeqNumber; i<nextSeqNumber; i++)
    {
        
        tolayer3(0,bufferToStore[i].packet);
    }
    starttimer(0,timeOut);
    
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */


/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    if(isCorruptPacket(packet)==true)
    {
        printf("Corrupt packet recieved\n");
        tolayer3(1,bufferedAck);
        return;
    }
    
    if(packet.seqnum==seqNumforB)
    {
    	strcpy(ackPacket.payload,"ack data");
        
        ackPacket.acknum=seqNumforB;
        tolayer5(1,packet.payload);
        ackPacket.seqnum=seqNumforB;
        
        ackPacket.checksum=calcCheckSum(ackPacket);
        bufferedAck=ackPacket;
        tolayer3(1,ackPacket);
        seqNumforB++;
    }
    else
    {
        tolayer3(1,bufferedAck);
    }

}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    
    seqNumforB=1;

}
