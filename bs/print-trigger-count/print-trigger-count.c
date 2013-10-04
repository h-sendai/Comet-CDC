#include <sys/time.h>

#include <arpa/inet.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int usage()
{
    fprintf(stderr, "Usage: ./print-trigger-count filename\n");
    return 0;
}

struct cdc_header {
    unsigned char packet_type;
    unsigned int  packet_id;
    int length;
    int trigger_count;
};

int main(int argc, char *argv[])
{
    //struct cdc_header header;

    int header_len  = 12;
    int window_size = 30;
    int n_ch        = 64;
    int one_data    = sizeof(short);
    int data_len    = 2*one_data*window_size*n_ch;
    int fread_len   = header_len + data_len;
    FILE *fp;
    int n;
    unsigned int trigger;
    unsigned int *trigger_p;

    if (argc != 2) {
        usage();
        exit(1);
    }
    
    unsigned char *buf = malloc(fread_len);
    if (buf == NULL) {
        err(1, "malloc for buf");
    }

    fp = fopen(argv[1], "r");
    if (fp == NULL) {
        err(1, "fopen");
    }

    for ( ; ; ) {
        n = fread(buf, 1, fread_len, fp);
        if (n == 0) {
            if (feof(fp)) {
                break;
            }
            else if (ferror(fp)) {
                if (fclose(fp) != 0) {
                    err(1, "flcose");
                }
                err(1, "fread");
            }
            else {
                if (fclose(fp) != 0) {
                    err(1, "flcose");
                }
                errx(1, "unknown error");
            }
        }
        else if (n != fread_len) {
            fprintf(stderr, "short read: %d\n", n);
            break;
        }
        else {
            trigger_p = (unsigned int *)&buf[8];
            trigger = ntohl(*trigger_p);
            printf("%d\n", trigger);
        }
    }

    if (fclose(fp) != 0) {
        err(1, "flcose");
    }

    return 0;
}
