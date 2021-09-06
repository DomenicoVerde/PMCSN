/* -------------------------------------------------------------------------- * 
 * This program makes a transient analysis of the queueing network.           *
 * Number of replication can be setted at row 290, in the for cycle.          *
 * Time of the simulation can be modified at 280.                             *  
 * times distributions and ratios are tested, meanwhile service time          *
 * Results of the analysis will be printed on a file.                         *
 *                                                                            *
 * Name            : transiente.c  (Transient Analysis of nsssn_bp.c)         *
 * Authors         : G. La Delfa, D. Verde, G. A. Tummolo                     *
 * Language        : C                                                        *
 * Latest Revision : 04-09-2021                                               *
 * -------------------------------------------------------------------------- */

#include <stdio.h>
#include <math.h>
#include "rngs.h"
#include "rvgs.h"
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>

#define START 0.0
#define INFINITE (30000000.0)
#define SERVERS 5
#define LAMBDA 10
#define ALPHA 0.5

typedef struct
{
    double t;
    int x;
} event_list[SERVERS + 1];

// Clock Time
typedef struct
{
    double current;
    double next;
} t;

// Output Statistics
typedef struct
{
    double service;
    long served;
    long arrives;

} sum[SERVERS + 1];

long number[SERVERS] = {0, 0, 0, 0, 0};
long arrivals = 0;
long departures = 0;
double area[SERVERS] = {0.0, 0.0, 0.0, 0.0, 0.0};
double STOP = 0;
double arrival = START;

sum statistics;

event_list event;

t clock;

double GetArrival(double arrival)
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
    SelectStream(1);
    return BoundedPareto(ALPHA, 0.3756009615, 8.756197416);
}

double GetService_Switch()
{
    /* -------------------------------------------------------------------------- * 
     * generate the next service time for the switch                              *
     * -------------------------------------------------------------------------- */
    SelectStream(2);
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

void Initialize()
{
    /* ------------------------------------------------------------------------ *
     * all system statistics are initialized                                    *
     * ------------------------------------------------------------------------ */
    for (int i = 0; i < SERVERS; i++)
    {
        number[i] = 0;
        area[i] = 0.0;
    }
    for (int s = 1; s <= SERVERS; s++)
    {
        event[s].t = INFINITE;
        event[s].x = 0;
        statistics[s].service = 0.0;
        statistics[s].served = 0;
    }
    arrivals = 0;
    departures = 0;
    event[0].t = 0; /* In modo che posso avere una nuova replica */
}

double transient(double t_arresto)
{
    Initialize();

    // GetSeed(&seed);
    // printf("seed iniziale: %ld\n", seed); //print the state of generator

    STOP = t_arresto;                    // stopping time
    clock.current = START;               // set the clock
    event[0].t = GetArrival(event[0].t); // schedule the first arrival
    event[0].x = 1;
    int e = 0;

    while (event[0].t < STOP) // system stop condition
    {
        e = NextEvent(event);
        clock.next = event[e].t;
        for (int j = 0; j < SERVERS; j++)
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
            event[0].t = GetArrival(event[0].t); // Scheduling Next Arrival
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
    double avg_wait = (area[0] / statistics[1].served +
                       area[1] / statistics[2].served +
                       area[2] / statistics[3].served +
                       area[3] / statistics[4].served) /
                          4 +
                      area[4] / statistics[5].served;

    // GetSeed(&seed); //ogni volta che eseguo getSeed, ottengo lo stato del generatore
    // printf("seed finale: %ld\n\n", seed);

    return (avg_wait);
}

int main()
{
    double t_arresto = 105; //210; //410; //820; //1640; //3280; //6560; //13110;
    long seed = 123456789;
    double response;
    FILE *file = fopen("file.txt", "w+");
    if (file == NULL)
    {
        printf("Error");
        return 0;
    }
    PlantSeeds(seed); // initialize plantSeeds out of the replication cycle
    for (int i = 0; i < 100; i++)
    {
        /* ------------------------------------------------------------------------ *
         * Replications loop                                                        *
         * ------------------------------------------------------------------------ */
        response = transient(t_arresto);
        fprintf(file, "%f\n", response);
        fflush(file);
    }
    fclose(file);
}
