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
 * Latest Revision : 08-09-2021                                               *
 * -------------------------------------------------------------------------- */

#include <stdio.h>
#include <math.h>
#include "rngs.h" /* the multi-stream generator */
#include "rvgs.h" /* random variate generators  */
#include <unistd.h>
#include <stdbool.h>
#include "rvms.h"

#define START 0.0               /* initial time                         */
#define STOP 100000.0           /* terminal (close the door) time       */
#define INFINITE (100.0 * STOP) /* must be much larger than STOP        */
#define SERVERS 5
#define LAMBDA 5  /* Traffic flow rate                    */
#define ALPHA 1.5 /* Shape Parameter of BP Distribution   */
#define N 400000
#define K 64
#define B (int)(N / K)

typedef struct
{
    double t; // next event time
    int x;    // status: 0 (off) or 1 (on)
} event_list[SERVERS + 1];

//Struct used to save statistics of the batchs
typedef struct
{
    double area;
    double departures;
} statistics_batch[64][5];

//Struct used to calculate confidance-intervall Batchs
typedef struct
{
    double avg_wait;
} intervall_batch[64];

// Clock Time
typedef struct
{
    double current; // current time
    double next;    // next-event time
} t;

// Output Statistics
typedef struct
{                   // aggregated sums of:
    double service; //   service times
    long served;    //   number of served jobs
    long arrives;   //   arrives in the node

} sum[SERVERS + 1];

long number[SERVERS] = {0, 0, 0, 0, 0}; // number of jobs in the node
long arrivals = 0;
int streams = 1;
long departures = 0;
int current_batch = 0; // a quale batch ci troviamo

//Output Statistics Struct
sum statistics;
// Event List Management
event_list event;

// Clock Time
t clock;

//Statistics Batch
statistics_batch s_batch;

//Batch data
intervall_batch b_intervall;

double arrival = START;

double GetArrival()
{
    /* -------------------------------------------------------------------------- * 
 * generate the next arrival time, with rate LAMBDA                           *
 * -------------------------------------------------------------------------- */

    SelectStream(0);
    arrival += Exponential(1.0 / LAMBDA);
    return (arrival);
}

double GetService_AP()
{
    /* -------------------------------------------------------------------------- * 
 * generate the next service time for the access points                       *
 * -------------------------------------------------------------------------- */
    SelectStream(streams);
    return BoundedPareto(ALPHA, 0.3756009615, 8.756197416);
}

double GetService_Switch()
{
    /* -------------------------------------------------------------------------- * 
 * generate the next service time for the switch                              *
 * -------------------------------------------------------------------------- */
    SelectStream(streams + 1);
    return BoundedPareto(ALPHA, 0.002709302035, 0.0631606037);
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
        s_batch[current_batch][index - 1].departures++;
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
        s_batch[current_batch][index - 1].departures++;
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

int main(void)
{

    for (int f = 1; f <= 10; f++)
    {

        int departures_batch = 0;
        PlantSeeds(46464);

        clock.current = START;
        streams = f * 2;
        //To delete the previus statistics
        current_batch = 0;
        arrival = START;
        departures = 0;
        number[0] = 0;
        number[1] = 0;
        number[2] = 0;
        number[3] = 0;
        number[4] = 0;
        for (int current_batch_index = 0; current_batch_index < 64; current_batch_index++)
        {
            for (int z = 0; z < SERVERS; z++)
            {
                s_batch[current_batch_index][z].area = 0.0;
                s_batch[current_batch_index][z].departures = 0;
            }
        }

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
        while (departures < N)
        {
            e = NextEvent(event);
            clock.next = event[e].t;

            if (current_batch < K)
            {
                for (int z = 0; z < SERVERS; z++)
                { //Take all batch's analyses
                    if (number[z] > 0)
                    {
                        s_batch[current_batch][z].area += (clock.next - clock.current) * number[z];
                    }
                }
            }
            if (B < departures_batch && current_batch < K)
            {
                // per passare al prossimo batch
                current_batch++;
                departures_batch = 0;
            }
            clock.current = clock.next;
            if (e == 0)
            {
                // Process an Arrival
                arrivals++;

                double rnd = Random(); // Detect where it comes
                int s;
                if (rnd > 0 && rnd <= 1.0 / 20)
                    s = 1;
                else if (rnd > 1.0 / 20 && rnd <= 2.0 / 20)
                    s = 2;
                else if (rnd > 2.0 / 20 && rnd <= 3.0 / 20)
                    s = 3;
                else if (rnd > 3.0 / 20 && rnd <= 4.0 / 20)
                    s = 4;
                else
                    s = 5;

                statistics[s].arrives++;
                ProcessArrival(s);

                event[0].t = GetArrival(); // Scheduling Next Arrival
                if (event[0].t > STOP)
                    event[0].x = 0;
            }
            else
            {
                // Process a Departure (e indicates server number)
                ProcessDeparture(e);
                departures_batch++;
            }
        }

        for (int z = 0; z < K; z++)
        {
            double avg_wait = (s_batch[z][0].area / s_batch[z][0].departures +
                               s_batch[z][1].area / s_batch[z][1].departures +
                               s_batch[z][2].area / s_batch[z][2].departures +
                               s_batch[z][3].area / s_batch[z][3].departures) /
                                  4 +
                              s_batch[z][4].area / s_batch[z][4].departures;

            b_intervall[z].avg_wait = avg_wait;
        }
        for (int i = 0; i < K; i++)
        {
            printf("%f\n", b_intervall[i].avg_wait);
        }
        printf("\n\n");
    }
    return (0);
}