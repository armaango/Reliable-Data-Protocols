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

/* called from layer 5, passed the data to be sent to other side */
bool reTransmitFlag=false;
bool isAReady=true;
int seqNumforA=0;
int seqNumforB=0;
struct msg msgToStore;
float timeOut=12.0;
struct pkt bufferedAck;
struct pkt ackPacket;


int flipSeqNumber(int seqNumber)
{
    if(seqNumber==0)
    {
        seqNumber=1;
    }
    else
    {
        seqNumber=0;
    }
    return seqNumber;
}
int calcCheckSum(struct pkt packet)
{
    int checkSum=0;
    for(int i=0;i<sizeof(packet.payload);i++)
    {
        checkSum=checkSum+packet.payload[i];
    }
    checkSum=checkSum+packet.acknum;
    checkSum=checkSum+packet.seqnum;
    printf("checksum calculated is = %d\n",checkSum);
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
    seqNumforA=0;
    isAReady = true;
    reTransmitFlag=false;
}


void A_output(struct msg message)
{
    if(isAReady==true)
    {
        printf("A is ready to send\n");
        pkt packet;
        packet.seqnum=seqNumforA;
        strncpy(packet.payload, message.data,sizeof(message.data));
        packet.acknum=0;
       	printf("payload data in packet is %s\n",packet.payload);
        printf("seq num sent is %d\n",seqNumforA);
        packet.checksum=calcCheckSum(packet);
        isAReady=false;
        msgToStore=message;
        tolayer3(0,packet);
        starttimer(0,timeOut);
    }
    if(reTransmitFlag==true)
    {
        reTransmitFlag=false;
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
    if(packet.acknum==seqNumforA)
    {
        printf("Ack received successfully\n");
        isAReady=true;
        seqNumforA=flipSeqNumber(seqNumforA);
        printf("seq num flipped for a\n");
        stoptimer(0);
    }
    else
    {
        printf("Invalid Ack received\n");
    }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    printf("Timer interrrupted\n");
    
    isAReady=true;
    reTransmitFlag=true;
    A_output(msgToStore);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    if(isCorruptPacket(packet)==true)
    {
        printf("Corrupt packet recieved \n");
        if(strcmp(bufferedAck.payload,"")!=0)
        {
        tolayer3(1,bufferedAck);
        }
        return;
    }
    
    if(packet.seqnum==seqNumforB)
    {
        tolayer5(1,packet.payload);
        ackPacket.seqnum=seqNumforB;
        strcpy(ackPacket.payload,"ack data");
        ackPacket.acknum=seqNumforB;
        
        
        ackPacket.checksum=calcCheckSum(ackPacket);
        bufferedAck=ackPacket;
        tolayer3(1,ackPacket);
        seqNumforB=flipSeqNumber(seqNumforB);
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
    seqNumforB=0;
}
