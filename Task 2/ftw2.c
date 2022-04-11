/* nftw_dir_tree.c
   Demonstrate the use of nftw(3). Walk though the directory tree specified
   on the command line (or the current working directory if no directory
   is specified on the command line), displaying an indented hierarchy
   of files in the tree. For each file, display:
      * a letter indicating the file type (using the same letters as "ls -l")
      * a string indicating the file type, as supplied by nftw()
      * the file's i-node number.
*/
#define _XOPEN_SOURCE 600 /* Get nftw() */
#include <ftw.h>
#include <sys/types.h>    /* Type definitions used by many programs */
#include <stdio.h>        /* Standard I/O functions */
#include <stdlib.h>       /* Prototypes of commonly used library functions,
                             plus EXIT_SUCCESS and EXIT_FAILURE constants */
#include <unistd.h>       /* Prototypes for many system calls */
#include <errno.h>        /* Declares errno and defines error constants */
#include <string.h>       /* Commonly used string-handling functions */
#include <pwd.h> // to get user name by user id
#include <grp.h> // to get group name by group id
#include <dirent.h> // read num of files in directory

// https://pubs.opengroup.org/onlinepubs/009695299/functions/nftw.html
// https://code.woboq.org/gcc/gcc/tree.c.html

int direcory_counter = 0;
int files_counter = 0;
int maxSpace = 1000;
int arrOfSpace[1000];
# define DT_REG 8
# define DT_DIR 4
// https://sites.uclouvain.be/SystInfo/usr/include/dirent.h.html

int getNumOfFiles(const char *pathname) {
    int file_count = 0;
    DIR * dirp;
    struct dirent * entry;
    dirp = opendir(pathname); /* There should be error handling after this */
    while ((entry = readdir(dirp)) != NULL) {
        /* If the entry is a regular file or dir */
        if (entry->d_type == DT_DIR && ((strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)))
            continue;
        if (entry->d_type == DT_REG || entry->d_type == DT_DIR) {
                file_count++;}}
    closedir(dirp);
    return file_count;
}


static int              /* Callback function called by ftw() */
dirTree(const char *pathname, const struct stat *sbuf, int type, struct FTW *ftwb) {
    if (strcmp("." , &pathname[ftwb->base]) == 0) {
        printf("\033[1m\033[34m.\033[0m\n"); // print dot in bold blue
        arrOfSpace[0] = getNumOfFiles(pathname);
        return 0;
    }
    if (!S_ISDIR(sbuf->st_mode) && !S_ISREG(sbuf->st_mode)) return 0;
    int place = ftwb->level;
    if(S_ISDIR(sbuf->st_mode)){

        int numOfFile = getNumOfFiles(pathname); // enter the number of file
        arrOfSpace[place] = numOfFile;
    }

    for (int i = 0; i < ftwb->level-1 && i < maxSpace; i++){
        if (arrOfSpace[i] > 1)printf("│   ");
        else if (arrOfSpace[i] == 1)printf("└── ");
        else printf("    ");
    }

    if(arrOfSpace[ftwb->level - 1] > 1)printf("├── ");
    else printf("└── ");
    if (S_ISDIR(sbuf->st_mode) || S_ISREG(sbuf->st_mode)) arrOfSpace[ftwb->level - 1]--;

    if(ftwb->level > maxSpace)printf(" %*s", 4 * ftwb->level, " ");         /* Indent suitably */
    printf("[");
    if (type != FTW_NS) {

        printf( (S_ISDIR(sbuf->st_mode)) ? "d" : "-");
        printf( (sbuf->st_mode & S_IRUSR) ? "r" : "-");
        printf( (sbuf->st_mode & S_IWUSR) ? "w" : "-");
        printf( (sbuf->st_mode & S_IXUSR) ? "x" : "-");
        printf( (sbuf->st_mode & S_IRGRP) ? "r" : "-");
        printf( (sbuf->st_mode & S_IWGRP) ? "w" : "-");
        printf( (sbuf->st_mode & S_IXGRP) ? "x" : "-");
        printf( (sbuf->st_mode & S_IROTH) ? "r" : "-");
        printf( (sbuf->st_mode & S_IWOTH) ? "w" : "-");
        printf( (sbuf->st_mode & S_IXOTH) ? "x" : "-");
        
        struct passwd *pws = getpwuid(sbuf->st_uid);
        printf(" %s ",pws->pw_name);
        struct group *grp = getgrgid(sbuf->st_gid);
        printf(" %7s ",grp->gr_name);
        printf(" %14ld] ",sbuf->st_size);

    } else
        printf("        ");
    if (S_ISDIR(sbuf->st_mode)) { /* Print basename */
       printf(" \033[1m\033[34m%s\033[0m\n",  &pathname[ftwb->base]); // bold blue
    } else if (sbuf->st_mode & S_IWOTH || sbuf->st_mode & S_IXOTH) {
        printf(" \033[1m\033[32m%s\033[0m\n",  &pathname[ftwb->base]); // bold green
    } else {
        printf(" %s\n",  &pathname[ftwb->base]);     /* Print basename */
    }

    if (S_ISDIR(sbuf->st_mode)) direcory_counter++;
    else files_counter++;

    return 0;                                   /* Tell nftw() to continue */
}



int
main(int argc, char *argv[])
{
    int flags = 0;
    if (argc != 2) {
        fprintf(stderr, "Usage: %s directory-path\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (nftw(argv[1], dirTree, 10, flags) == -1) {
        perror("nftw");
        exit(EXIT_FAILURE);
    }
    printf("\n%d directories, %d files\n", direcory_counter, files_counter);
    exit(EXIT_SUCCESS);
}