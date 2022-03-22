#include <stdio.h>
#include <fcntl.h>
//#include <sys/types.h>
#include <utmp.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#define UTSIZE (sizeof(struct utmp))
#define NULLUT ((struct utmp *)NULL)

int fd;
int cur_rec = 0;
static char utmpbuf[UTSIZE];

int get_wtmp_fd() {
        int new_fd = open("/var/log/wtmp", O_RDONLY);
        if (new_fd == -1) {
                perror("/var/log/wtmp");
                exit(1);
        }
        return new_fd;
}

/* print log-out time */
void print_logout_time(time_t rawtime,struct timeval *time_login){
        int mins, hours, days;
        time_t secs  = rawtime - time_login->tv_sec;
        mins  = (secs / 60) % 60;
        hours = (secs / 3600) % 24;
        days  = secs / 86400;
        if (days) { /* print with days */
                printf("(%d+%02d:%02d)", days, abs(hours), abs(mins));
        } else if (hours) { /* print without days */
                printf(" (%02d:%02d)", hours, abs(mins));
        } else if (secs >= 0) {
                printf(" (%02d:%02d)", hours, mins);
        } else {
                printf(" (-00:%02d)", abs(mins));
        }
}

void print_user_logout_status(struct utmp *p, time_t rawtime){
        /* The first record found is a computer shutdown record. print "down" */
        if ((strncmp(p->ut_user, "shutdown", 8) == 0))
                printf(" - down  ");
        /* If the first record that found is boot record OR another login record
                then apparently there was a crash */
        else if (p->ut_type == USER_PROCESS || p->ut_type == BOOT_TIME)
                printf(" - crash ");
        /* The user has logged off normally */
        else if (p->ut_type == DEAD_PROCESS)
                printf(" - %-5.5s ", 11 + ctime(&rawtime)); /*  */
}

/* print the tail of the utmp details (logout status, logout time etc.) */
void print_info_tail(struct utmp *ut, struct timeval *time_login) {
        if (ut->ut_type == BOOT_TIME) {
                int bool = 0;
                strcpy(ut->ut_line, "system boot");
                struct utmp *p;
                struct timeval temp_tv;
                int temp_fd = get_wtmp_fd();
                lseek(temp_fd, (cur_rec + 1) * UTSIZE, SEEK_CUR);
                char temp_utmpbuf[UTSIZE];
                while (bool < 1 && (read(temp_fd, temp_utmpbuf, UTSIZE) > 0)) {
                        p = (struct utmp *)&temp_utmpbuf;
                        if (strncmp(p->ut_user, "shutdown", 8) == 0) {
                                temp_tv.tv_sec = p->ut_tv.tv_sec;
                                temp_tv.tv_usec = p->ut_tv.tv_usec;
                                time_t rawtime = temp_tv.tv_sec;
                                printf(" - %-5.5s ", 11 + ctime(&rawtime)); /*  */
                                print_logout_time(rawtime, time_login);
                                bool ++;
                        }
                }
                close(temp_fd);
                if (bool == 0) {
                        printf("   still running");
                }
        } else if (ut->ut_type == USER_PROCESS) {
                int bool = 0;
                struct utmp *p;
                struct timeval temp_tv;
                int temp_fd = get_wtmp_fd();
                lseek(temp_fd, (cur_rec + 1) * UTSIZE, SEEK_CUR);
                char temp_utmpbuf[UTSIZE];
                while (bool < 1 && (read(temp_fd, temp_utmpbuf, UTSIZE) > 0)) {
                        p = (struct utmp *)&temp_utmpbuf;
                        if ((strncmp(p->ut_user, "shutdown", 8) == 0) 
                                || (strncmp(p->ut_user, "reboot", 6) == 0) 
                                || (strncmp(p->ut_line, ut->ut_line, sizeof(ut->ut_line)) == 0)) {
                                temp_tv.tv_sec = p->ut_tv.tv_sec;
                                temp_tv.tv_usec = p->ut_tv.tv_usec;
                                time_t rawtime = temp_tv.tv_sec;
                                print_user_logout_status(p, rawtime);
                                print_logout_time(rawtime, time_login);
                                close(temp_fd);
                                bool ++;
                        }
                }
                close(temp_fd);
                if (bool == 0) {
                        printf("   still logged in");
                }
        }
        printf("\n");
}

void print_info(struct utmp *utbufp) {
        struct utmp *temp = utbufp;
        struct timeval temp_tv;
        temp_tv.tv_sec = utbufp->ut_tv.tv_sec;
        temp_tv.tv_usec = utbufp->ut_tv.tv_usec;
        if ((utbufp->ut_type == USER_PROCESS || utbufp->ut_type == BOOT_TIME)) {
                if (utbufp->ut_type == BOOT_TIME) strcpy(utbufp->ut_line, "system boot");
                printf("%-8.8s %-12.12s", utbufp->ut_user, utbufp->ut_line);
                if (_HAVE_UT_HOST)
                        printf(" %-16.16s", utbufp->ut_host);
                printf(" %-16.16s", ctime(&temp_tv.tv_sec));
                print_info_tail(temp, &temp_tv);
        }
}

int main(int argc, char *argv[]) {
        fd = get_wtmp_fd();
        if (argc > 1) {
                printf("number of args: %d",argc);
                return 0;
        }
        while ((read(fd, utmpbuf, UTSIZE) > 0)) {
                print_info((struct utmp *)&utmpbuf);
                cur_rec++;
        }
        close(fd);
        //printf("%d\n", cur_rec);
        fd = get_wtmp_fd();
        if ((read(fd, utmpbuf, UTSIZE) > 0)) {
                struct utmp *p = (struct utmp *)&utmpbuf;
                struct timeval temp_tv;
                temp_tv.tv_sec = p->ut_tv.tv_sec;
                temp_tv.tv_usec = p->ut_tv.tv_usec;
                printf("\nwtmp begins ");
                printf(" %-16.24s\n", ctime(&temp_tv.tv_sec));
        }
        close(fd);
        return 0;
}