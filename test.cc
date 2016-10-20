/*
* shmipclib: shared-memory IPC without going through the kernel.
*
* Copyright (C) 2012 Chris Fallin <cfallin@c1f.net>.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "shmlib.h"
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <sched.h>
#include <unistd.h> /* sysconf */

struct Message {
    uint64_t m[128];
};

int queue_size = 1024;

void produce(SHM *s) {
    printf("Producer starting up\n");

    uint64_t seq = 0;

    SHMQueue<Message> q(s, queue_size);
    Message m;
    while (seq < queue_size) {
        usleep(500000);
        struct timespec put_time;
        clock_gettime(CLOCK_MONOTONIC, (struct timespec *)&m);
        seq++;

        q.push(m, false);
    }
}

void consume(SHM *s) {
    printf("Consumer starting up\n");

    uint64_t seq = 0;

    SHMQueue<Message> q(s, queue_size);
    Message m;

    while (seq < queue_size) {
        while (!q.pop(&m)) /* nothing */;
        struct timespec get_time;
        clock_gettime(CLOCK_MONOTONIC, &get_time);
        struct timespec put_time;
        memcpy(&put_time, &m, sizeof(struct timespec));
        printf("seq = %d, time diff[%lds:%ldns]\n", seq, get_time.tv_sec - put_time.tv_sec, get_time.tv_nsec - put_time.tv_nsec);
        seq++;
    }
}

int main(int argc, char **argv) {
    if (argc < 2) return 1;
    const char *shared_mem_name = "producer_consumer12";
    bool producer = !strcmp(argv[1], "-p");
    bool consumer = !strcmp(argv[1], "-c");

    if (!producer && !consumer) return 1;


    if (producer) { 
        SHM *s = new SHM(shared_mem_name); 
        produce(s);
        delete s;
    }
    if (consumer) { 
        SHM *tmp = new SHM(shared_mem_name);
        tmp->unlink();
        SHM *s = new SHM(shared_mem_name);
        consume(s); 
        s->unlink();
        delete s;
    }

    return 0;
}
