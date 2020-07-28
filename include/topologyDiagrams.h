#include <stdio.h>
#include <stdlib.h>
#include <regex.h>     
#include <pcre.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>

enum lineType{
    PROCESS = 1,
    RADIO_SEND_PACKET = 2,
    LBARD_SEND_PACKET = 3,
    LBARD_RECEIVE_PACKET = 4,
    LBARD_NEW_BUNDLE = 5,
    RHIZOME_SEND_PACKET = 6,
    RHIZOME_RECEIVE_PACKET = 7,
    RHIZOME_ADD_MANIFEST = 8,
    FAKERADIO_RULES = 9,
    NODE_INFOMATION = 10,
    FAKERADIO_BUNDLE = 11,
    RHIZOME_LAYOUT = 12,
};

void setProcess(const char proc[]);

int isLineRelevant(const char line[]);

void print_rhizome_packets(char line[]);

void print_rhizome_add_manifest(char line[]);

void print_node_info(char line[]);

void print_lbard_bundle(char line[]);

void print_lbard_send(char line[]);

void print_rhizome_layout(char line[]);

void print_fakeradio_bundle(char line[]);

char *timestamp_str(long tmpLong);

static char *convert_lbard_time(long initialTime, char *timeStamp);

void fix_lbard_timestamp(char timeStamp[]);

long lbard_timestamp_to_long(const char line[]);

void writeLine(char line[], FILE* logFile);
