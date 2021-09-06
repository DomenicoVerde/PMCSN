/* -------------------------------------------------------------------------- * 
 * This program is a next-event simulation of a queueing network model of     *
 * the Wi-Fi network of Campus X.                                             *
 * Queues have infinite capacity and a FIFO scheduling discipline.            *
 * Both interarrival time distribution and service time                       *
 * distribution are assumed to be Exponential for each service                *
 * node. The service nodes are assumed to be initially idle, no arrivals are  *
 * permitted after the terminal time STOP, and the node is then purged by     *
 * processing any remaining jobs in the service node.                         *
 *                                                                            *
 * Name            : nsssn_exp.c  (Network of Single-Server Service Nodes)    *
 * Authors         : D. Verde, G. A. Tummolo, G. La Delfa                     *
 * Language        : C                                                        *
 * Latest Revision : 08-09-2021                                               *
 * -------------------------------------------------------------------------- */

#include <stdio.h>
#include <math.h>
#include "rngs.h"               /* the multi-stream generator           */
#include "rvgs.h"               /* random variate generators            */
#include <unistd.h>
#include <stdbool.h>

#define START 0.0               /* initial time                         */
#define STOP 30000.0            /* terminal (close the door) time       */
#define INFINITE (100.0 * STOP) /* must be much larger than STOP        */
#define SERVERS 5
#define LAMBDA 5                /* Traffic flow rate                    */
#define MU_AP 0.332800          /* Service rate of APs                  */
#define MU_SWITCH 46.137344     /* Service rate of Switch               */
#define RUN_TESTS_AND_CHECKS 1  /* Set this to 1 if you want to verify
                                   and validate the model               */

typedef struct
{
    double t; // next event time
    int x;    // status: 0 (off) or 1 (on)
} event_list[SERVERS + 1];

// Clock Time
typedef struct
{
    double current; // current time
    double next;    // next-event time
} t;

// Output Statistics
typedef struct
{                   // aggregated sums of:       
    double service;     //   service times                    
    long served;        //   number of served jobs          
    long arrives;       //   arrives in the node             

} sum[SERVERS + 1];

long number[SERVERS] = {0, 0, 0, 0, 0}; // number of jobs in the node
long arrivals = 0;
long departures = 0;
double area[SERVERS] = {0.0, 0.0, 0.0, 0.0, 0.0}; 

//Output Statistics Struct
sum statistics;

// Event List Management
event_list event;

// Clock Time
t clock;

double E_TQ(double lambda, double mu)
{
/* -------------------------------------------------------------------------- * 
 * function to calculate E(Tq): mu represents the service rate                *
 * while lambda represents the traffic input fl                               *
 * (This is valid only if both are Exponential)                               *
 * -------------------------------------------------------------------------- */
    double ro = lambda / mu;
    return (ro * (1 / mu)) / (1 - ro);
}

double E_TS(double lambda, double mu)
{
/* -------------------------------------------------------------------------- * 
 * function to calculate E(Ts): mu represents the service rate                *
 * while lambda represents the traffic input flow                             *
 * (This is valid only if both are Exponential)                               *
 * -------------------------------------------------------------------------- */
    return (1/(mu-lambda));
}

double GetArrival()
{
/* -------------------------------------------------------------------------- * 
 * generate the next arrival time, with rate LAMBDA                           *
 * -------------------------------------------------------------------------- */
    static double arrival = START;

    SelectStream(0);
    arrival += Exponential(1.0 / LAMBDA);
    return (arrival);
}

double GetService_AP()
{
/* -------------------------------------------------------------------------- * 
 * generate the next service time for the access points                       *
 * -------------------------------------------------------------------------- */
    SelectStream(1);
    return (Exponential(1.0 / MU_AP)); 
}

double GetService_Switch()
{
/* -------------------------------------------------------------------------- * 
 * generate the next service time for the switch                              *
 * -------------------------------------------------------------------------- */
    SelectStream(2);
    return (Exponential(1.0 / MU_SWITCH)); 
}

void ProcessArrival(int index)
{
/* -------------------------------------------------------------------------- * 
 * function that processes arrivals                                           *
 * -------------------------------------------------------------------------- */
    double service_time = 0.0;
    if (number[index - 1] == 0)
    { // if the queue is empty, serve it immediately
        if (index == 5)
        {
            service_time = GetService_Switch();
        }
        else
        {
            service_time = GetService_AP();
        }
        event[index].t = service_time + clock.current;
        event[index].x = 1;
        statistics[index].service += service_time;
        statistics[index].served++;
    }

    number[index - 1]++;
}

void ProcessDeparture(int index)
{
/* -------------------------------------------------------------------------- * 
 * function that processes departures                                         *
 * -------------------------------------------------------------------------- */
    double service_time = 0.0;
    if (index < 5)
    {
        ProcessArrival(5); // if it comes at APs send the job to the switch
    }
    else
    {
        departures++; // else the job leaves the system
    }

    number[index - 1]--;

    if (number[index - 1] > 0)
    { // schedule next departure from this node
        if (index == 5)
        {
            service_time = GetService_Switch();
        }
        else
        {
            service_time = GetService_AP();
        }
        event[index].t = service_time + clock.current;
        event[index].x = 1;
        statistics[index].service += service_time;
        statistics[index].served++;
    }
    else
    {
        event[index].t = INFINITE;
        event[index].x = 0;
    }
}

bool empty_queues()
{
/*-------------------------------------------------------------------------- *
 * return false if there are jobs in the queues else retrun true             *
 * ------------------------------------------------------------------------- */
    for (int i = 0; i < SERVERS; i++)
    {
        if (number[i] != 0)
        {
            return false;
        }
    }
    return true;
}

int NextEvent(event_list event)
{
/* -------------------------------------------------------------------------- *
 * return the index of the next event type                                    *
 * -------------------------------------------------------------------------- */
    int e;
    int i = 0;

    while (event[i].x == 0) // find the index of the first active event
        i++;

    e = i;
    while (i < SERVERS + 1)
    { // find the most imminent event
        i++;
        if ((event[i].x == 1) && (event[i].t < event[e].t))
            e = i;
    }
    return (e);
}



/*---------------------------------Tests--------------------------------------*/

bool TestProcessArrival(int index, bool first)
{
/* -------------------------------------------------------------------------- *
 * Function to verify the correct implementation of ProcessArrival().         *     
 * This simulates the system in the case there is 1 job in service            *
 * (and the next will be enqueued), and in the case there is no job in        *
 * service (and the next will be served).                                     *
 * -------------------------------------------------------------------------- */
    if (first)
    {
        clock.next = START + GetService_AP();
        event[index].t = clock.next;
        statistics[index].served = 1;
        number[index - 1] = 1;
        ProcessArrival(index);
        if (number[index - 1] == 2 && statistics[index].served == 1 && 
            event[index].t == clock.next)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        clock.next = START;
        event[index].t = clock.next;
        statistics[index].served = 0;
        number[index - 1] = 0;
        ProcessArrival(index);
        if (number[index - 1] == 1 && statistics[index].served == 1 && 
            event[index].t > clock.next)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}

bool TestProcessDeparture(int index)
{
/* -------------------------------------------------------------------------- *
 * Function to verify the correct implementation of ProcessDeparture().       * 
 * This simulates the system in the case there are 2 job in the system        *
 * and there is a departure from that service node.                           *  
 * -------------------------------------------------------------------------- */
    clock.next = START + GetService_AP();
    event[index].t = clock.next;
    number[index - 1] = 2;
    statistics[index].served = 1;
    ProcessDeparture(index);
    if (number[index - 1] == 1 && event[index].t > clock.next && 
        statistics[index].served == 2)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool TestEmptyQueue()
{
/* -------------------------------------------------------------------------- *
 * Function to verify the correct implementation of empty_queues().           *
 * This simulates the case there are some jobs in the system.                 *
 * -------------------------------------------------------------------------- */

    for (int s = 0; s < SERVERS; s++)
    {
        number[s] = 5;
    }

    if (empty_queues() == false)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/*-----------------------------End of Tests-----------------------------------*/



int main(void)
{
    // Init
    PlantSeeds(0);
    clock.current = START;     // set the clock
    event[0].t = GetArrival(); // schedule the first arrival
    event[0].x = 1;
    for (int s = 1; s <= SERVERS; s++)
    {
        event[s].t = INFINITE;
        event[s].x = 0; // Departure process is off at the start
        statistics[s].service = 0.0;
        statistics[s].served = 0;
    }

    int e = 0;
    while ((event[0].t < STOP) || !empty_queues())
    {
        e = NextEvent(event);
        clock.next = event[e].t;
        for (int j=0; j<SERVERS; j++) 
        {
            area[j] += (clock.next - clock.current) * number[j];
        }

        clock.current = clock.next;
        if (e == 0)
        {
            // Process an Arrival
            arrivals++;

            double rnd = Random(); // Detect where it comes
            int s;
            if (rnd > 0 && rnd <= 1.0 / 20)
            {
                s = 1;
            }
            else if (rnd > 1.0 / 20 && rnd <= 2.0 / 20)
            {
                s = 2;
            }
            else if (rnd > 2.0 / 20 && rnd <= 3.0 / 20)
            {
                s = 3;
            }
            else if (rnd > 3.0 / 20 && rnd <= 4.0 / 20)
            {
                s = 4;
            }
            else
            {
                s = 5;
            }
            statistics[s].arrives++; 
            ProcessArrival(s);

            event[0].t = GetArrival(); // Scheduling Next Arrival
            if (event[0].t > STOP)
            {
                event[0].x = 0;
            }
        }
        else
        {
            // Process a Departure (e indicates server number)
            ProcessDeparture(e);
        }
    }

    // Print of Output Statistics
    double tot_area = area[0] + area[1] + area[2] + area[3] + area[4];
    printf("Output Statistics (computed using %ld jobs) are:\n\n", departures);
    printf("1) Global Statistics\n");
    printf("  avg interarrival time = %6.6f\n", event[0].t/arrivals);
    printf("  avg waiting time = %6.6f\n", tot_area/departures);
    printf("  avg number of jobs in the network = %6.2f\n",
           tot_area/clock.current);

    for (int s = 1; s <= SERVERS; s++)
    {
        tot_area -= statistics[s].service;
    }
    printf("  avg delay = %6.6f\n", tot_area/departures);
    printf("  avg number of jobs in queues = %6.6f\n", tot_area/clock.current);
    printf("\n");
    printf("\n");

    printf("2) Local Statistics\n");
    printf("  server     utilization   avg service   share        ");
    printf("avg wait      avg delay\n");

    for (int s = 1; s <= SERVERS; s++)
    {
        if (s <= 4)
        {
            printf("   AP-");
        }
        else 
        {
            printf("   Sw-");
        }
        printf("%d %13.6f %13.6f %13.6f %13.6f %13.6f\n", s,
               statistics[s].service / clock.current,
               statistics[s].service / statistics[s].served,
               (double) statistics[s].arrives / arrivals,
               area[s-1] / statistics[s].served,
               (area[s-1] - statistics[s].service) / statistics[s].served);
    }
    double avg_wait = (area[0] / statistics[1].served +
                       area[1] / statistics[2].served + 
                       area[2] / statistics[3].served + 
                       area[3] / statistics[4].served ) / 4 +
                       area[4] / statistics[5].served;
    printf("\n");
    printf("  Average Waiting Time of Users: %13.6f\n", avg_wait);

    
    if (RUN_TESTS_AND_CHECKS)
    {
        printf("Now, we do some tests to verify and validate the model\n");
        printf("If you see some errors, there is something that should ");
        printf("be wrong and you need to check it.\n");
        
        printf("Test 1: function empty_queues() ");
        if (TestEmptyQueue())
            printf("OK\n");
        else 
            printf("Error!!!\n");
        
        printf("Test 2: function ProcessDeparture() ");
        if (TestProcessDeparture(1)) 
            printf("OK\n");
        else 
            printf("Error!!!\n");
        
        printf("Test 3: function ProcessArrival() ");
        if (TestProcessArrival(1,1) && TestProcessArrival(1,0)) 
            printf("OK\n");
        else 
            printf("Error!!!\n");

        printf("Consistency Check 1: Arrivals = Departures ");
        if (arrivals == departures) 
            printf("OK\n");
        else 
            printf("Error!!!\n");

        printf("\n");
        printf("\n");
        printf("3) Theorical Values (Exponential arrives/service times only)\n");
        printf("  Utilization of APs: %f\n", (1.0 / 20) * LAMBDA / MU_AP);
        printf("  Utilization of Switch: %f\n", LAMBDA/MU_SWITCH);
        
        double Etq_ap = E_TQ((1 / 20.0)*LAMBDA, MU_AP);
        double Etq_sw = E_TQ(LAMBDA, MU_SWITCH);
        printf("  E(Tq)_AP: %10.6f\n", Etq_ap);
        printf("  E(Tq)_SW: %10.6f\n", Etq_sw);
        
        double Ets_ap = E_TS((1 / 20.0)*LAMBDA, MU_AP);
        double Ets_sw = E_TS(LAMBDA, MU_SWITCH);
        printf("  E(Ts)_AP: %10.6f\n", Ets_ap);
        printf("  E(Ts)_SW: %10.6f\n", Ets_sw);

        double Ets = Ets_ap * 4/20.0 + Ets_sw * 4/5.0;
        double Etq = Etq_ap * 4/20.0 + Etq_sw * 4/5.0;
        printf("  E(Tq):    %10.6f\n", Etq);
        printf("  E(Ts):    %10.6f\n", Ets);
        printf("  E(Nq):    %10.6f\n", Etq * LAMBDA);
        printf("  E(Ns):    %10.6f\n", Ets * LAMBDA);
        
        printf("\n");
        printf("  E(Ts)_User:    %10.6f\n", Ets_ap + Ets_sw);
    }

    return (0);
}
