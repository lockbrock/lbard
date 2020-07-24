#include <stdio.h>
#include <stdlib.h>
#include <regex.h>     
#include <pcre.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

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
};

setProcess(const char proc[]);

isLineRelevant(const char line[]);

print_rhizome_packets(char line[]);

print_rhizome_add_manifest(char line[]);

print_node_info(char line[]);

print_lbard_bundle(char line[]);

print_lbard_send(char line[]);

const char *timestamp_str(long tmpLong);

static char *convert_lbard_time(long initialTime, char *timeStamp);

fix_lbard_timestamp(char timeStamp[]);

lbard_timestamp_to_long(char line[]);

writeLine(char line[], FILE* logFile);
