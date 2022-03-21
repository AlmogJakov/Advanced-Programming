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

#define UTSIZE (sizeof(struct utmp))
#define NULLUT ((struct utmp * )NULL)

int fd;
int cur_rec = 0;
static char utmpbuf[UTSIZE];


int new_wtmp_fd() {
    int new_fd = open( "/var/log/wtmp", O_RDONLY );
        if (new_fd == -1) {
            perror("/var/log/wtmp");
            exit(1);
    }
    return new_fd;
}

void logoutDetails(struct utmp *ut) {
    if(ut->ut_type == BOOT_TIME) {
        int bool = 0;
        strcpy(ut->ut_line, "system boot");
        //printf("[%-8.8s]",ut->ut_user);
        struct utmp *p;
        struct timeval temp_tv;
        int temp_fd = new_wtmp_fd();
        lseek (temp_fd, cur_rec * UTSIZE, SEEK_CUR);
        char temp_utmpbuf[UTSIZE];
        while (bool < 1 && (read(temp_fd, temp_utmpbuf, UTSIZE) > 0)) {
            p = (struct utmp * ) &temp_utmpbuf;
            if(strncmp(p->ut_user,"shutdown",8)==0) {
                temp_tv.tv_sec = p->ut_tv.tv_sec;
                temp_tv.tv_usec = p->ut_tv.tv_usec;
                time_t rawtime = temp_tv.tv_sec;
                printf ( "\n[%-8.8s] time: %s", p->ut_user, ctime (&rawtime) );
                //strcpy(p->ut_user, "used");
                //(printf)(" [%ld]",(4 + ctime (&temp_tv.tv_sec),(long int) temp_tv.tv_usec));
                bool++;
            }
        }
        if (bool==0) {
            printf ("crashed");
        }
    }
    printf ("\n");
}


void show_info(struct utmp *utbufp) {
    struct utmp *temp = utbufp;
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
logoutDetails(temp);
}

int main() {
    fd = new_wtmp_fd();
    //printf("utbufp size = %d\n", (int)sizeof(struct utmp));
    while ((read(fd, utmpbuf, UTSIZE) > 0)) {
        show_info((struct utmp * ) &utmpbuf);
        cur_rec++;
    }
    // lseek (fd, UTSIZE, SEEK_CUR);
    // memset(fd, 0, UTSIZE);

    //read (fd, currRecord, UTSIZE);
    //show_info(currRecord);
    printf("hey\n");
    return 0;
}
































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