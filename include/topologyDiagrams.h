#include <stdio.h>
#include <stdlib.h>
#include <regex.h>     
#include <pcre.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>


// ============= EVENTS (for DOT diagram generation)
typedef struct Events {
    char sendingNode;
    char destinationNode;
    char transferDetails[300];
    char majorTime[50]; // The time that is displayed in the bottom right of the diagram
    // An array of 100 strings each of length 256
    //char minorEventList[256][100];
    int numMinorEvents;
}Events;

// This will store ALL (relevant) minor events, so we need a lot of space.
char globalMinorEventArray[2000][256];
static struct Events listEvents[200];
int listEventsLength = 0;
int currentEventIndex = 0;
int numGlobalMinorEvents = 0;


// ============= FOR PROCESSING INPUT LOG
FILE* outputFile;
FILE* fptr;

// Used for the sections of LBARD where it measures in T+[x]ms. Just takes the time
// that /that/ specific LBARD process was started as a long.
long long lbardInitialTime = 0;
int setupLineNumber = 0;
char instanceChar = '-';
char process[20] = "";

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

// ============= NETWORK LAYOUT
struct FakeradioLink{
    char node1;
    char node2;
};

#define MAX_LINKS 50
struct FakeradioLink allFakeradioLinks[MAX_LINKS];
int numFakeradioLinks = 0;

// Max of 26 per interface because there's a max of 26 nodes.
char RhizomeInterfaces[MAX_LINKS][26];
int indexRhizomeInterfaces[MAX_LINKS];


// Max nodes allowed in the test is 26 (A->Z) 
char sidArray[26][65];
int numSidArray = 0;


// ============= FUNCTION PROTOTYPES
void createDotFile();

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

long timestamp_to_long(const char line[]);

char get_node_from_sid(char sid[]);

int eventSort (const void* a, const void* b);

void add_to_minor_events(const char line[]);

void writeLine(char line[], FILE* logFile);
