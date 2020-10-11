# include <ar.h>
# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <utime.h>
# include <dirent.h>
# include <time.h>

struct Meta {
    char name[16]; // room for null
    mode_t mode;
    int size;
    time_t mtime; // a time_t is a long
};

typedef struct ar_hdr header;

int fill_ar_hdr(header *file_header, int fd, struct stat* information, char filename[]);

int fill_meta(header *file_header, struct Meta meta);

void listFiles(int fd);

int extract(int fd, char *file);

int append(int fd, char *file);

int fill_meta(header *file_header, struct Meta meta) {
    char *ptr;
    meta.mode = strtol(file_header->ar_mode, &ptr,8);
    meta.size = (int) atoi(file_header->ar_size);
    meta.mtime = (time_t) atoll(file_header->ar_date);
}

int fill_ar_hdr(header *file_header, int fd, struct stat* information, char filename[]) {
    // ar_name[16]
    // ar_date[12]
    // ar_uid[6]
    // ar_gid[6]
    // ar_mode[8]
    // ar_size[10]
    // ar_fmag[2]

    sprintf(file_header->ar_name, "%-16s", filename);
    sprintf(file_header->ar_date, "%-12ld", information->st_mtime);
    sprintf(file_header->ar_uid, "%-6u", information->st_uid);
    sprintf(file_header->ar_gid, "%-6u", information->st_gid);
    sprintf(file_header->ar_mode, "%-8o", information->st_mode);
    sprintf(file_header->ar_size, "%-10ld", information->st_size);
    sprintf(file_header->ar_fmag, "%-2s", ARFMAG);

    write(fd, file_header, sizeof(header));
}

// option A
int appendAll(int fd, char *file, int n) {
    DIR *dp;
    // check if the current dir can be opened
    if ((dp = opendir("./")) == NULL) {
        printf("myar: cannot open current directory\n");
        exit(-1);
    }

    double length_of_days = n * 86400;
    struct dirent *dir;
    char *filename;
    struct stat *info = malloc(sizeof(struct stat));
    time_t current;

    while ((dir = readdir(dp)) != NULL) {

        // if it is a regular file, and it is not the current ar file
        if (dir->d_type == DT_REG) {
            filename = dir->d_name;
            stat(filename , info);
            current = time(NULL);
            if (difftime(current, info->st_mtime) >= length_of_days && (strcmp(file, dir->d_name)) != 0) {
                append(fd, dir->d_name);
                printf("file: %s appended\n", dir->d_name);
            }
        }
    }
}


// Option x and o, restore timestamp
// just a single file
int extract(int fd, char *file) {
    int flag;
    struct Meta meta;
    int file_size;

    header *file_header = malloc(sizeof(header));
    // read the file
    while (read(fd, file_header, sizeof(header)) == sizeof(header)) {
        flag = 0;
        int i = 16;
        while (i >= 0) {
            if (file_header->ar_name[i] == '/') {
                file_header->ar_name[i] = '\0';
                break;
            }
            i--;
        }

        file_size = (int) atoi(file_header->ar_size);
        // if we cannot find the ar file
        if (strncmp(file, file_header->ar_name, strlen(file)) != 0) {
            if (file_size % 2 != 0) file_size ++;
            lseek(fd, file_size, SEEK_CUR);
        } else {
            flag = 1;
            break;
        }

    }

    if (flag == 0) {
        printf("myar: %s: No such file or directory\n", file);
        exit(-1);
    }

    // copying and formatting variables into meta
    // do not use copy_meta method here
    char *ptr;
    // mode is a long
    meta.mode = strtol(file_header->ar_mode, &ptr,8);
    meta.size = (int) atoi(file_header->ar_size);
    // mtime is long
    meta.mtime = (time_t) atol(file_header->ar_date);

    // create the file or truncate it if it already exits
    int new_file_fd = creat(file, meta.mode);
    struct stat *information = malloc(sizeof(struct stat));
    fstat(fd, information);
    int buf_size = information->st_blksize;
    int temp;

    struct utimbuf* timestamp_buf = (struct utimbuf*) malloc(sizeof(struct utimbuf));
    timestamp_buf->modtime = (time_t) meta.mtime;
    timestamp_buf->actime = (time_t) meta.mtime;
    char* buf[buf_size];

    // write the content
    while (file_size > buf_size) {
        if ((temp = read(fd, buf, buf_size)) > 0) {
            if (write(new_file_fd, buf, temp) != temp) {
                printf("myar: writing error occurred\n");
                exit(-1);
            }
        }

        // we meed to decrease the size
        file_size -= buf_size;
    }
    buf_size = file_size;
    read(fd, buf, buf_size);


    // fix the timestamp
    utime(file, timestamp_buf);

    // if there is an error
    lseek(fd, SARMAG, SEEK_SET);
    free(timestamp_buf);

    // free variables
    free(file_header);
    free(information);
    close(new_file_fd);
}

// option o or x, do not restore the timestamp

int simple(int fd, char *file) {
    int flag;
    struct Meta meta;
    int file_size;

    header *file_header = malloc(sizeof(header));
    // read the file
    while (read(fd, file_header, sizeof(header)) == sizeof(header)) {
        flag = 0;
        int i = 16;
        while (i >= 0) {
            if (file_header->ar_name[i] == '/') {
                file_header->ar_name[i] = '\0';
                break;
            }
            i--;
        }
        file_size = (int) atoi(file_header->ar_size);
        // if we cannot find the ar file
        if (strncmp(file, file_header->ar_name, strlen(file)) != 0) {
            if (file_size % 2 != 0) file_size ++;
            lseek(fd, file_size, SEEK_CUR);
        } else {
            flag = 1;
            break;
        }

    }

    if (flag == 0) {
        printf("myar: %s: No such file or directory\n", file);
        exit(-1);
    }

    // copying and formatting variables into meta
    // do not use copy_meta method here
    char *ptr;
    // mode is a long
    meta.mode = strtol(file_header->ar_mode, &ptr,8);
    meta.size = (int) atoi(file_header->ar_size);
    // mtime is long
    meta.mtime = (time_t) atol(file_header->ar_date);

    // create the file or truncate it if it already exits
    int new_file_fd = creat(file, meta.mode);
    struct stat *information = malloc(sizeof(struct stat));
    fstat(fd, information);
    int buf_size = information->st_blksize;
    int temp;

    // write the content
    while (file_size > 0) {
        // the buffer size has to be smaller
        if (file_size < buf_size) {
            buf_size = file_size;
        }

        char* buf[buf_size];
        if ((temp = read(fd, buf, buf_size)) > 0) {
            if (write(new_file_fd, buf, temp) != temp) {
                printf("myar: writing error occurred\n");
                exit(-1);
            }
        }

        // we meed to decrease the size
        file_size -= buf_size;
    }

    // free variables
    free(file_header);
    free(information);
    close(new_file_fd);


}


// Option q
// just a single file
int append(int fd, char *file) {

    struct stat* information = (struct stat*) malloc(sizeof(struct stat));
    // if the file cannot be opened
    if (open(file, O_RDONLY) == -1) {
        printf("myar: %s: no such file or directory\n", file);
        free(information);
        exit(-1);
    } else if (stat(file, information) == -1) {
        // if the file info cannot be read
        printf("myar: %s: cannot read the file information\n", file);
        free(information);
        exit(-1);
    }


    header *file_header = malloc(sizeof(header));
    char filename[16];
    strcpy(filename, file);
    filename[strlen(file)] = '\0';
    strcpy(file_header->ar_name, filename);

    // write the header
    fill_ar_hdr(file_header, fd, information, file);
    int f_block = information->st_blocks;
    char* f_buffer[f_block];
    int size;
    while (read(fd, f_buffer, f_block) > 0) {
        size = read(fd, f_buffer, f_block);
        write(fd, f_buffer, size);
    }

    if (lseek(fd, 0, SEEK_END) % 2 != 0) write(fd, "\n", 1);

    free(file_header);
    free(information);

    return 1;
}



// Option t
void listFiles(int fd) {
    header *file_header = malloc(sizeof(header));

    char buff[16];
    // read in the files
    while (read(fd, file_header, sizeof(header)) == sizeof(header)) {
        int i;
        int filesize = (int) atoi(file_header->ar_size);

        sprintf(buff, "%.*s", sizeof(file_header->ar_name) - 1, file_header->ar_name);

        i = 15;
        while (i > 0) {
            if (buff[i] == '/') {
                buff[i] = '\0';
                break;
            }
            i--;
        }
        printf("%s\n", buff);
        if (filesize % 2 != 0) filesize++;
        lseek(fd, filesize, SEEK_CUR);
    }
    free(file_header);
}

// handle different kinds of errors
void printUsage() {
    printf("usage:\n");
    printf("q\tquickly append specified files to the archive-file\n");
    printf("x\textract the specified files from the archive-file\n");
    printf("o\tused in combination with 'x' restore the original permission and mtime\n");
    printf("t\tlist the file names in the archive-file\n");
    printf("v\tused in combination with 't' print the verbose info as 'ar' does\n");
    printf("d\tdelete files specified from the archive-file\n");
    printf("A\tquickly append all regular files in the current directory modified more than N days ago\n");
}

int main(int argc, const char *argv[]) {
    char option;
    char *ar_file;
    int fd;

    // exit if the command is too short
    if (argc <= 2) {
        printf("myar: too few arguments\n");
        printUsage();
        exit(-1);
    }

    option = argv[1][0];
    ar_file = (char*) argv[argc - 2];


    char *single_file;
    single_file = (char *) argv[argc - 1];

    // check if the options are valid
    if (!(option == 'q' || option == 'x' || option == 'o' || option == 't' || option == 'A' || option == 'v' || option == 'd')) {
        printUsage();
        exit(-1);
    }

    if (option == 'A') {
        ar_file = (char*) argv[argc - 1];
    }

    // check if the AR file exists
    if (access(ar_file, F_OK) != -1) {

        fd = open(ar_file, O_RDWR | O_APPEND);
        char magic[9];
        magic[8] = '\0';
        read(fd, magic, 8);

        if (strcmp(magic, ARMAG) != 0) {
            printf("myar: %s: File format not recognized\n", ar_file);
            close(fd);
            exit(-1);

        }

    }

    if (access(ar_file, F_OK) == -1) {
            if (option == 'q' || option == 'A') {
                // other cases 1: file does not exist
                fd = open(ar_file, O_RDWR | O_CREAT, 0666);
                printf("myar: creating: %s\n", ar_file);
                write(fd, ARMAG, 8);
            } else {
                // other case 2:
                printf("myar: %s: No such file or directory\n", ar_file);
                exit(-1);
            }

    }

    int number;

    switch (option) {
        case ('t'):
            listFiles(fd);
            break;

        case ('q'):
            append(fd, single_file);
            break;

        case ('x'):
            if (argv[2][0] == 'o') {
                extract(fd, single_file);
                printf("updating timestamp\n");
            } else {
                simple(fd, single_file);
                printf("not updating timestamp\n");
            }
            break;

        case ('A'):
            number = argv[2][0] - '0';
            appendAll(fd, ar_file, number);
            break;

        case ('o'):
            if (argv[2][0] == 'x') {
                extract(fd, single_file);
                printf("updating timestamp\n");
            } else {
                simple(fd, single_file);
                printf("not updating timestamp\n");
            }
            break;
    }

    close(fd);
}