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
int lvl_checkpoint[1000];
//int is_referenced = 0;

# define DT_REG 8
# define DT_DIR 4
# define DT_LNK 10
# define DT_UNKNOWN 0
# define DT_FIFO 1
# define DT_CHR 2
# define DT_BLK 6
# define DT_LNK 10
# define DT_SOCK 12
# define DT_WHT 14
# define IGNORE_ -500



// https://sites.uclouvain.be/SystInfo/usr/include/dirent.h.html

int getNumOfFiles(const char *pathname) {
    int file_count = 0;
    DIR * dirp;
    struct dirent * entry;
    dirp = opendir(pathname); /* There should be error handling after this */
    while ((entry = readdir(dirp)) != NULL) {
        /* If the entry is a regular file or dir */
        //if (entry->d_type == DT_REG || entry->d_type == DT_DIR) {
            if (((entry->d_name)[0] != '.') && (entry->d_type != DT_LNK)) {
                file_count++;
                //if (a==0) {printf("%s\n", entry->d_name);}
                //if (entry->d_type != DT_DIR || entry->d_type != DT_REG) printf("\nFile: %s\n",entry->d_name);
            }
        }
    closedir(dirp);
    return file_count;
}


static int              /* Callback function called by ftw() */
dirTree(const char *pathname, const struct stat *sbuf, int type, struct FTW *ftwb) {
    if (strcmp("." , &pathname[ftwb->base]) == 0) {
        //printf("%d", getNumOfFiles(pathname));
        lvl_checkpoint[0] = getNumOfFiles(pathname);
        printf("\033[1m\033[34m.\033[0m\n"); // print dot in bold blue
        return 0;
    }

    // if (S_ISLNK(sbuf->st_mode) {
    //     printf("\n%s", sbuf->);
    //     return 0;
    // }
    // if (S_ISLNK(sbuf->st_mode) && is_referenced==0) { // first print
    //     is_referenced = 1;
    //     printf("link");
    //     //printf("|%d\n", ftwb->level);
    //     //printf("%d|", lvl_checkpoint[ftwb->level-1]);
    //     return 0;
    // } else if (S_ISLNK(sbuf->st_mode)) {
    //     return 0;
    // } else {
    //     is_referenced = 0;
    // }
    

    if (S_ISDIR(sbuf->st_mode)) {
        lvl_checkpoint[ftwb->level] = getNumOfFiles(pathname);
    }
    
    if ('.' == pathname[ftwb->base] && S_ISDIR(sbuf->st_mode)){
        lvl_checkpoint[ftwb->level] = IGNORE_;
        return 0;
    }
    else if ('.' == pathname[ftwb->base]){
        return 0;
    }
    if (lvl_checkpoint[ftwb->level-1]>= 0) {
        lvl_checkpoint[ftwb->level-1]--;
    }
    else if (lvl_checkpoint[ftwb->level - 1] <= IGNORE_){
        lvl_checkpoint[ftwb->level] = IGNORE_;
        return 0;
    }

    for (int i = 0; i < ftwb->level-1; i++) {
        if (lvl_checkpoint[i]>0) printf("│   ");
        else if (lvl_checkpoint[i]==0) printf("    ");
        else printf("   ");
    }
    if (lvl_checkpoint[ftwb->level-1] > 0) printf("├──");
    else if (lvl_checkpoint[ftwb->level-1] == 0) printf("└──");
    else printf("│  ");

    //if (lvl_checkpoint[ftwb->level-1] >= 0) lvl_checkpoint[ftwb->level-1]--;
    //printf("%d", lvl_checkpoint[ftwb->level-1]);
    // printf("%d", ftwb->level);

    printf(" [");
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