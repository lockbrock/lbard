/*
    Written as part of a 2020 Honours thesis by Lachlan Brock for
    a Bachelor of Software Engineering(Honours).
    For more information, see: https://github.com/lockbrock/flinders-thesis 

    Creates a simple log file from a given test file. 
    TBD: Add more as features added.
*/
#include "topologyDiagrams.h"

/*
    TODO:
    #- Get all the SID and instance letter(number) of each node
    #- Get LBARD start time for T+ lines
    #- Detect when Fakeradio logs
    #- Detect when ServalDna logs
    - Set writeline to use global variable file
    #- Move fakeradio rules in to SETUP
    #- Sort by timestamp but ignore setup section
    - Fix all the warnings: remove the -w from the Makefile
    - Call log from command line, produce simple log in same directory
    #- Work out some way of showing when/where a bundle is /ADDED/ (ie. by a test)
    - Sort and uniq the output - some lines are identical for whatever reason
*/


// TO BE SET WHEN PROCESSING - SO WE KNOW WHAT LOG/INSTANCE WE'RE ON
char instanceChar = '-';
char process[20] = "";
FILE* outputFile;
FILE* fptr;
// Used for the sections of LBARD where it measures in T+[x]. Just takes the time
// that /that/ specific LBARD process was started as a long.
long long lbardInitialTime = 0;
int setupLineNumber = 0;


int main(){
    // TODO: Get this from command line
    char filename[] = "./tempLog.txt";
    int bufferLength = 255;
    char buffer[bufferLength];

    if ((fptr = fopen(filename,"r")) == NULL){
        printf("Error opening log file.");
        // Program exits if the file pointer returns NULL.
        exit(1);
    }

    // TODO: Get output file/folder from command line
    if (fopen("./simpleLogOutput.txt","w") == NULL){
        printf("Error creating/opening output file.");
        // Program exits if the file pointer returns NULL.
        exit(1);
    }else{
        outputFile = fopen("./simpleLogOutput.txt","w");
        // Erase the log file
        write(outputFile, "", 1);
    }
    printf("Opened both files\n");
    setProcess("NONE");

    while(fgets(buffer, bufferLength, fptr)) {
    // If line matches one of our important lines then send it to a function to pretty
    // the output and write it to the simple log file.
        char msg[100] = "";
        int res = isLineRelevant(buffer);

        if(res != NULL){
            // TODO: Move each of these to a function to pretty the output & then write
            switch(res){

                case PROCESS:;
                // Process line
                    // Then the beginning of a process so don't print it
                    //sprintf(msg, "File: %s:%c\n", process, instanceChar);
                    //writeLine(msg, outputFile);
                    break;
                case RHIZOME_SEND_PACKET:
                case RHIZOME_RECEIVE_PACKET:
                    print_rhizome_packets(buffer);
                    break;
                case RHIZOME_ADD_MANIFEST:
                    print_rhizome_add_manifest(buffer);
                    break;
                case NODE_INFOMATION:
                    print_node_info(buffer);
                    break;
                case LBARD_RECEIVE_PACKET:
                case LBARD_NEW_BUNDLE:
                    print_lbard_bundle(buffer);
                    break;
                case LBARD_SEND_PACKET:
                    print_lbard_send(buffer);
                    break;
                case FAKERADIO_RULES:
                    // So that once sorted fakeradio rules is up the top in the setup section
                    sprintf(msg, "#%03d %s", setupLineNumber, buffer);
                    setupLineNumber += 1;
                    writeLine(msg, outputFile);
                    break;
                case FAKERADIO_BUNDLE:
                    print_fakeradio_bundle(buffer);
                    break;
                case RHIZOME_LAYOUT:
                    print_rhizome_layout(buffer);
                    break;
                case -1:
                    printf("Error processing line: %s", buffer);
                    break;
                case 0:
                    break;
                default:
                    writeLine(buffer, outputFile);
                    //msg[100] = "";
                    printf("");
                    //printf("No format for: %s\n", buffer);
            }
        }
    }
    fclose(fptr);
    printf("Finished reading input file\n");
    return 0;
}
/**
 *  Set the process variable. Do it in this function so that we can do 
 *  specific things on certain process changes (ie. begin/end setup)
 */
void setProcess(const char proc[]){
    //printf("PROCESS: %s\n", proc);
    char buffer[50] = "";
    if (strcmp(process, "NONE") == 0){
        // Should ALWAYS be the first line
        sprintf(buffer, "#%03d ====== BEGIN SETUP ======\n", 0);
        setupLineNumber += 1;
        writeLine(buffer, outputFile);
    }else if (!strcmp(process, "SETUP")) {
        // Ideally we don't have more than 99 lines in the setup. If so, just increase the 100
        sprintf(buffer, "#%03d ======= END SETUP =======\n", 100);
        writeLine(buffer, outputFile);
    }
    strcpy(process, proc);
}

/**
 *  Checks if the line sent to it is a line relevant to the simple log file
 *  Simply checks if it contains specific key words (it's good enough)
 *  Returns an enum value signifying which type of line it is
 *  May in some cases change the process value depending on the line.
 */
int isLineRelevant(const char line[]){
    char v[100] = "";
    char tmp[1000] = "";
    char instance[100] = "";
    

    // Parsing start of a log file
    if (strstr(line, "++++++++++") != NULL){
        instanceChar = '-';
        //printf("The substring is: %s\n", line);

        sscanf(line, "%s %s %s", &tmp, &tmp, &v);

        // If LBARD
        if (strstr(v, "lbard") != NULL){
            // TODO: Get the instance number from here
            setProcess("LBARD");
            sscanf(line, "%s %s %s", &tmp, &tmp, &instance);
            // Get the last character of the string 'LBARDX'
            if (strlen(instance) == 7){
                instanceChar = instance[6];
            }else{
                return -1;
            }

            return PROCESS;
        } else if (strstr(v, "fakeradio")){
            setProcess("FAKERADIO");
            return PROCESS;
        }
        return 0;
    }

    if (strcmp(process, "NONE") == 0){
        // We haven't even gotten to the log.stdout bit yet
        //Name:     Lab_Message (topologies)
        //Result:   FAIL
        //Started:  2020-07-27 22:01:18.828
        //Finished: 2020-07-27 22:04:23.278
        if (strstr(line, "Name:") || strstr(line, "Result:") || strstr(line, "Started:") || strstr(line, "Finished")){
            //sscanf(line, "%s: %[^\t\n]", &tmp, &buffer);
            setupLineNumber += 1;
            sprintf(tmp, "#%03d %s", setupLineNumber, line);
            writeLine(tmp, outputFile);
        }
    }

    if (strcmp(process, "SETUP") == 0){
        // Still in the setup log section
        if (strstr(line, "is being connected to Wifi")){
            //16:04:25.418 Servald instance B is being connected to Wifi interface {1}
            return RHIZOME_LAYOUT;
        }
    }


    // ServalD log 
    // 13:10:27.512 #----- var/servald/instance/D/servald.log -----
    if (strstr(line, "#-----") && strstr(line, "servald.log")){
        setProcess("SERVALD");
        sscanf(line, "%s %s %s", &tmp, &tmp, &instance);
        //strcpy(tmp, "");
        sscanf(instance, "var/servald/instance/%s", &instance);
        // Get the last character of the string 'LBARDX'
        if (strlen(instance) > 1){
            instanceChar = instance[0];
            return PROCESS;
        }
        return -1;
    }

    if (strcmp(process, "SERVALD") == 0){
        if (strstr(line, "mdprequests") && !(strstr(line, "debug.mdprequests"))){
            // TODO: Add more filters for mdprequest log lines
            if (strstr(line, "send_frame")){
                // This just shows packets, NOT when a bundle is sent. How do we find this?
                return RHIZOME_SEND_PACKET;
            }
        }
        if (strstr(line, "sync_keys") && !(strstr(line, "debug.rhizome_sync_keys"))){
            return RHIZOME_RECEIVE_PACKET;
        }
        if (strstr(line, "ADD MANIFEST")){
            return RHIZOME_ADD_MANIFEST;
        }
        return 0;
    }
    
    // Filtering for LBARD 
    if (strcmp(process, "LBARD") == 0){
        if (strstr(line, "executeOk --core-backtrace --timeout=600 --executable=lbard")){
            //18:45:06.376 # executeOk --core-backtrace --timeout=600 --executable=lbard --stdout-file=E_LBARDOUT --stderr-file=E_LBARDERR 127.0.0.1:4114 lbard:lbard 9280164C7615E9E61E954219E39B531BB5CF032EA1BCE6CFF0A21ED517FE1A5A FEC1C61CCE2A2E70FE4135C6463980D907915FDA12F352C2C1A567FB1B35A76A /dev/pts/6 announce pull bundles
            lbardInitialTime = lbard_timestamp_to_long(line);
        }

        if(strstr(line, "HAS some bundle")){
            return LBARD_RECEIVE_PACKET;
        }
        if (strstr(line, "We have the entire bundle")){
            return LBARD_RECEIVE_PACKET;
        }
        if (strstr(line, "We have new bundle")){
            return LBARD_NEW_BUNDLE;
        }
        if (strstr(line, "Sending length")){
            return LBARD_SEND_PACKET;
        }
        if (strstr(line, "Resending bundle")){
            return LBARD_SEND_PACKET;
        }
    }
    
    if (strcmp(process, "FAKERADIO") == 0){
        // Fakeradio Rules
        if (strstr(line, "RULE:")){
            return FAKERADIO_RULES;
        }
        if (strstr(line, ">>>") && strstr(line, "RADIO")){
            return FAKERADIO_BUNDLE;
        }
    }

    // GET SID of each node
    if (strstr(line, "SID") && !(strstr(line, "My"))){
        return NODE_INFOMATION;
    }
    return 0;
}

/**
 *  For when Rhizome send/receives a packet
 *  Simply prints the relevant lines in a nicely formatted and consistent 
 *  manner
 */
void print_rhizome_packets(char line[]){
// Both the send and receive use the same format
    char msg[1000] = "";
    char timeStamp[16];
    char tmp[100];
    char details[500];
    if (strstr(line, "httpd")){
        // These lines have an additional field
        sscanf(line, "%s %s %s %s %s %[^\t\n]", &tmp, &timeStamp, &tmp, &tmp, &tmp, &details);

    }else{
        sscanf(line, "%s %s %s %s %[^\t\n]", &tmp, &timeStamp, &tmp, &tmp, &details);
    }
    // format: [timestamp] SERVALD:[X] [details]
    sprintf(msg, "%s SERVALD:%c %s\n", timeStamp, instanceChar, details);
    writeLine(msg, outputFile);
}

/**
 *  For when a bundle is added to Rhizome
 *  Simply prints the relevant lines in a nicely formatted and consistent 
 *  manner
 */
void print_rhizome_add_manifest(char line[]){
    //INFO: [287944] 13:10:19.844 [httpd/4] rhizome_database.c:1501:rhizome_add_manifest_to_store()  RHIZOME ADD MANIFEST service=file bid=3889B11DAFB5C6814EFB77EA2D4BE65F059D5006F08933D3C37CA1AD764D5FAA version=1595389191543
    char msg[1000] = "";
    char timeStamp[16];
    char tmp[100];
    char details[500];
    sscanf(line, "%s %s %s %s %s %[^\t\n]", &tmp, &tmp, &timeStamp, &tmp, &tmp, &details);
    sprintf(msg, "%s SERVALD:%c %s\n", timeStamp, instanceChar, details);
    writeLine(msg, outputFile);
}

/**
 *  For printing node SIDs
 *  Simply prints the relevant lines in a nicely formatted and consistent 
 *  manner
 */
void print_node_info(char line[]){
    char instance[5] = "";
    // SID is always 64 chars long.. We need the \0 at the end so 65
    char sid[65] = "";
    char msg[100];
    memset(msg, 0, strlen(msg));
    char tmp[50] = "";

    if (strcmp(process, "NONE") == 0){
        setProcess("SETUP");
    } 
    // TODO: Extract SID infomation from line here
    sscanf(line, "%s %[^=]=%s", &tmp, &instance, &sid);
    if (strlen(instance) == 4){
        sprintf(msg, "#%03d %s : %s\n", setupLineNumber, instance, sid);
        setupLineNumber += 1;
        writeLine(msg, outputFile);
    }
}

/**
 *  For when LBARD receives a bundle OR a bundle is added (ie. via
 *  Rhizome)
 *  Simply prints the relevant lines in a nicely formatted and consistent 
 *  manner
 */
void print_lbard_bundle(char line[]){
    //>>> [13:10.21.821 7408*] Peer 7a43982622b4* HAS some bundle that we don't have (key prefix=48CC*).
    //>>> [18:45.25.432 3F76*] We have the entire bundle 92aefeabda1687ad*/1595582106764 now.

    char msg[1000] = "";
    char timeStamp[16];
    char tmp[100];
    char details[500];
    sscanf(line, "%s %s %s %[^\t\n]", &tmp, &timeStamp, &tmp, &details);
    
    fix_lbard_timestamp(timeStamp);
    sprintf(msg, "%s LBARD:%c %s\n", timeStamp, instanceChar, details);
    writeLine(msg, outputFile);
}

/**
 *  For when LBARD sends/resends a bundle
 *  Simply prints the relevant lines in a nicely formatted and consistent 
 *  manner
 */
void print_lbard_send(char line[]){
    //T+27471ms : Sending length of bundle 92AEFEABDA1687ADC16BBECFD5B09A66296DE14F6D2007474212601FA0933B5C (bundle #0, version 1595582106764, cached_version 1595582106764)
    char msg[1000] = "";
    char timeStamp[50] = "";
    char tmp[100] = "";
    char details[500] = "";
    sscanf(line, "%s %s %[^:\t\n]", &timeStamp, &tmp,  &details);
    const char* timestamp = convert_lbard_time(lbardInitialTime, timeStamp);
    sprintf(msg, "%s LBARD:%c %s\n", timestamp,
        instanceChar, details);
    writeLine(msg, outputFile);
}

void print_rhizome_layout(char line[]){
    char msg[1000] = "";
    char tmp[100] = "";
    char instChar = '-';
    char interface = '-';
    //printf("line: %s\n", line);
    //16:04:25.418 Servald instance B is being connected to Wifi interface {1}
    // This looks ugly but it's just getting the instance character and the Wifi interface character
    // and dumping the rest.
    sscanf(line, "%s %s %s %c %s %s %s %s %s %s {%c}", \
        &tmp, &tmp, &tmp, &instChar, &tmp, &tmp, &tmp, &tmp, &tmp, &tmp, &interface);

    //printf("Instance: %c connected to %c\n", instChar, interface);
    sprintf(msg, "#%03d %c connected to wifi interface %c\n", setupLineNumber, instChar, interface);
    setupLineNumber += 1;
    writeLine(msg, outputFile);
}

void print_fakeradio_bundle(char line[]){
    // ftell & fseek to go forwards/backwards
    int bufferLength = 255;
    char buffer[bufferLength];
    char msg[bufferLength];
    char tmp[50] = "";
    char sendNode[2] = "";
    char destNode[2] = "";
    char byteLength[10] = "";
    char messageType[50] = "";
    char timeStamp[20] = "";
    char bundleID[32] = "";
    char tmpC = '-';    
    char startBytes[10] = "";
    char endBytes[10] = "";
    char sentBytes[10] = "";
    int startLine = ftell(fptr);


    //>>> [16:38.12.334 RADIO] @ T+14666ms: 53 bytes, 2 packets, 24 sync bytes, 0 manifest bytes, 0 body bytes, 0 colissions, 26.9% channel utilisation.
    sscanf(line, "%s %[^R]", &tmp, &timeStamp);
    fix_lbard_timestamp(timeStamp);

    int hasMessage = 0;
    
    // By default move to next line
    fgets(buffer, bufferLength, fptr);
    int maxLines = 0;
    while(!strstr(buffer, ">>>") && (maxLines < 20)){
        // Process the line
        //printf("line: %s", buffer);
        // Every message ends with "There are [x] filter rules"
        if (hasMessage && strstr(buffer, "There are")){
            char destLetter = '-';
            char sendLetter = '-';
            destLetter = atoi(destNode) + 65;
            sendLetter = atoi(sendNode) + 65;
            if (strcmp(messageType, "unknown") != 0){
                // Unknown types are broadcast and specific to fakeradio
                // Since we're already logging after they're broadcast this is redundant
                if (strstr(messageType, "Bundle piece")){
                    if (strcmp(startBytes, "")){
                        sprintf(msg, "%sFAKERADIO %c -> %c [partial bundle]  bid=%s [%s to %s. Total: %s bytes]\n", timeStamp, sendLetter, destLetter, bundleID, startBytes, endBytes, byteLength);
                    }else{
                        sprintf(msg, "%sFAKERADIO %c -> %c [bundle piece]  bid=%s [%s bytes]\n", timeStamp, sendLetter, destLetter, bundleID, byteLength);
                    }
                }else{
                    //sprintf(msg, "%sFAKERADIO (type: %s) %s -> %s of size %s\n", timeStamp, messageType, sendNode, destNode, byteLength);
                    sprintf(msg, "%sFAKERADIO %c -> %c [%s] [%s bytes]\n", timeStamp, sendLetter, destLetter, messageType, byteLength);
                }
            }
            // Just clear the variables so we don't have issues
            writeLine(msg, outputFile);
            hasMessage = 0;
        }


        if (strstr(buffer, "Filter and enqueue")){
            // Start of a new transfer so clear these
            memset(bundleID, 0, strlen(bundleID));
            memset(messageType, 0, strlen(messageType));
            memset(sendNode, 0, strlen(sendNode));
            memset(destNode, 0, strlen(destNode));
            memset(byteLength, 0, strlen(byteLength));
            memset(tmp, 0, strlen(tmp));

            //printf("line11 %s", buffer);
            // NOTE: This is the /previous/ transfer, if one exists (multiple can occur) per radio command

            // Filter and enqueue 52 bytes from 3 -> 2
            // clear the message variable
            memset(msg, 0, bufferLength);
            sscanf(buffer, "%s %s %s %s %s %s %s %s %s",
                &tmp, &tmp, &tmp, &byteLength, &tmp, &tmp, &sendNode, &tmp, &destNode
            );

            //printf("f&e %s", buffer);
        }

        if (strstr(buffer, "Fragment type")){
            //Fragment type 'T' : Time stamp
            sscanf(buffer, "%s %s %s %s %[^()\t\n]", &tmp, &tmp, &tmp, &tmp, &messageType);
            // Every message must have a type so IF we have a type then assume its a message
            hasMessage = 1;
        }
        if (strstr(buffer, "bid")){
            sscanf(buffer, "%[^=] %c %s", &tmp, &tmpC, &bundleID);
        }

        if (strstr(buffer, "body bytes")){
            //body bytes [3456..3583] (128 bytes) @ T+27694ms
            //[ 3456..3582] [128 bytes]

            sscanf(buffer, "%s %s [%[^.].%c %[^]] %c (%s", &tmp, &tmp, &startBytes, &tmpC, &endBytes, &tmpC, &sentBytes);
            //printf("Sent bytes: %s to %s | total: %s\n", startBytes, endBytes, sentBytes);
        }

        // Increment line by one
        fgets(buffer, bufferLength, fptr);
        maxLines += 1;
    }
    
    // Go back to our starting line - prevents accidentally missing lines from going too far 
    // (causes issues with LBARD which is right after in the log)
    fseek(fptr, startLine, SEEK_SET);
}

/**
 *  Converts a long of milliseconds since the 00:00:00.00 to when test is
 *  conducted to a timestamp. 
 *  NOTE: May break at times around midnight, this has not been tested.
 */
char *timestamp_str(long tmpLong){
    char *buffer = ((char*) malloc((50)*sizeof(char)));

    time_t time = tmpLong /1000;
    struct tm tm;
    gmtime_r(&time, &tm);
    
    long milli = tmpLong %1000;


    snprintf(buffer,50,"%02d:%02d:%02d.%03ld",
        tm.tm_hour, tm.tm_min, tm.tm_sec, milli);
    return buffer;
}


/**
 * Converts a timestamp in the form T+[x]ms to a timestamp
 * NOTE: Most of the time, this will be called with lbardInitialTime as
 * the first variable.
 */
char *convert_lbard_time(long initialTime, char *timeStamp){
    // Get the number out of the timestamp
    // T+44432ms
    char tmp[10] = "";
    sscanf(timeStamp, "%[^m]", tmp);
    strcpy(tmp, *(&tmp +1));
    
    // Always starts with T+ soooo just skip it..
    char* timeStampPtr = tmp + 2; 
    strcpy(timeStamp, timeStampPtr);
    
    // Add to initial time
    long timeStampMilli = atoi(timeStamp) + initialTime; 
    
    // Convert to timestamp
    char* buf = timestamp_str(timeStampMilli);
    return buf;
}

/**
 * Fixes LBARD timestamps so they are consistent with servald timestamps
 * Simply a matter of removing a '[' and changing a '.' to a ':'
 */
void fix_lbard_timestamp(char timeStamp[]){
    // Timestamp still has that beginning [ so cut that off.
    char* timeStampPtr = timeStamp + 1;
    strcpy(timeStamp, timeStampPtr);
    // Yes, I could just use &timeStampPtr in sprintf.. but I like consistency.

    // Annoyingly. The LBARD timestamp has a . instead of a : in the timestamp..
    char* dot = strstr(timeStamp, ".");
    *dot = ':';
    // Alters the original string so no need to return
}

/**
 *  Converts a LBARD timestamp (T+[x]ms) to a long of milliseconds since
 *  00:00:00.000 that day.
 *  NOTE: May break around midnight, this is untested 
 */
long lbard_timestamp_to_long(const char line[]){
    // Why do we do it like this and not just change LBARD?
    /// Well, hypothetical question, this is because we are going to be comparing these logs
    // against existing topologies and we don't want to have to update LBARD on them too!
    
    char timeStamp[16] = "";
    sscanf(line, "%s", &timeStamp);

    // We'll be getting time from 00:00:00.000
    //                            hh:mm:ss.mss
    // Will this break if done right on midnight? Probably. That's an edge case and I don't care!
    char delim[2] = ":";
    char *hourText;
    hourText = strtok(timeStamp, delim);
    char *minuteText;
    minuteText = strtok(NULL, delim);
    // Note this tricky bit. We need to change the value here /WITHOUT/ changing
    // the variable we're sending to the function.
    *delim = '.';
    char *secondText;
    secondText = strtok(NULL, delim);
    char *milliText;
    milliText = strtok(NULL, delim);

    // Convert to milliseconds
    long hourLong = atoi(hourText) * 3600000;
    long minuteLong = atoi(minuteText) * 60000;
    long secondLong = atoi(secondText) * 1000;
    long milliLong= atoi(milliText);
    //printf("converted to longs: hh:%d, mm:%d, ss:%d ms:%d\n", hourLong, minuteLong, secondLong, milliLong);
    long totalMilliseconds = hourLong + minuteLong + secondLong + milliLong;
    return totalMilliseconds;
}

void writeLine(char line[], FILE* logFile){
    // TODO: Just call the global variable instead of passing it every time, unnecessary
    fputs(line, logFile);
}