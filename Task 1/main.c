#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <utmp.h>
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
#define NULLUT ((struct utmp *)NULL)

int fd;
int cur_rec = 0;
static char utmpbuf[UTSIZE];

int new_wtmp_fd()
{
        int new_fd = open("/var/log/wtmp", O_RDONLY);
        if (new_fd == -1)
        {
                perror("/var/log/wtmp");
                exit(1);
        }
        return new_fd;
}

void print_time(time_t rawtime,struct timeval *time_login){
                /* log-out time */
        int mins, hours, days;
        time_t secs  = rawtime - time_login->tv_sec; /* Under strange circumstances, secs < 0 can happen */
        mins  = (secs / 60) % 60;
        hours = (secs / 3600) % 24;
        days  = secs / 86400;

        // printf("[%-8.8s] time: %s", p->ut_user, ctime(&rawtime));
        // printf("time_login: %s", ctime(&time_login->tv_sec));
        if (days) {
                printf("(%d+%02d:%02d)", days, abs(hours), abs(mins)); /* hours and mins always shown as positive (w/o minus sign!) even if secs < 0 */
        } else if (hours) {
                printf(" (%02d:%02d)", hours, abs(mins));  /* mins always shown as positive (w/o minus sign!) even if secs < 0 */
        } else if (secs >= 0) {
                printf(" (%02d:%02d)", hours, mins);
        } else {
                printf(" (-00:%02d)", abs(mins));  /* mins always shown as positive (w/o minus sign!) even if secs < 0 */
        }
}


void logoutDetails(struct utmp *ut, struct timeval *time_login)
{
        if (ut->ut_type == BOOT_TIME)
        {
                int bool = 0;
                strcpy(ut->ut_line, "system boot");
                struct utmp *p;
                struct timeval temp_tv;
                int temp_fd = new_wtmp_fd();
                lseek(temp_fd, (cur_rec + 1) * UTSIZE, SEEK_CUR);
                char temp_utmpbuf[UTSIZE];
                while (bool < 1 && (read(temp_fd, temp_utmpbuf, UTSIZE) > 0))
                {
                        p = (struct utmp *)&temp_utmpbuf;
                        if (strncmp(p->ut_user, "shutdown", 8) == 0)
                        {
                                temp_tv.tv_sec = p->ut_tv.tv_sec;
                                temp_tv.tv_usec = p->ut_tv.tv_usec;
                                time_t rawtime = temp_tv.tv_sec;
                                print_time(rawtime, time_login);
                                bool ++;
                        }
                }
                close(temp_fd);
                if (bool == 0)
                {
                        printf("\nstill running\n");
                }
        }
        else if (ut->ut_type == USER_PROCESS)
        {
                int bool = 0;
                struct utmp *p;
                struct timeval temp_tv;
                int temp_fd = new_wtmp_fd();
                lseek(temp_fd, (cur_rec + 1) * UTSIZE, SEEK_CUR);
                char temp_utmpbuf[UTSIZE];
                while (bool < 1 && (read(temp_fd, temp_utmpbuf, UTSIZE) > 0))
                {
                        p = (struct utmp *)&temp_utmpbuf;
                        if ((strncmp(p->ut_user, "shutdown", 8) == 0) || (strncmp(p->ut_line, ut->ut_line, sizeof(ut->ut_line)) == 0))
                        {
                                /* The first record found is a computer shutdown record. print "down" */
                                if ((strncmp(p->ut_user, "shutdown", 8) == 0))
                                        printf(" - down ");
                                /* The first record found is another login record (of the same user)
                                Apparently there was a crash. */
                                else if (p->ut_type == USER_PROCESS)
                                        printf(" - crash ");
                                /* The user has logged off normally */
                                else if (p->ut_type == DEAD_PROCESS)
                                        printf(" - good "); /*  */
                                temp_tv.tv_sec = p->ut_tv.tv_sec;
                                temp_tv.tv_usec = p->ut_tv.tv_usec;
                                time_t rawtime = temp_tv.tv_sec;

                                print_time(rawtime, time_login);
                                close(temp_fd);
                                bool ++;
                        }
                }
                close(temp_fd);
                if (bool == 0)
                {
                        printf("\nstill logged in\n");
                }
        }

        printf("\n");
}

void show_info(struct utmp *utbufp)
{
        struct utmp *temp = utbufp;
        struct timeval temp_tv;
        temp_tv.tv_sec = utbufp->ut_tv.tv_sec;
        temp_tv.tv_usec = utbufp->ut_tv.tv_usec;

        //     if (_HAVE_UT_TYPE)
        //             printf("[%d] ", utbufp->ut_type);

        //     if (_HAVE_UT_PID)
        //             printf("[%05d] ", utbufp->ut_pid);
        //     if (_HAVE_UT_ID)
        //             printf("[%-4.4s] ", utbufp->ut_id);
        printf("%-8.8s %-12.12s", utbufp->ut_user, utbufp->ut_line);
        if (_HAVE_UT_HOST)
                printf(" %-16.16s", utbufp->ut_host);
        printf(" %-15.15s", ctime(&temp_tv.tv_sec));
        //     if (_HAVE_UT_TV)
        //             printf(" [%ld]", (long int) temp_tv.tv_usec); // "\n"
        /* The arguments.  */
        logoutDetails(temp, &temp_tv);
}

int main()
{
        fd = new_wtmp_fd();
        while ((read(fd, utmpbuf, UTSIZE) > 0))
        {
                show_info((struct utmp *)&utmpbuf);
                cur_rec++;
        }
        close(fd);
        printf("FINISH!\n");
        return 0;
}