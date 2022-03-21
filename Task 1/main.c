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
        struct utmp *p;
        struct timeval temp_tv;
        int temp_fd = new_wtmp_fd();
        lseek (temp_fd, (cur_rec + 1) * UTSIZE, SEEK_CUR);
        char temp_utmpbuf[UTSIZE];
        while (bool < 1 && (read(temp_fd, temp_utmpbuf, UTSIZE) > 0)) {
            p = (struct utmp * ) &temp_utmpbuf;
            if(strncmp(p->ut_user,"shutdown",8)==0) {
                temp_tv.tv_sec = p->ut_tv.tv_sec;
                temp_tv.tv_usec = p->ut_tv.tv_usec;
                time_t rawtime = temp_tv.tv_sec;
                printf ( "\n[%-8.8s] time: %s", p->ut_user, ctime (&rawtime) );
                bool++;
            }
        }
        close(temp_fd);
        if (bool==0) {
            printf ("\nstill running\n");
        }
    } else if (ut->ut_type == USER_PROCESS) {
        int bool = 0;
        struct utmp *p;
        struct timeval temp_tv;
        int temp_fd = new_wtmp_fd();
        lseek (temp_fd, (cur_rec + 1) * UTSIZE, SEEK_CUR);
        char temp_utmpbuf[UTSIZE];
        while (bool < 1 && (read(temp_fd, temp_utmpbuf, UTSIZE) > 0)) {
            p = (struct utmp * ) &temp_utmpbuf;
            if (strncmp(p->ut_line, ut->ut_line, sizeof(ut->ut_line)) == 0) {
                temp_tv.tv_sec = p->ut_tv.tv_sec;
                temp_tv.tv_usec = p->ut_tv.tv_usec;
                time_t rawtime = temp_tv.tv_sec;
                printf ( "\n[%-8.8s] time: %s", p->ut_user, ctime (&rawtime) );
                close(temp_fd);
                bool++;
            }
        }
        close(temp_fd);
        if (bool==0) {
            printf ("\nstill logged in\n");
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
    while ((read(fd, utmpbuf, UTSIZE) > 0)) {
        show_info((struct utmp * ) &utmpbuf);
        cur_rec++;
    }
    close(fd);
    printf("FINISH!\n");
    return 0;
}