/* ---------------------------------------------------------------------------------- */

/* nftw_dir_tree.c
   Demonstrate the use of nftw(3). Walk though the directory tree specified
   on the command line (or the current working directory if no directory
   is specified on the command line), displaying an indented hierarchy
   of files in the tree. For each file, display:
      * a letter indicating the file type (using the same letters as "ls -l")
      * a string indicating the file type, as supplied by nftw()
      * the file's i-node number.
*/

/* ---------------------------------------------------------------------------------- */

/* The feature test macro _GNU_SOURCE must be defined (before
   including any header files) in order to obtain the
   definition of FTW_ACTIONRETVAL from <ftw.h>.
   #define _GNU_SOURCE 0
   source: https://man7.org/linux/man-pages/man3/ftw.3.html
*/
#define _GNU_SOURCE 0
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

# define MAX_DEPTH 1000
# define MAX_PATH_LENGTH 1000

int direcory_counter = 0;
int files_counter = 0;
int numOfFilesInlvl[MAX_DEPTH];
int first_iteration = 1;

int IsDir(const char *path) {
    // https://stackoverflow.com/questions/56066067/how-to-check-if-a-symbolic-link-refers-to-a-directory
    // https://stackoverflow.com/questions/1563168/example-of-realpath-function-in-c
    char buf[MAX_PATH_LENGTH];
    realpath(path, buf);
    struct stat path_stat;
    stat(buf, &path_stat);
    return S_ISDIR(path_stat.st_mode);
}

void printSymLnkTarget(const char *path) {
    char target[MAX_PATH_LENGTH];
    memset(target,'\0',MAX_PATH_LENGTH);
    readlink(path, target, MAX_PATH_LENGTH);
    printf("%s",target);
}

// https://sites.uclouvain.be/SystInfo/usr/include/dirent.h.html

int getNumOfFiles(const char *pathname) {
    int file_count = 0;
    DIR * dirp;
    struct dirent * entry;
    dirp = opendir(pathname); /* There should be error handling after this */
    while ((entry = readdir(dirp)) != NULL) {
            if (((entry->d_name)[0] != '.')) {
                file_count++;
            }
        }
    closedir(dirp);
    return file_count;
}

// https://man7.org/linux/man-pages/man3/ftw.3.html

static int              /* Callback function called by ftw() */
dirTree(const char *pathname, const struct stat *sbuf, int type, struct FTW *ftwb) {
    //if (strcmp("." , &pathname[ftwb->base]) == 0) {
    if (first_iteration) {
        numOfFilesInlvl[0] = getNumOfFiles(pathname);
        printf("%s\n", pathname); // print dot in bold blue
        first_iteration = 0;
        return 0;
    }
    // if dir -> update number of files in current level
    if (S_ISDIR(sbuf->st_mode)) {
        numOfFilesInlvl[ftwb->level] = getNumOfFiles(pathname);
    }
    // if start with '.' just ignored subtree and continue to the sibling
    if ('.' == pathname[ftwb->base]){
        return FTW_SKIP_SUBTREE;
    }
    if (numOfFilesInlvl[ftwb->level-1]>= 0) {
        numOfFilesInlvl[ftwb->level-1]--;
    }
    for (int i = 0; i < ftwb->level-1; i++) {
        if (numOfFilesInlvl[i]>0) printf("│   ");
        else if (numOfFilesInlvl[i]==0) printf("    ");
        else printf("   ");
    }
    if (numOfFilesInlvl[ftwb->level-1] > 0) printf("├──");
    else if (numOfFilesInlvl[ftwb->level-1] == 0) printf("└──");
    else printf("│  ");
    printf(" [");
    if (type != FTW_NS) {
        if (S_ISLNK(sbuf->st_mode) && IsDir(pathname)) printf("l");
        else printf( (S_ISDIR(sbuf->st_mode)) ? "d" : "-");
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
    
    printf(" %s\n",  &pathname[ftwb->base]);     /* Print basename */

    if (S_ISDIR(sbuf->st_mode) || (S_ISLNK(sbuf->st_mode) && IsDir(pathname))) direcory_counter++;
    else files_counter++;

    return 0; /* Tell nftw() to continue */
}

int main(int argc, char *argv[]) {
    if (argc == 2) {
        if (nftw(argv[1], dirTree, 10, FTW_ACTIONRETVAL | FTW_PHYS) == -1) {
            perror("nftw");
            exit(EXIT_FAILURE);
        }
    } else if (argc == 1) {
        if (nftw(".", dirTree, 10, FTW_ACTIONRETVAL | FTW_PHYS) == -1) {
            perror("nftw");
            exit(EXIT_FAILURE);
        }
    } else {
        fprintf(stderr, "Usage: %s directory-path\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    printf("\n%d directories, %d files\n", direcory_counter, files_counter);
    exit(EXIT_SUCCESS);
}