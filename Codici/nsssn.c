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
 * Latest Revision : 13-08-2021                                               *
 * -------------------------------------------------------------------------- */

#include <stdio.h>
#include <math.h>
#include "rngs.h"                      /* the multi-stream generator */
#include "rvgs.h"                      /* random variate generators  */
#include <unistd.h>
#include <stdbool.h>

#define START         0.0              /* initial time                   */
#define STOP      30000.0              /* terminal (close the door) time */
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

// Output Statistics
typedef struct {                     /*        accumulated sums of         */
    double service;                  /*   service times                    */
    long   served;                   /*   number of served jobs            */

} sum[SERVERS+1];


long number[SERVERS] = {0, 0, 0, 0, 0};     // number of jobs in the node
long arrivals = 0;
long departures  = 0;
double area = 0.0;
double areaAP_SW[SERVERS] = {0.0, 0.0, 0.0, 0.0, 0.0};
double lambda = 5;
double mu_AP = 0.3328; 
double mu_SW = 46.137344; 
//Output Statistics Struct
sum statistics;

// Event List Management
event_list event;

// Clock Time
t clock;

double CalculateETQ(double prob,double mu){ 
/* -------------------------------------------------------------------------- * 
 * function to calculate E(Tq) mu represents the service rate                 *
 * while prob represents the percentage of flow                               *
 * Exponential
 * -------------------------------------------------------------------------- */ 
    double ro = prob*(lambda)/(mu);
    return (ro*(1/mu))/(1-ro);
}


double GetArrival() {
/* -------------------------------------------------------------------------- * 
 * generate the next arrival time, with rate 1/2                              *
 * -------------------------------------------------------------------------- */
    static double arrival = START;

    SelectStream(0);
    arrival += Exponential(1.0/lambda);
    return (arrival);
}

double GetService_AP() {
/* -------------------------------------------------------------------------- * 
 * generate the next service time with rate 2/3                               *
 * -------------------------------------------------------------------------- */
    SelectStream(1);
    return (Exponential(1.0/mu_AP)); //Erlang(5, 0.3));
}

double GetService_Switch() {
/* -------------------------------------------------------------------------- * 
 * generate the next service time with rate 2/3                               *
 * -------------------------------------------------------------------------- */
    SelectStream(2);
    return (Exponential(1.0/mu_SW)); //Erlang(5, 0.3));
}

void ProcessArrival(int index) {
/* -------------------------------------------------------------------------- * 
 * function that processes arrivals                                           *
 * -------------------------------------------------------------------------- */
    double service_time= 0.0;
    if(number[index-1] == 0) {  // if the queue is empty, serve it immediately
        if(index == 5) {
            service_time = GetService_Switch();
        } else {
            service_time = GetService_AP();
        }
        event[index].t = service_time + clock.next;
        event[index].x = 1;
        statistics[index].service += service_time;
        statistics[index].served++;
    }

	number[index-1]++;
}

void ProcessDeparture(int index) {
/* -------------------------------------------------------------------------- * 
 * function that processes departures                                         *
 * -------------------------------------------------------------------------- */
    double service_time= 0.0;
	if(index < 5) {
        ProcessArrival(5);  // if it comes at APs send the job to the switch
    } else {
        departures++;      // else the job leaves the system
    }

    number[index-1]--;

	if(number[index-1] > 0) {   // schedule next departure from this node
        if(index == 5) {
            service_time = GetService_Switch();
        } else {
            service_time = GetService_AP();
        }
        event[index].t = service_time + clock.next;
        event[index].x = 1;
        statistics[index].service += service_time;
        statistics[index].served++;
    } else {
        event[index].t = INFINITE;
        event[index].x = 0;
    }        
}

bool empty_queues(){
/*-------------------------------------------------------------------------- *
 * return false if there are jobs in the queues else retrun true             *
 * ------------------------------------------------------------------------- */
    for(int i=0; i<SERVERS; i++) {
		if(number[i] != 0) {
			return false;
		}
	}
	return true;
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
    
                            
    // Init
    PlantSeeds(0);
    clock.current    = START;        // set the clock
    event[0].t   = GetArrival();     // schedule the first arrival
    event[0].x = 1;
    for (int s = 1; s <= SERVERS; s++) { 
        event[s].t     = INFINITE;
        event[s].x     = 0;          // Departure process is off at the start
        statistics[s].service = 0.0;
        statistics[s].served = 0;
    }

    int e = 0;
    int current_number = 0;
    while ((event[0].t < STOP) || !empty_queues()) { 
	    e = NextEvent(event);
        clock.next = event[e].t;
        // current_number = number[0]+number[1]+number[2]+number[3]+number[4];
        // area     += (clock.next - clock.current) * current_number;
        if (e == 0) {
            // Process an Arrival
            arrivals++;
            
            double rnd = Random();              // Detect where it comes
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
            ProcessArrival(s);
            event[0].t = GetArrival(); // Scheduling Next Arrival
            if (event[0].t > STOP)
                event[0].x = 0;
        } else {
            // Process a Departure (e indicates server number)
            ProcessDeparture(e);         
        }
        current_number = number[0]+number[1]+number[2]+number[3]+number[4];
        area     += (clock.next - clock.current) * current_number;
        areaAP_SW[0] +=  (clock.next - clock.current) * number[0];
        areaAP_SW[1] +=  (clock.next - clock.current) * number[1];
        areaAP_SW[2] +=  (clock.next - clock.current) * number[2];
        areaAP_SW[3] +=  (clock.next - clock.current) * number[3];
        areaAP_SW[4] +=  (clock.next - clock.current) * number[4];
        clock.current = clock.next;
    }

    // Print of Output Statistics 
    printf("\nStatistics for %ld jobs are:\n\n", departures);
    printf("  avg interarrivals [1/lambda] = %6.2f\n", event[0].t / departures);
    printf("  avg wait ........... = %6.2f\n", area / departures);
    printf("  avg # in node ...... = %6.2f\n", area / clock.current);

    for (int s = 1; s <= SERVERS; s++)            /* adjust area to calculate */ 
       area -= statistics[s].service;             /* averages for the queue   */   
    

    printf("  avg delay .......... = %6.2f\n", area / departures);
    printf("  avg # in queue ..... = %6.2f\n", area / clock.current);
    printf("\nStatistics for each Server are:\n\n");
    printf("    server     utilization     avg service        share        avg wait       avg delay\n");
    
    int total_served = statistics[5].served +
                       statistics[1].served + statistics[2].served +
                       statistics[3].served + statistics[4].served;
    for (int s = 1; s <= SERVERS; s++)
    printf("%8d %14.3f %15.2f %15.3f %14.3f %14.3f\n", s, 
            statistics[s].service / clock.current,
            statistics[s].service / statistics[s].served,
            (double) statistics[s].served / total_served, areaAP_SW[s-1] / statistics[s].served, (areaAP_SW[s-1] -statistics[s].service) / statistics[s].served);
    printf("\n");


    printf(" Valori Teorici\n");
    double eq_ap = CalculateETQ(1/20.0,mu_AP);
    double eq_sw = CalculateETQ(1.0,mu_SW);
    printf(" E(Tq) AP %6.4f\n",eq_ap);
    printf(" E(Tq) SW %6.4f\n",eq_sw);
    printf(" E(Nq) AP+SW %6.2f\n",((eq_ap*4/20.0)+(eq_sw*4/5.0))*(lambda));
    printf(" E(Ns) AP+SW %6.2f\n",(((eq_ap+(1/mu_AP))*4/20.0)+((eq_sw+(1/mu_SW))*4/5.0))*(lambda));
    printf(" Globale    E(Tq) =%6.2f\n",(eq_ap*4/20.0)+(eq_sw*4/5.0));
    printf(" Globale    E(Ts) =%6.2f\n",((eq_ap+(1/mu_AP))*4/20.0)+((eq_sw+(1/mu_SW))*4/5.0));



    return (0);
}
