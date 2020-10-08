# include <ar.h>
# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>

struct meta {
    char name[16]; // room for null
    int mode;
    int size;
    time_t mtime; // a time_t is a long
};

typedef struct ar_hdr header;

int fill_ar_hdr(char *filename, struct ar_hdr *hdr);

int fill_meta(struct ar_hdr hdr, struct meta *meta);

void listFiles(int fd);

void appendFiles(int fd, char **files, int num);

int append(int fd, char *file) {

    // if the file cannot be opened
    if (open(file, O_RDONLY) == -1) {
        printf("myar: %s: no such file or directory", file);
        exit(-1);
    }

    // if
}


// Option q
// Node that there can be multiple files to append
void appendFiles(int fd, char **files, int num) {
    int i;
    for (i = 0; i < num; i++) {
        append(fd, files[i]);
    }
}

// Option t
void listFiles(int fd) {
    header *file_header = malloc(sizeof(header));

    char buff[16];
    // read in the files
    while (read(fd, file_header, sizeof(header)) == sizeof(header)) {
        int i;
        int filesize = (int) atoi(file_header->ar_size);

        memset(buff, ' ', sizeof(file_header->ar_name));
        sprintf(buff, "%.*s", sizeof(file_header->ar_name) - 1, file_header->ar_name);

        for (i = 15; i > 0; i--) {
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
    char *file;
    int fd;

    // exit if the command is too short
    if (argc <= 2) {
        printUsage();
        exit(1);
    }

    option = argv[1][0];
    ar_file = (char*) argv[2];
    file = (char*) argv[3];

    // check if the options are valid
    if (!(option == 'q' || option == 'x' || option == 'o' || option == 't' || option == 'A' || option == 'v' || option == 'd')) {
        printUsage();
        exit(1);
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
                write(ar_file, ARMAG, 8);
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


    }
}