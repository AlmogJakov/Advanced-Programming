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

# define DT_REG 8
# define DT_DIR 4
// https://sites.uclouvain.be/SystInfo/usr/include/dirent.h.html

int getNumOfFiles(char *pathname) {
    int file_count = 0;
    DIR * dirp;
    struct dirent * entry;
    dirp = opendir(pathname); /* There should be error handling after this */
    while ((entry = readdir(dirp)) != NULL) {
        /* If the entry is a regular file or dir */
        if (entry->d_type == DT_REG || entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) 
                file_count++;}}
    closedir(dirp);
    return file_count;
}


static int              /* Callback function called by ftw() */
dirTree(const char *pathname, const struct stat *sbuf, int type, struct FTW *ftwb) {
    if (strcmp("." , &pathname[ftwb->base]) == 0) {
        printf(".\n");
        return 0;
    }
    
    // if (type == FTW_NS) {                  /* Could not stat() file */
    //     printf("?");
    // } else {
    //     switch (sbuf->st_mode & S_IFMT) {  /* Print file type */
    //     case S_IFREG:  printf("-"); break;
    //     case S_IFDIR:  printf("d"); break;
    //     case S_IFCHR:  printf("c"); break;
    //     case S_IFBLK:  printf("b"); break;
    //     case S_IFLNK:  printf("l"); break;
    //     case S_IFIFO:  printf("p"); break;
    //     case S_IFSOCK: printf("s"); break;
    //     default:       printf("?"); break; /* Should never happen (on Linux) */
    //     }
    // }
    printf(" %*s", 4 * ftwb->level, " ");         /* Indent suitably */
    printf("[");
    if (type != FTW_NS) {
        //printf("%7ld ", (long) sbuf->st_ino);
        //printf("%3o", sbuf->st_mode&0777);
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
        //printf("%40s\n", ptr);     home
        printf(" %14ld] ",sbuf->st_size);
        //printf(" Base:%d ",ftwb->base);
        //printf(" Level:%d ",ftwb->level);
    } else
        printf("        ");
    // printf(" %*s", 4 * ftwb->level, " ");         /* Indent suitably */
    if (S_ISDIR(sbuf->st_mode)) { /* Print basename */
        //printf(" \x1B[32m%s\033[0m\n",  &pathname[ftwb->base]);
        // printf(" \033[0;34m%s\033[0m\n",  &pathname[ftwb->base]); // regular blue
        printf(" \033[1m\033[34m%s\033[0m\n",  &pathname[ftwb->base]); // bold blue
    } else if (sbuf->st_mode & S_IWOTH || sbuf->st_mode & S_IXOTH) {
        printf(" \033[1m\033[32m%s\033[0m\n",  &pathname[ftwb->base]); // bold green
    } else {
        printf(" %s\n",  &pathname[ftwb->base]);     /* Print basename */
    }

    if (S_ISDIR(sbuf->st_mode)) direcory_counter++;
    else files_counter++;

    return 0;                                   /* Tell nftw() to continue */
    //return FTW_CONTINUE;
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
