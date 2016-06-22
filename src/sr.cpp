#include "../include/simulator.h"
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
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


struct packetData {
    struct pkt packet;
    int sentBit;
    int canBeSentBit;
    int ackedBit;
    float timestamp;
    bool timerRunning;
};


struct packetData bufferToStoreatA[8000];
struct packetData bufferToStoreatB[8000];

struct pkt bufferedAck;
struct pkt ackPacket;
float timeOut=16.0;
int windowSize;

int nextSeqNumberatA;
int baseSeqNumberatA;
int currentPositioninBufferatA;

bool isTimerRunning=false;
int nextSeqNumberatB;
int baseSeqNumberatB;
int currentPositioninBufferatB;

int calcCheckSum(struct pkt packet)
{
    int checkSum=0;
    int size=sizeof(packet.payload);
    checkSum=packet.acknum+packet.seqnum;
    for(int i=0;i<size;i++)
    {
        checkSum=checkSum+packet.payload[i];
    }
    //printf("checksum calculated is = %d\n",checkSum);
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

void initialisepacketToStore(int currentPositioninBufferatA, int callingFlag)
{
    bufferToStoreatA[currentPositioninBufferatA].canBeSentBit=1;
    bufferToStoreatA[currentPositioninBufferatA].ackedBit=0;
    if(callingFlag==1)
    {
        bufferToStoreatA[currentPositioninBufferatA].sentBit=1;
        bufferToStoreatA[currentPositioninBufferatA].timestamp=get_sim_time();
    }
    else{
        bufferToStoreatA[currentPositioninBufferatA].sentBit=0;
        bufferToStoreatA[currentPositioninBufferatA].timestamp=-1;
    }
    bufferToStoreatA[currentPositioninBufferatA].timerRunning=false;
    
}

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)

   {
    printf("Inside A output\n");
    printf("curr position is %d and window limit is %d\n",currentPositioninBufferatA,(baseSeqNumberatA+windowSize));
    if (currentPositioninBufferatA<baseSeqNumberatA+windowSize)
    {
        pkt packet;
        packet.seqnum=currentPositioninBufferatA;
        packet.acknum=0;
        packet.checksum=0;
        strncpy(packet.payload,message.data,sizeof(message.data));
        packet.checksum=calcCheckSum(packet);
        printf("Sending packet with seq num %d at time %f\n",packet.seqnum,get_sim_time());
        tolayer3(0,packet);
        initialisepacketToStore(currentPositioninBufferatA,1);
        bufferToStoreatA[currentPositioninBufferatA].packet=packet;
        if(isTimerRunning==false)
        {
            
            if(currentPositioninBufferatA==1)
            {
                starttimer(0,timeOut);
                printf("Timer started in A output for first message\n");
                isTimerRunning=true;
                bufferToStoreatA[currentPositioninBufferatA].timerRunning=true;
            }
            else
            {
                float timeStartedMin=9999999.0;
                int minTimerLocation=-1;
                for(int i=1;i<=currentPositioninBufferatA;i++)
                {
                    if(bufferToStoreatA[i].ackedBit==0 && bufferToStoreatA[i].timestamp!=-1)
                    {
                    if(bufferToStoreatA[i].timestamp < timeStartedMin && bufferToStoreatA[i].timerRunning==false)
                    {
                        timeStartedMin=bufferToStoreatA[i].timestamp;
                        minTimerLocation=i;
                    }
                    }
                }
                if(minTimerLocation!=-1)
                {
                    bufferToStoreatA[minTimerLocation].timerRunning=true;
                    printf("minimum on the fly packet timestamp is %f\n",bufferToStoreatA[minTimerLocation].timestamp);
                    printf("time difference is %f\n",(get_sim_time()-bufferToStoreatA[minTimerLocation].timestamp));
                    float new_timeOut=timeOut-(get_sim_time()-bufferToStoreatA[minTimerLocation].timestamp);
                    starttimer(0,new_timeOut);
                    printf("timer started inside a output second loop and timeout is %f\n",new_timeOut);
                    isTimerRunning=true;
                }
            }
        }
        nextSeqNumberatA++;
        currentPositioninBufferatA++;
    }
    else
    {
        pkt packet;
        packet.checksum=0;
        strncpy(packet.payload,message.data,sizeof(packet.payload));
        packet.acknum=0;
        packet.seqnum=currentPositioninBufferatA;
        packet.checksum=calcCheckSum(packet);
        bufferToStoreatA[currentPositioninBufferatA].packet=packet;
        initialisepacketToStore(currentPositioninBufferatA, 2),
        printf("Buffering a packet\n");
        currentPositioninBufferatA++;
    }
}


/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
    
    printf("Inside A Input\n");
        if(isCorruptPacket(packet)==true)
        {
            printf("Corrupt Ack packet received\n");
            return;
        }
        printf("Packet receieved is not corrupt\n");
        
        if(packet.acknum >= baseSeqNumberatA && packet.seqnum< (baseSeqNumberatA+windowSize))
        {
            for(int i=baseSeqNumberatA;i<currentPositioninBufferatA;i++)
            {
                printf("i value is %d\n",i);
                if(bufferToStoreatA[i].packet.seqnum==packet.acknum )
                {
                    bufferToStoreatA[i].ackedBit=1;
                    printf("Packet with seqnum %d is acked\n",i);
                    if(bufferToStoreatA[i].timerRunning==true)
                    {
                        stoptimer(0);
                        printf("Timer stopped inside A input\n");
                        isTimerRunning=false;
                        bufferToStoreatA[i].timerRunning=false;
                    }
                    break;
                }
            }
            
            if(packet.seqnum==baseSeqNumberatA)
            {
                printf("changing base\n");
                int i=baseSeqNumberatA;
                bool flag=false;
                while(i<currentPositioninBufferatA)
                {
                    if(bufferToStoreatA[i].ackedBit==0)
                    {
                        baseSeqNumberatA=i;
                        flag=true;
                        break;
                    }
                    i++;
                }
            
            
                if (flag==false)
                    baseSeqNumberatA=currentPositioninBufferatA;
            }
    
    while(nextSeqNumberatA<(baseSeqNumberatA+windowSize) && nextSeqNumberatA< currentPositioninBufferatA)
    {
        printf("Sending packet from A input with seq num %d at time %f\n",bufferToStoreatA[nextSeqNumberatA].packet.seqnum,get_sim_time());
        tolayer3(0,bufferToStoreatA[nextSeqNumberatA].packet);
        bufferToStoreatA[nextSeqNumberatA].sentBit=1;
        
        bufferToStoreatA[nextSeqNumberatA].timestamp=get_sim_time();
        bufferToStoreatA[nextSeqNumberatA].timerRunning=false;
              if(isTimerRunning==false)
              {
                  
                  if(currentPositioninBufferatA==1)
                  {
                      starttimer(0,timeOut);
                      printf("timer restarted inside A input\n");
                      isTimerRunning=true;
                      bufferToStoreatA[currentPositioninBufferatA].timerRunning=true;
                  }
                  else
                  {
                      float timeStartedMin=9999999.0;
                      int minTimerLocation=-1;
                      for(int i=1;i<currentPositioninBufferatA;i++)
                      {
                          if(bufferToStoreatA[i].ackedBit==0 && bufferToStoreatA[i].timestamp!=-1)
                          {
                              if(bufferToStoreatA[i].timestamp<timeStartedMin && bufferToStoreatA[i].timerRunning==false)
                              {
                                  timeStartedMin=bufferToStoreatA[i].timestamp;
                                  minTimerLocation=i;
                              }
                          }
                      }
                      if(minTimerLocation!=-1)
                      {
                          bufferToStoreatA[minTimerLocation].timerRunning=true;
                          float new_timeOut=timeOut-(get_sim_time()-bufferToStoreatA[minTimerLocation].timestamp);
                          starttimer(0,new_timeOut);
                          printf("timer started inside a input second loop and timeout is %f\n",new_timeOut);
                          isTimerRunning=true;
                      }
                  }
              }
        nextSeqNumberatA++;
    
}
            if(isTimerRunning==false)
            {
                int j=0;
                float timeStartedMin=999999.0;
                int minTimerLocation=0;
                while(j<currentPositioninBufferatA)
                {
                    
                    if(bufferToStoreatA[j].timestamp< timeStartedMin &&bufferToStoreatA[j].timerRunning==false && bufferToStoreatA[j].ackedBit==0)
                    {
                        timeStartedMin=bufferToStoreatA[j].timestamp;
                        minTimerLocation=j;
                    }
                    j++;
                }
                if(minTimerLocation!=1)
                {
                    bufferToStoreatA[minTimerLocation].timerRunning=true;
                    
                    float new_timeOut=timeOut-(get_sim_time()-bufferToStoreatA[minTimerLocation].timestamp);
                    starttimer(0,new_timeOut);
                    isTimerRunning=true;
                }
                
            }
            
         
            
            }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  printf("Inside timer interrupt\n");
    isTimerRunning=false;
    for(int i=1;i<currentPositioninBufferatA;i++)
    {
        if(bufferToStoreatA[i].timerRunning==true)
        {
            bufferToStoreatA[i].timerRunning=false;
             printf("Resending packet from timer interrupt with seq num %d at time %f\n", bufferToStoreatA[i].packet.seqnum,get_sim_time());
            tolayer3(0,bufferToStoreatA[i].packet);
           
            bufferToStoreatA[i].timestamp=get_sim_time();
            break;
        }
        
    }
    float timeStartedMin=9999999.0;
    int minTimerLocation=-1;
    for(int i=1;i<currentPositioninBufferatA;i++)
    {
        if(bufferToStoreatA[i].ackedBit==0 && bufferToStoreatA[i].timestamp!=-1)
        {
            if(bufferToStoreatA[i].timestamp<timeStartedMin && bufferToStoreatA[i].timerRunning==false)
            {
                timeStartedMin=bufferToStoreatA[i].timestamp;
                minTimerLocation=i;
            }
        }
    }
    if(minTimerLocation!=-1)
    {
        bufferToStoreatA[minTimerLocation].timerRunning=true;
        printf("min val index is %d\n",minTimerLocation);
        printf("minimum on the fly packet timestamp is %f\n",bufferToStoreatA[minTimerLocation].timestamp);
        printf("current time is %f",get_sim_time());
        printf("time difference is %f\n",(get_sim_time()-bufferToStoreatA[minTimerLocation].timestamp));
        float new_timeOut=timeOut-(get_sim_time()-bufferToStoreatA[minTimerLocation].timestamp);
        printf("timer started inside timer Interrupt second loop and timeout is %f\n",new_timeOut);
        starttimer(0,new_timeOut);
        isTimerRunning=true;
    }
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    currentPositioninBufferatA=1;
    baseSeqNumberatA=1;
    nextSeqNumberatA=1;
    windowSize=getwinsize();
    timeOut=16.0;

}


/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
 printf("inside B input");
        if(isCorruptPacket(packet)==true)
        {
            printf("Corrupt packet received \n");
            return;
        }
        
        int dupBit=0;
        if(bufferToStoreatB[packet.seqnum].canBeSentBit==2 && bufferToStoreatB[packet.seqnum].ackedBit==2)
        {
            dupBit=1;
        }
        
        if(dupBit==1)
        {
            printf("Duplicate packet receievd\n");
            ackPacket.acknum=packet.seqnum;
            ackPacket.seqnum=packet.seqnum;
            ackPacket.checksum=0;
            ackPacket.checksum=calcCheckSum(ackPacket);
            printf("Sending ack for duplicate packet\n");
            tolayer3(1,ackPacket);
            
            return;
        }
        
        if(packet.seqnum>=baseSeqNumberatB && packet.seqnum<(baseSeqNumberatB+windowSize) )
        {
            currentPositioninBufferatB=packet.seqnum;
            
            bufferToStoreatB[currentPositioninBufferatB].packet=packet;
            bufferToStoreatB[currentPositioninBufferatB].canBeSentBit=2;
            bufferToStoreatB[currentPositioninBufferatB].ackedBit=2;
            
            ackPacket.acknum=packet.seqnum;
            ackPacket.seqnum=packet.seqnum;
            ackPacket.checksum=0;
            strcpy(ackPacket.payload,"ack data");
            ackPacket.checksum=calcCheckSum(ackPacket);
            printf("Sending ack for correct packet\n");
            tolayer3(1,ackPacket);
            
            
            for(int i=baseSeqNumberatB; i<(baseSeqNumberatB+windowSize) ; i++)
            {
                if (bufferToStoreatB[i].canBeSentBit!=2)
                {
                    break;
                }
                else
                {
                    baseSeqNumberatB++;
                    tolayer5(1,bufferToStoreatB[i].packet.payload);
                    printf("Packet delivered at B with seqNum %d\n",bufferToStoreatB[i].packet.seqnum);
                }
            }
        }
        
        

}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  baseSeqNumberatB=1;
    nextSeqNumberatB=1;
    currentPositioninBufferatB=1;
    windowSize=getwinsize();
    
    
}
