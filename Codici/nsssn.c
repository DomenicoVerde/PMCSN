/* -------------------------------------------------------------------------- * 
 * This program is a next-event simulation of a queueing network. Topology    *
 * of the network is described by the transition-matrix P, queues have        *
 * infinite capacity and a FIFO scheduling discipline. Different interarrival *
 * times distributions and ratios are tested, meanwhile service time          *
 * distribution is fixed and it is assumed to be Exponential for each service *
 * node. The service nodes are assumed to be initially idle, no arrivals are  *
 * permitted after the terminal time STOP, and the node is then purged by     *
 * processing any remaining jobs in the service node.                         *
 *                                                                            *
 * Name            : nsssn.c  (Network of Single-Server Service Nodes)        *
 * Authors         : D. Verde, G. A. Tummolo, G. La Delfa                     *
 * Language        : C                                                        *
 * Latest Revision : 29-07-2021                                               *
 * -------------------------------------------------------------------------- */

#include <stdio.h>
#include <math.h>
#include "rngs.h"                      /* the multi-stream generator */
#include "rvgs.h"                      /* random variate generators  */
#include <unistd.h>

#define START         0.0              /* initial time                   */
#define STOP      20000.0              /* terminal (close the door) time */
#define INFINITE   (100.0 * STOP)      /* must be much larger than STOP  */
#define SERVERS 5


typedef struct {
    double t;                             // next event time   
    int    x;                             // status: 0 (off) or 1 (on)
} event_list[SERVERS + 1]; 

// Clock Time
typedef struct {
        double current;                 // current time 
        double next;                    // next-event time
} t;

long number[SERVERS] = {0, 0, 0, 0, 0};     // number of jobs in the node
long arrivals = 0;
long departures  = 0;

// Event List Management
event_list event;

// Clock Time
t clock;


double GetArrival() {
/* -------------------------------------------------------------------------- * 
 * generate the next arrival time, with rate 1/2                              *
 * -------------------------------------------------------------------------- */
    static double arrival = START;

    SelectStream(0);
    arrival += Exponential(2.0);
    return (arrival);
}

double GetService() {
/* -------------------------------------------------------------------------- * 
 * generate the next service time with rate 2/3                               *
 * -------------------------------------------------------------------------- */
    SelectStream(1);
    return (Exponential(3.0)); //Erlang(5, 0.3));
}


void ProcessArrival(int index){
/* -------------------------------------------------------------------------- * 
 * function to process arrival                                    *
 * -------------------------------------------------------------------------- */

	double service_time= 0.0;
        if(number[index-1] == 0){
                service_time =GetService()+clock.current;
                event[index].t = service_time;
                event[index].x = 1;
        }
	number[index-1]++;
	printf("La dimensione della coda dell AP %d è %ld\n",index-1,number[index-1]);

}
void ProcessDeparture(int index){
/* -------------------------------------------------------------------------- * 
 * function to process departure                                    *
 * -------------------------------------------------------------------------- */

        double service_time= 0.0;
        number[index-1]--;
	if(index <5){

                ProcessArrival(5);

        }else{
                departures++;
		
       	}
	if(number[index-1] > 0){
		service_time =GetService()+clock.current;
                event[index].t = service_time;
                event[index].x = 1;
        }else{
                event[index].t = INFINITE;
                event[index].x = 0;
        }

        
}

long Min(long array[], int len) { 
/* -------------------------------------------------------------------------- * 
 * return the smallest number in the array                                    *
 * -------------------------------------------------------------------------- */
    int min = array[0];
    for (int i=1; i<len; i++) {
        if (array[i] < min) 
            min = array[i];
    }

    return min;
} 

int NextEvent(event_list event) {
/* -------------------------------------------------------------------------- *
 * return the index of the next event type                                    *
 * -------------------------------------------------------------------------- */
    int e;                                      
    int i = 0;

    while (event[i].x == 0)       // find the index of the first active event
        i++;

    e = i;
    while (i < SERVERS + 1) {                 // find the most imminent event
        i++;
        if ((event[i].x == 1) && (event[i].t < event[e].t))
            e = i;
    }
    return (e);
}


int main(void) {

    // Transition Matrix
    double P[SERVERS+1][SERVERS+1] = { 
                                {0, 1.0/20, 1.0/20, 1.0/20, 1.0/20, 4.0/5},
                                {0,      0,      0,      0,      0,     1},
                                {0,      0,      0,      0,      0,     1},
                                {0,      0,      0,      0,      0,     1},
                                {0,      0,      0,      0,      0,     1},
                                {1,      0,      0,      0,      0,     0}};
    
    
    // Output Statistics
  struct {
    double node;                    /* time integrated number in the node  */
    double queue;                   /* time integrated number in the queue */
    double service;                 /* time integrated number in service   */
  } area      = {0.0, 0.0, 0.0};
                             

    // Init
    PlantSeeds(0);
    clock.current    = START;           // set the clock
    event[0].t   = GetArrival();    // schedule the first arrival
    event[0].x = 1;
    for (int s = 1; s <= SERVERS; s++) { 
        event[s].t     = INFINITE;
        event[s].x     = 0;          // Departure process is off at the start
    }

    int e = 0;
    while ((event[0].t < STOP) || (Min(number, SERVERS)  > 0)) { 
        //printf("STOP  =%f\n",event[0].t);
	e = NextEvent(event);
        clock.next = event[e].t;
	//printf("Sono l evento %d\n",e);
	for(int z=0; z<6; z++){
		printf("Sono l'evento %d  e ho il tempo = %f\n",z,event[z].t);
	}
	printf("Clock corrente %f",clock.current);	
        if (e == 0) {
            // Process an Arrival
            arrivals++;
            double rnd = Random();                  // Detect where it comes
            int s;
            if (rnd > 0 && rnd <= 1.0/20)
                s=1;
            else if (rnd > 1.0/20 && rnd <= 2.0/20)
                s=2;
            else if (rnd > 2.0/20 && rnd <= 3.0/20)
                s=3;
            else if (rnd > 3.0/20 && rnd <= 4.0/20)
                s=4;
            else
                s=5;
            /* TODO: call ProcessArrival(s) :
                if server is busy -> add it to the queue
                else getService() -> schedule next departure from node */
	    ProcessArrival(s);
            event[0].t = GetArrival(); // Scheduling Next Arrival
	    printf("Sono un arrivo\n");
            if (event[0].t > STOP)
                event[0].x = 0;
        } else {
            // Process a Departure e=s
	    ProcessDeparture(e);
            // Detect where it goes (switch or leaves net)
            // take another job in service           
        }
        clock.current = clock.next;
    }

/**
    if (number > 0)  {                               // update integrals  
      area.node    += (t.next - t.current) * number;
      area.queue   += (t.next - t.current) * (number - 1);
      area.service += (t.next - t.current);
    }
    t.current       = t.next;                    // advance the clock 

    if (t.current == t.arrival)  {               // process an arrival 0
      number++;
      t.arrival     = GetArrival();
      if (t.arrival > STOP)  {
        t.last      = t.current;
        t.arrival   = INFINITE;
      }
      if (number == 1)
        t.completion = t.current + GetService();
    }

    else {                                        // process a completion
      index++;
      number--;
      if (number > 0)
        t.completion = t.current + GetService();
      else
        t.completion = INFINITE;
    }**/ 

  //printf("\nfor %ld jobs\n", arrivals);
  //printf("   average interarrival time = %6.2f\n", t.last / index);
  //printf("   average wait ............ = %6.2f\n", area.node / index);
  //printf("   average delay ........... = %6.2f\n", area.queue / index);
  //printf("   average service time .... = %6.2f\n", area.service / index);
  //printf("   average # in the node ... = %6.2f\n", area.node / t.current);
  //printf("   average # in the queue .. = %6.2f\n", area.queue / t.current);
  //printf("   utilization ............. = %6.2f\n", area.service / t.current);

  return (0);
}
