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
# define FTW_CONTINUE 2

// // https://stackoverflow.com/questions/56066067/how-to-check-if-a-symbolic-link-refers-to-a-directory
// int IsDir(const char *path) {
//     int newSize = 1 + strlen(path) + 1;
//     char * newBuffer = (char *)malloc(newSize);

//     // do the copy and concat
//     strcpy(newBuffer,path);
//     //strcat(newBuffer,buffer); // or strncat
//     newBuffer[newSize-2] = '/';
//     //std::string tmp = path;
//     //tmp += '/';
//     struct stat statbuf;
//     int res = (lstat(newBuffer, &statbuf) >= 0) && S_ISDIR(statbuf.st_mode);
//     free(newBuffer);
//     return res;
// }

int IsDir(const char *path) {
    // https://stackoverflow.com/questions/56066067/how-to-check-if-a-symbolic-link-refers-to-a-directory
    // https://stackoverflow.com/questions/1563168/example-of-realpath-function-in-c
    char buf[1000];
    realpath(path, buf);
    struct stat path_stat;
    stat(buf, &path_stat);
    return S_ISDIR(path_stat.st_mode);
}

void printSymLnkTarget(const char *path) {
    char target[1000];
    memset(target,'\0',1000);
    readlink(path, target, 1000);
    printf("%s",target);
}

// https://sites.uclouvain.be/SystInfo/usr/include/dirent.h.html

int getNumOfFiles(const char *pathname) {
    int file_count = 0;
    DIR * dirp;
    struct dirent * entry;
    dirp = opendir(pathname); /* There should be error handling after this */
    while ((entry = readdir(dirp)) != NULL) {
        /* If the entry is a regular file or dir */
        //if (entry->d_type == DT_REG || entry->d_type == DT_DIR) {
            if (((entry->d_name)[0] != '.')) { //  && (entry->d_type != DT_LNK)
                file_count++;
                //if (a==0) {printf("%s\n", entry->d_name);}
                //if (entry->d_type != DT_DIR || entry->d_type != DT_REG) printf("\nFile: %s\n",entry->d_name);
            }
            // if (((entry->d_name)[0] != '.') && (entry->d_type == DT_LNK)) {
            //    printf("%s\n",entry->d_name);
            // }
        }
    closedir(dirp);
    return file_count;
}

// https://man7.org/linux/man-pages/man3/ftw.3.html

static int              /* Callback function called by ftw() */
dirTree(const char *pathname, const struct stat *sbuf, int type, struct FTW *ftwb) {
    if (strcmp("." , &pathname[ftwb->base]) == 0) {
        //printf("%d", getNumOfFiles(pathname));
        lvl_checkpoint[0] = getNumOfFiles(pathname);
        printf("\033[1m\033[34m.\033[0m\n"); // print dot in bold blue
        return 0;
    }

    if (S_ISDIR(sbuf->st_mode)) {
        lvl_checkpoint[ftwb->level] = getNumOfFiles(pathname);
    }
    
    if ('.' == pathname[ftwb->base] && S_ISDIR(sbuf->st_mode)){ //  && S_ISDIR(sbuf->st_mode)
        lvl_checkpoint[ftwb->level] = IGNORE_;
        return FTW_CONTINUE;
    }
    else if ('.' == pathname[ftwb->base]){
        return FTW_CONTINUE;
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
    printf(" [");
    if (type != FTW_NS) {
        //printf("%7ld ", (long) sbuf->st_ino);
        //printf("%3o", sbuf->st_mode&0777);
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
    // printf(" %*s", 4 * ftwb->level, " ");         /* Indent suitably */

    // permissions colors: https://askubuntu.com/questions/17299/what-do-the-different-colors-mean-in-ls
    // https://unix.stackexchange.com/questions/94498/what-causes-this-green-background-in-ls-output
    if (S_ISDIR(sbuf->st_mode) && sbuf->st_mode & S_IWUSR 
        && sbuf->st_mode & S_IWGRP && sbuf->st_mode & S_IWOTH) { /* Print basename */
        printf(" \033[0m\033[34;42m%s\033[0m\n",  &pathname[ftwb->base]); // blue with green background
    } else if (S_ISLNK(sbuf->st_mode)) {
        printf(" \033[1m\033[36m%s\033[0m -> ",  &pathname[ftwb->base]); // bold Cyan
        printSymLnkTarget(pathname);
        printf("\n");
    } else if (S_ISDIR(sbuf->st_mode)) {
        printf(" \033[1m\033[34m%s\033[0m\n",  &pathname[ftwb->base]); // bold blue
    } else if (sbuf->st_mode & S_IXUSR) {
        printf(" \033[1m\033[32m%s\033[0m\n",  &pathname[ftwb->base]); // bold green
    } else {
        printf(" %s\n",  &pathname[ftwb->base]);     /* Print basename */
    }

    // // https://stackoverflow.com/questions/56066067/how-to-check-if-a-symbolic-link-refers-to-a-directory
    // // https://stackoverflow.com/questions/1563168/example-of-realpath-function-in-c
    // char buf[1000];
    // realpath(pathname, buf);
    // struct stat path_stat;
    // stat(buf, &path_stat);

    if (S_ISDIR(sbuf->st_mode) || (S_ISLNK(sbuf->st_mode) && IsDir(pathname))) direcory_counter++;
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

    if (nftw(argv[1], dirTree, 10, 16 | FTW_PHYS) == -1) {
        perror("nftw");
        exit(EXIT_FAILURE);
    }
    printf("\n%d directories, %d files\n", direcory_counter, files_counter);
    exit(EXIT_SUCCESS);
}