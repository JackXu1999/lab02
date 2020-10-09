# include <ar.h>
# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <utime.h>

struct Meta {
    char name[16]; // room for null
    int mode;
    int size;
    time_t mtime; // a time_t is a long
};

typedef struct ar_hdr header;

int fill_ar_hdr(header *file_header, int fd, struct stat* information, char filename[]);

int fill_meta(struct ar_hdr hdr, struct meta *meta);

void listFiles(int fd);

int extract(int fd, char *file);

int append(int fd, char *file);

int fill_ar_hdr(header *file_header, int fd, struct stat* information, char filename[]) {
    // ar_name[16]
    // ar_date[12]
    // ar_uid[6]
    // ar_gid[6]
    // ar_mode[8]
    // ar_size[10]
    // ar_fmag[2]

    // copy and print

    sprintf(file_header->ar_name, "%-16s", filename);
    sprintf(file_header->ar_date, "%-12ld", information->st_mtime);
    sprintf(file_header->ar_uid, "%-6u", information->st_uid);
    sprintf(file_header->ar_gid, "%-6u", information->st_gid);
    sprintf(file_header->ar_mode, "%-8o", information->st_mode);
    sprintf(file_header->ar_size, "%-10ld", information->st_size);
    sprintf(file_header->ar_fmag, "%-2s", ARFMAG);

    write(fd, file_header, sizeof(header));
}

// Option x
// just a single file
int extract(int fd, char *file) {
    header *file_header = malloc(sizeof(header));

    struct Meta meta;
    // read the file
    while (read(fd, file_header, sizeof(struct header)) == sizeof(struct header)) {

        int i = 15;
        while (i >= 0) {
            if (file_header->ar_name[i] == '/') {
                file_header->ar_name[i] = '\0';
                break;
            }
        }

        // if we cannot find the ar file
        if (strcmp(file, file_header->ar_name) != 0) {
            printf("myar: %s: No such file or directory", file);
            exit(-1);
        }

    }

    // copying and formatting variables into meta
    meta.mode = atoi(file_header->ar_mode);
    meta.name = file_header->ar_name;
    meta.size = atoi(file_header->ar_size);
    meta.mtime = atoi(file_header->ar_date);

    int new_file_fd;
    // create the file or truncate it if it already exits
    new_file_fd = creat(file, meta.mode);
    struct stat *information = malloc(sizeof(struct stat));
    fstat(new_file_fd, information);

    // write the content
    while (meta.size > 0) {
        char* buf[meta.size];
        if (read(new_file_fd, buf, meta.size) > 0) {
            int temp = read(ew_file_fd, buf, meta.size);
            write(new_file_fd, buf, temp);
        }
    }

    // fix the timestamp
    struct utimbuf* timestamp_buf = (struct utimbuf*) malloc(sizeof(struct utimbuf));
    timestamp_buf->modtime = meta.mtime;
    timestamp_buf->actime = meta.mtime;

    // if there is an error
    if (utime(file, timestamp_buf) == -1) {
        printf("Error occurred in updating the timestamp");
        exit(-1);
    }
    lseek(new_file_fd, SARMAG, SEEK_SET);

    // free variables
    free(timestamp_buf);
    free(file_header);
    free(information);

}


// Option q
// just a single file
int append(int fd, char *file) {

    header *file_header = malloc(sizeof(header));
    char filename[16];
    strcpy(filename, file);

    // if the file cannot be opened
    if (open(file, O_RDONLY) == -1) {
        printf("myar: %s: no such file or directory", file);
        free(file_header);
        exit(-1);
    } else if (stat(file, (struct stat*) malloc(sizeof(struct stat))) == -1) {
        // if the file info cannot be read
        printf("myar: %s: cannot read the file information", file);
        free(file_header);
        exit(-1);
    }

    struct stat* information = (struct stat*) malloc(sizeof(struct stat));

    // write the header
    fill_ar_hdr(file_header, fd, information, filename);

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
        printUsage();
        exit(-1);
    }

    option = argv[1][0];
    ar_file = (char*) argv[2];


    char *single_file;
    single_file = (char *)argv[3];

    // check if the options are valid
    if (!(option == 'q' || option == 'x' || option == 'o' || option == 't' || option == 'A' || option == 'v' || option == 'd')) {
        printUsage();
        exit(-1);
    }

    // check if the AR file exists
    if (access(ar_file, F_OK) != -1) {

        char magic[9];
        magic[8] = '\0';
        fd = open(ar_file, O_RDWR | O_APPEND);
        read(fd, magic, 8);

        if (strcmp(magic, ARMAG) != 0) {
            printf("myar: %s: File format not recognized\n", ar_file);
            close(fd);
            exit(-1);

        }

    } else {
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

    switch (option) {
        case ('t'):
            listFiles(fd);
            break;

        case ('q'):
            append(fd, single_file);

        case ('x'):
            extract(fd, single_file);
    }
}