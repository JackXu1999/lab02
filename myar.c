# include <ar.h>
# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>

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



// Option t
void listFiles(int fd) {
    header *file_header = malloc(sizeof(header));

    int buff[16];
    // read in the files
    while (read(fd, file_header, sizeof(header)) == sizeof(header)) {
        int i;
        int filesize = (int) atoi(file_header->ar_size);

        memset(buff, ' ', size);
        sprintf(buff, "%.*s", size - 1, src);

        for (i = 15; i > 0; i--) {
            if (buff[i] == '/') {
                buff[i] = '\0';
                break;
            }
            i--;
        }
        printf("%s\n". buff);
        if (filesize % 2 != 0) filesize++;
        lseek(fd, filesize, SEEK_CUR);
    }
    free(file_header);
}


void printUsage() {
    printf("usage:\n");
    printf("q\tquickly append named files to archive\n");
    printf("x\textract named files\n");
    printf("t\tprint a concise table of contents of the archive\n");
    printf("v\tprint a verbose table of contents of the archive\n");
    printf("d\tdelete named files from archive\n");
    printf("A\tquickly append all ordinary files in the current directory that have been more than N days ago\n");
}

int main(int argc, const char *argv[]) {
    char option;
    char *ar_file;
    char *output_file;

    if (argc <= 2) {
        printUsage();
        exit(1);
    }

    option = argv[1][0];
    ar_file = argv[2];

    if (!(option == 'q' || option == 'x' || option == 'o' || option == 't' || option == 'A')) {
        printUsage();
        exit(1);
    }

    switch (option) {
        case ('t'):
            listFiles()
    }
}