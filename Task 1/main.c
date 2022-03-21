#include<stdio.h>
#include<fcntl.h>
#include<sys/types.h>
#include<utmp.h>
#include <unistd.h>
#include <time.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

//#include "utmplib.c"

#define SHOWHOST


#define NRECS 16
#define NULLUT ((struct utmp * )NULL)
#define UTSIZE (sizeof(struct utmp))

static char utmpbuf[NRECS *UTSIZE];     /*storage*/
static int num_recs;        /* num stored */
static int cur_rec;         /* next to go*/
static int fd_utmp;         /* read from */
struct utmp *first;

utmp_open( char * filename ){
    fd_utmp = open( filename, O_RDONLY );       /*open it */
    cur_rec = num_recs = 0;     /* no records yet */
    return fd_utmp;     /*report*/
}

struct utmp *utmp_next(){
    struct utmp *recp;
    if (fd_utmp == -1 )
        return NULLUT;
    if (cur_rec == num_recs && utmp_reload() == 0)
        return NULLUT;

    /* get adress of next record */

    recp = (struct utmp * ) &utmpbuf[cur_rec * UTSIZE];
    cur_rec++;
    return recp;
}

int utmp_reload(){
    /* 
     * read next bunch of records into buffer
     */
    int amt_read;
    amt_read = read (fd_utmp, utmpbuf, NRECS * UTSIZE);
    num_recs = amt_read/UTSIZE;
    cur_rec = 0;
    return num_recs;
}

utmp_close(){
    if ((fd_utmp )!= -1)
        close( fd_utmp);
}

void show_time(long);
void show_info(struct utmp*);

int main() {
    struct utmp *utbufp,          // holds pointer to next record
                *utmp_next();     // returns pointer to next

    if (utmp_open("/var/log/wtmp") == -1) {
        perror("/var/log/wtmp");
        exit(1);
    }
    first = utbufp;
    printf("utbufp size = %d\n", (int)sizeof(struct utmp));
    while ((utbufp = utmp_next()) != ((struct utmp*)NULL)) show_info(utbufp);
    utmp_close();
    return 0;
}





struct utmp *utmp_get(int record){
    struct utmp *recp;
    if (fd_utmp == -1 )
        return NULLUT;
    if (record == num_recs && utmp_reload() == 0)
        return NULLUT;

    /* get adress of next record */

    recp = (struct utmp * ) &utmpbuf[record * UTSIZE];
    return recp;
}



void logoutDetails(struct utmp *ut) {
    int c = 0;
    int whydown = 0;
    time_t lastboot = 0;
    int quit = 0;
    if(ut->ut_type == USER_PROCESS) {
        printf("[%-8.8s]",ut->ut_user);
        /*
         *  This was a login - show the first matching
         *  logout record and delete all records with
         *  the same ut_line.
         */
        struct utmp *p = utmp_get(0);
        struct timeval temp_tv;
        for (int i = 0; i < num_recs; i++) {
            p = utmp_get(i);
            if ((strncmp(p->ut_user,"shutdown",8)==0) && (strncmp(p->ut_line, ut->ut_line, UT_LINESIZE) == 0)) {
                /* Show it */
                if (c == 0) {
                    //quit = list(&ut, p->ut_time, R_NORMAL);
                    
                    temp_tv.tv_sec = p->ut_tv.tv_sec;
                    temp_tv.tv_usec = p->ut_tv.tv_usec;
                    time_t rawtime = temp_tv.tv_sec;
                    printf ( "[%-8.8s] time: %s", p->ut_user, ctime (&rawtime) );
                    strcpy(p->ut_user, "used");
                    //(printf)(" [%ld]",(4 + ctime (&temp_tv.tv_sec),(long int) temp_tv.tv_usec));
                    c = 1;
                }
                // if (p->next) p->next->prev = p->prev;
                // if (p->prev)
                //     p->prev->next = p->next;
                // else
                //     //utmplist = p->next;
                // free(p);
            }
        }
        
        //time_t rawtime = temp_tv.tv_sec;
        //printf ( "[%-8.8s] Current date and time are: %s", p->ut_user, ctime (&rawtime) );
        /*
         *  Not found? Then crashed, down, still
         *  logged in, or missing logout record.
         */
        // if (c == 0) {
        //     if (lastboot == 0) {
        //         c = R_NOW;
        //         /* Is process still alive? */
        //         if (p->ut_pid > 0 &&
        //             kill(p->ut_pid, 0) != 0 &&
        //             errno == ESRCH)
        //             c = R_PHANTOM;
        //     } else
        //         c = whydown;
        //     //quit = list(&ut, lastboot, c);
        // }
    }
    if (c==0) {
            printf ("crashed\n");
    }
}







void show_info(struct utmp *utbufp) {
    struct utmp *temp = utbufp;
    
    //printf("%.*s\n" , (int)(sizeof utbufp->ut_id), utbufp->ut_id);
/*
 *  displays the contents of the utmp struct
 *  in human readable form
 *  displays nothing if record has no user name
 */
    // if (utbufp->ut_type != USER_PROCESS)
    //     return;
    
    // printf("% -10.10s", utbufp->ut_name); /* the log name*/
    // printf(" ");    
    ///////////////////////////////
    struct timeval temp_tv;
  temp_tv.tv_sec = utbufp->ut_tv.tv_sec;
  temp_tv.tv_usec = utbufp->ut_tv.tv_usec;
    (printf) (
            /* The format string.  */
#if _HAVE_UT_TYPE
            "[%d] "
#endif
#if _HAVE_UT_PID
            "[%05d] "
#endif
#if _HAVE_UT_ID
            "[%-4.4s] "
#endif
            "[%-8.8s] [%-12.12s]"
#if _HAVE_UT_HOST
            " [%-16.16s]"
#endif
            " [%-15.15s]"
#if _HAVE_UT_TV
            " [%ld]"
#endif
            "" // "\n"
            /* The arguments.  */
#if _HAVE_UT_TYPE
            , utbufp->ut_type
#endif
#if _HAVE_UT_PID
            , utbufp->ut_pid
#endif
#if _HAVE_UT_ID
            , utbufp->ut_id
#endif
            , utbufp->ut_user, utbufp->ut_line
#if _HAVE_UT_HOST
            , utbufp->ut_host
#endif
#if _HAVE_UT_TV
            , 4 + ctime (&temp_tv.tv_sec)
            , (long int) temp_tv.tv_usec
#else
            , 4 + ctime (&utbufp->ut_time)
#endif
           );
    ///////////////////////////////
    // printf("% -8.8s", utbufp->ut_line); /* the tty */
    // printf(" ");
    // show_time(utbufp->ut_time);
// #ifdef SHOWHOST
//     if (utbufp->ut_host[0] != '\0')
//         printf("( %s)", utbufp->ut_host);
// #endif
logoutDetails(temp);
}

// void show_time(long timeval) {
// /*
//  * displays time in a format fit for human consumption
//  * uses ctime to build a string then picks parts out of it
//  */
//     char *cp;   /* to hold address of time*/
//     cp = ctime(&timeval); /* convert time to string */
//     printf("% 12.12s", cp+4);
// } 














// /* utmpdump - dump utmp-like files.
//    Copyright (C) 1997-2019 Free Software Foundation, Inc.
//    This file is part of the GNU C Library.
//    Contributed by Mark Kettenis <kettenis@phys.uva.nl>, 1997.
//    The GNU C Library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Lesser General Public
//    License as published by the Free Software Foundation; either
//    version 2.1 of the License, or (at your option) any later version.
//    The GNU C Library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Lesser General Public License for more details.
//    You should have received a copy of the GNU Lesser General Public
//    License along with the GNU C Library; if not, see
//    <http://www.gnu.org/licenses/>.  */
// #include <stdio.h>
// #include <stdlib.h>
// #include <time.h>
// #include <unistd.h>
// #include <utmp.h>
// static void
// print_entry (struct utmp *up)
// {
//   /* Mixed 32-/64-bit systems may have timeval structs of different sixe
//      but need struct utmp to be the same size.  So in 64-bit up->ut_tv may
//      not be a timeval but a struct of __int32_t's.  This would cause a compile
//      time warning and a formating error when 32-bit int is passed where
//      a 64-bit long is expected. So copy up->up_tv to a temporary timeval.
//      This is 32-/64-bit agnostic and expands the timeval fields to the
//      expected size as needed. */
//   struct timeval temp_tv;
//   temp_tv.tv_sec = up->ut_tv.tv_sec;
//   temp_tv.tv_usec = up->ut_tv.tv_usec;
//   (printf) (
//             /* The format string.  */
// #if _HAVE_UT_TYPE
//             "[%d] "
// #endif
// #if _HAVE_UT_PID
//             "[%05d] "
// #endif
// #if _HAVE_UT_ID
//             "[%-4.4s] "
// #endif
//             "[%-8.8s] [%-12.12s]"
// #if _HAVE_UT_HOST
//             " [%-16.16s]"
// #endif
//             " [%-15.15s]"
// #if _HAVE_UT_TV
//             " [%ld]"
// #endif
//             "\n"
//             /* The arguments.  */
// #if _HAVE_UT_TYPE
//             , up->ut_type
// #endif
// #if _HAVE_UT_PID
//             , up->ut_pid
// #endif
// #if _HAVE_UT_ID
//             , up->ut_id
// #endif
//             , up->ut_user, up->ut_line
// #if _HAVE_UT_HOST
//             , up->ut_host
// #endif
// #if _HAVE_UT_TV
//             , 4 + ctime (&temp_tv.tv_sec)
//             , (long int) temp_tv.tv_usec
// #else
//             , 4 + ctime (&up->ut_time)
// #endif
//            );
// }
// int
// main (int argc, char *argv[])
// {
//   struct utmp *up;
//   if (argc > 1)
//     utmpname (argv[1]);
//   setutent ();
//   while ((up = getutent ()))
//     print_entry (up);
//   endutent ();
//   return EXIT_SUCCESS;
// }