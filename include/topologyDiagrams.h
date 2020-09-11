#include <stdio.h>
#include <stdlib.h>
#include <regex.h>     
#include <pcre.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>


// ============= TEST DETAILS
typedef struct TestDetails{
    char testName[100];
    char result[10];
    char timeStarted[25];
    char timeFinished[25];
}TestDetails;

// ============= EVENTS (for DOT diagram generation)
typedef struct MinorEvents{
    char majorTime[20];
    char message[256];
}MinorEvents;

// This will store ALL (relevant) minor events, so we need a lot of space.
static struct MinorEvents globalMinorEventArray[2000];
int numGlobalMinorEvents = 0;

typedef struct Events {
    char sendingNode;
    char destinationNode;
    char transferDetails[300];
    char majorTime[50]; // The time that is displayed in the bottom right of the diagram
    char eventType; 
    /*
    Event types: (Can be expanded later to handle more types)
    W : Wifi
    F : Fakeradio
    */
    struct MinorEvents minorEventList[200];
    int numMinorEvents;
}Events;

static struct Events listEvents[1000];
int listEventsLength = 0;
int currentEventIndex = 0;


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

// ============= CREATING LATEX FILE
FILE *latexFile;


// ============= FUNCTION PROTOTYPES
void usage();

void createDotFile();

void createLATEXFile(const struct Events ev, const char *outputImageName, const int isLastEvent);

void setProcess(const char proc[]);

int isLineRelevant(const char line[]);

void writeRhizomePacket(char line[]);

void writeRhizomeSendPacket(char line[]);

void writeRhizomeAddManifest(char line[]);

void writeNodeInfo(char line[]);

void writeLBARDBundle(char line[]);

void writeLBARDSend(char line[]);

void writeRhizomeLayout(char line[]);

void writeFakeradioBundle(char line[]);

char *longToTimestamp(long tmpLong);

static char *convertLBARDTime(long initialTime, char *timeStamp);

void fixLBARDTimestamp(char timeStamp[]);

long timestampToLong(const char line[]);

char getNodeFromSid(char sid[]);

void addToMinorEvents(const char line[]);

void shortenBID(const char longBID[], char shortBID[]);


char shortenSID(const char longSID[], char shortSID[]);


int eventSort(const void* a, const void* b);

int minorEventSort(const void* a, const void* b);

void writeLineToLog(char line[]);
