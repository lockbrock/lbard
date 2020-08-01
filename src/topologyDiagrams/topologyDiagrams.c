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
    #- Fix all the warnings: remove the -w from the Makefile
    - Call log from command line, produce simple log in same directory
    #- Work out some way of showing when/where a bundle is /ADDED/ (ie. by a test)
    - Sort and uniq the output - some lines are identical for whatever reason
*/

// TODO: Set to 1 if command line option
int createDot = 1;

int main(){
    // TODO: Get this from command line
    char filename[] = "./tempLog2.txt";
    int bufferLength = 500;
    char buffer[bufferLength];

    if ((fptr = fopen(filename,"r")) == NULL){
        printf("Error opening log file.");
        // Program exits if the file pointer returns NULL.
        exit(1);
    }

    // TODO: Get output file/folder from command line
    if ((outputFile = fopen("./simpleLogOutput.txt","w")) == NULL){
        printf("Error creating/opening output file.");
        // Program exits if the file pointer returns NULL.
        exit(1);
    }else{
        //outputFile = fopen("./simpleLogOutput.txt","w");
        // Erase the log file
        write(outputFile, "", 1);
    }
    printf("Opened both files\n");
    setProcess("NONE");
    while(fgets(buffer, bufferLength, fptr)) {
    // If line matches one of our important lines then send it to a function to pretty
    // the output and write it to the simple log file.
        char msg[100] = "";
        char tmp[50] = "";
        int res = isLineRelevant(buffer);

        if(res != NULL){
            switch(res){
                case PROCESS:
                    // Do nothing, doesn't need to be printed.
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
                    if (strstr(msg, "allow between")){
                        int nodeA = 0;
                        int nodeB = 0;
                        //printf("yo\n");
                        sscanf(buffer, "RULE: 'allow between %d,%d", &nodeA, &nodeB);
                        //printf("nodeA: %c, nodeB: %c\n", nodeA+65, nodeB+65);
                        struct FakeradioLink fl;
                        fl.node1 = nodeA+65;
                        fl.node2 = nodeB+65;
                        allFakeradioLinks[numFakeradioLinks] = fl;
                        numFakeradioLinks += 1;
                    }
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
                    printf("");
            }
        }
    }
    fclose(fptr);
    fclose(outputFile);
    printf("Finished reading input file\n");

    if (createDot){
        createDotFile();
    }

    return 0;
}

/**
 *  The method used when creating dot files.
 *  Simply writes a new file that contains the entire network topology of the test
 *  TODO: Iterate through each major event (transmission of bundle) and display that on the diagram
 *  TODO: Add each minor event to the side using a LaTeX template.
 */
void createDotFile(){
    if ((outputFile = fopen("./dotFileTest.dot","w")) == NULL){
        printf("Error creating/opening output file.");
        // Program exits if the file pointer returns NULL.
        exit(1);
    }else{
        // Erase the log file
        write(outputFile, "", 1);
    }

    printf("Now printing the events\n");


    // I'm keeping it all nicely formatted when it's produced so this bit is a little ugly
    // The digraph needs to be strict since when network packets are shown they need to replace the
    // links already shown. This /may/ be able to be removed if the processing to remove existing edges
    // if a packet link is added
    // This unfortunately does mean that Wifi/Fakeradio links between the same nodes overlap and so only wifi is shown
    const char *startText =  "\
// Auto-generated by topologyDiagrams.c \n\
strict digraph A {\n\
    size=\"11.7,8.3!\"\n\
    ratio=fill\n\
    pad=1\n\
    mindist=0.5\n\
    subgraph Nodes {\n\
        node [pin=true]\n";
    fputs(startText, outputFile);
    for(int i = 0; i < numSidArray; i++){
        char msg[5] = "";
        sprintf(msg, "\t\t%c\n", i+65);
        fputs(msg, outputFile);
    }

    // TODO: Sort all of the major/minor events
    //          minor events may need a struct defined.. to easily do this
    //      convert them all to longs to compare them

    // Current event index /should/ be at the end
    listEventsLength = currentEventIndex;
    // Sort the major event array
    qsort(listEvents, listEventsLength, sizeof(struct Events), eventSort);

    for (int i = 0; i < listEventsLength; i++){
        struct Events ev = listEvents[i];
        printf("%d: Event: %s \n%c -> %c\n", (i+1), ev.transferDetails, ev.sendingNode, ev.destinationNode);
        printf("timestamp: %s\n\n", ev.majorTime);
    }

    // =============== Layout ===============  
    const char *layoutText = "\
    }\n\
    subgraph Layout {\n\
        edge [dir=none, penwidth=3]\n";
    fputs(layoutText, outputFile);

    // TODO: Add the transfer bits here (needs to be before the layout for arrow directions)
    // Perhaps, we move all the fputs() to the end. That way we don't have to process anything new
    // except the transfers..

    // FAKERADIO
    // TODO: Find some way of colouring these depending on the type of fakeradio node?
    // ie. red for RFD900, green for Codan, etc.
    for (int i = 0; i < numFakeradioLinks; i++){
        char msg[48] = "";
        sprintf(msg, "\t\t%c -> %c [color=red]\n", allFakeradioLinks[i].node1, allFakeradioLinks[i].node2);
        fputs(msg, outputFile);
    }

    // WIFI
    // Wifi goes last because it hides fakeradio links (wifi will almost always be 
    // used over fakeradio between two nodes)

    // For each interface, add the link
    for(int interface = 0; interface < MAX_LINKS; interface++){
        char listNodes[26] = "";
        strcpy(listNodes, RhizomeInterfaces[interface]);

        if (strlen(listNodes) > 1){
            for (int i = 0; i < strlen(listNodes) -1; i++){
                char charA = listNodes[i];
                for (int j = i +1; j < strlen(listNodes); j++){
                    char msg[48] = "";
                    sprintf(msg, "\t\t%c -> %c [color=blue]\n", charA, listNodes[j]);
                    fputs(msg, outputFile);
                }
            }
        }
    }
    const char *endLayout = "\
    }\n";
    fputs(endLayout, outputFile);

    const char *endBrace = "}";
    fputs(endBrace, outputFile);
    fclose(outputFile);
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


// Store the SID of the destination in the mdprequests
char rhizomeDestination[65] = "";
char frameMessage[100] = "";
char frameBID[65] = "";
char dotMsg[256] = "";
int readyToSend = 0;

/**
 *  Checks if the line sent to it is a line relevant to the simple log file
 *  Simply checks if it contains specific key words (it's good enough)
 *  Returns an enum value signifying which type of line it is
 *  May in some cases change the process value depending on the line.
 *  Does some processing for adding major events for Servald.
 *  TODO:   This probably should be moved to the relevant functions but this is TBD.
 */
int isLineRelevant(const char line[]){
    char tprocess[100] = "";
    char tmp[1000] = "";
    char instance[100] = "";

    // Parsing start of a log file
    if (strstr(line, "++++++++++") != NULL){
        instanceChar = '-';
        //printf("The substring is: %s\n", line);

        sscanf(line, "%s %s %s", &tmp, &tmp, &tprocess);

        // If LBARD
        if (strstr(tprocess, "lbard") != NULL){
            setProcess("LBARD");
            sscanf(line, "%s %s %s", &tmp, &tmp, &instance);
            // Get the last character of the string 'LBARDX'
            if (strlen(instance) == 7){
                instanceChar = instance[6];
            }else{
                return -1;
            }

            return PROCESS;
        } else if (strstr(tprocess, "fakeradio")){
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
            if (strstr(line, "send_frame")){
                if (strstr(line, "Attempting to queue mdp packet from")){
                    // We need to get the destination of a packet. That's all this bit is
                    // broadcast ones are to anyone on the network, not super relevant. We're kinda just ignoring it
                    if (createDot){
                        char tempMsg[300] = "";
                        //DEBUG:[584836] 15:23:17.870 overlay_mdp.c:817:_overlay_send_frame()  {mdprequests} Attempting to queue mdp packet from 8AEF473837C24E56ADD9C864228A0B453BF70161639D851A5FBC150E15977052:18 to 2BEF74429C78AEB1347B6182E8689F9B9644EDF74379D7C0635F5649D85A2F39:18
                        //printf("test %s\n", line);
                        memset(tempMsg, 0, strlen(tempMsg));
                        memset(rhizomeDestination, 0, strlen(rhizomeDestination));
                        sscanf(line, "%s %s %s %s %[^\t\n]", &tmp, &tmp, &tmp, &tmp, &tempMsg);
                        sscanf(tempMsg, "%s %s %s %s %s %s %s %s %[^:]", &tmp, &tmp, &tmp, &tmp, &tmp, &tmp, &tmp, &tmp, &rhizomeDestination);
                        readyToSend = 1;
                    }
                }
                if (strstr(line, "Send frame")){
                    // TODO: Add it so it actually logs the same as this!!
                    // TODO: Move this to the relevant print function - shouldn't be in filter section
                    if(createDot && readyToSend && strcmp(frameMessage, "") != 0){
                        // We need to get the frame size
                        char numBytes[5] = "";
                        char timeStamp[20] = "";
                        //memset(timeStamp, 0, strlen(timeStamp));

                        //DEBUG:[584682] 15:22:58.209 overlay_mdp.c:859:_overlay_send_frame()  {mdprequests} Send frame 9 bytes
                        sscanf(line, "%s %s %s %s %s %s %s", &tmp, &timeStamp, &tmp, &tmp, &tmp, &tmp, &numBytes);
                        // Now we make this into a major event
                        
                        //[SEND_MANIFEST] Send frame 464 bytes to 8AEF473837C24E56ADD9C864228A0B453BF70161639D851A5FBC150E15977052
                        char sidChar = get_node_from_sid(rhizomeDestination);
                        struct Events currentEvent;
                        currentEvent.destinationNode = sidChar;
                        currentEvent.sendingNode = instanceChar;
                        sprintf(dotMsg, "[%s] [%s bytes]", frameMessage, numBytes);
                        strcpy(&currentEvent.transferDetails, dotMsg);
                        strcpy(&currentEvent.majorTime, timeStamp);
                        //printf("index: %d", currentEventIndex);
                        listEvents[currentEventIndex] = currentEvent;
                        currentEventIndex += 1;

                        //currentEvent.numMinorEvents = 0;
                        //memset(timeStamp, 0, strlen(timeStamp));
                        memset(dotMsg, 0, strlen(dotMsg));
                        memset(frameMessage, 0, strlen(frameMessage));
                        readyToSend = 0;
                    }
                }
                return RHIZOME_SEND_PACKET;
            }
        }
        if (strstr(line, "sync_keys") && !(strstr(line, "debug.rhizome_sync_keys"))){
            if (strstr(line, "Queued transfer message")){
                if (createDot){
                    // We're saving all of these so that they're classified as a major event
                    char tempTransferMessage[255] = "";
                    //DEBUG:[584682] 15:23:17.868 rhizome_sync_keys.c:157:find_and_update_transfer()  {rhizome_sync_keys} Queued transfer message SEND_MANIFEST 15EB502E2D2D9521
                    memset(tempTransferMessage, 0, strlen(tempTransferMessage));
                    memset(frameMessage, 0, strlen(frameMessage));
                    
                    sscanf(line, "%s %s %s %s %[^\t\n]", &tmp, &tmp, &tmp, &tmp, &tempTransferMessage);
                    sscanf(tempTransferMessage, "%s %s %s %s %[^\t\n]", &tmp, &tmp, &tmp, &frameMessage, &frameBID);
                }
                return RHIZOME_SEND_PACKET;
            }
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
            lbardInitialTime = timestamp_to_long(line);
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

    // TODO: Come back to this. We'll need to add some more filtering since not all the rhizome packets
    // should be minor events
    //if (createDot){
    //    add_to_minor_events(msg);
    //}
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
 *  Saves each SID to an array so we can convert SIDs to node chars easily
 */
void print_node_info(char line[]){
    char instance[5] = "";
    // SID is always 64 chars long.. We need the \0 at the end so 65
    char sid[65] = "";
    char msg[100] = "";
    memset(msg, 0, strlen(msg));
    char tmp[50] = "";

    // So that the SETUP bit is only called once, at the beginning
    if (strcmp(process, "NONE") == 0){
        setProcess("SETUP");
    } 

    sscanf(line, "%s %[^=]=%s", &tmp, &instance, &sid);
    if (strlen(instance) == 4){
        // Add it to the array so we can compare things to it later
        strcpy(sidArray[numSidArray], sid);
        numSidArray += 1;
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
    char timeStamp[16] = "";
    char tmp[100];
    char details[500];
    char t = '=';
    sscanf(line, ">>> %c %s %s %[^\t\n]", &t, &timeStamp, &tmp, &details);
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

/**
 *  For printing the rhizome layout of a network test.
 *  Just filters the given line for the instance ID, and what wifi interface it is connected to
 */
void print_rhizome_layout(char line[]){
    char msg[1000] = "";
    char tmp[100] = "";
    char instChar = '-';
    int interface = 0;
    //printf("line: %s\n", line);
    //16:04:25.418 Servald instance B is being connected to Wifi interface {1}
    sscanf(line, "%s %s %s %c %s %s %s %s %s %s {%d}", \
        &tmp, &tmp, &tmp, &instChar, &tmp, &tmp, &tmp, &tmp, &tmp, &tmp, &interface);

    sprintf(msg, "#%03d %c connected to wifi interface %d\n", setupLineNumber, instChar, interface);
    if (createDot){
        int indx = indexRhizomeInterfaces[interface];
        RhizomeInterfaces[interface][indx] = instChar;
        indexRhizomeInterfaces[interface] += 1;        
    }
    setupLineNumber += 1;
    writeLine(msg, outputFile);
}


/**
 *  For printing fakeradio messages.
 *  Goes from the start of the message >>> and then processes until the end of the message.
 *  Iterates through each line in the message and determines the message type (if any), the sender, the
 *  recipient, the bundle size, and the time.
 *  Prints it to the file.
 *  If creating a dot file, additionally save these messages as a major event (ie. a packet transfer) to be
 *  displayed on the dot file produced.
 */
char timeStamp[20] = "";
char msgDot[256] = "";
void print_fakeradio_bundle(char line[]){
    int bufferLength = 255;
    char buffer[bufferLength];
    char msg[bufferLength];
    char tmp[50] = "";
    char sendNode = '-';
    char destNode = '-';
    char byteLength[10] = "";
    char messageType[50] = "";
    char bundleID[32] = "";
    char tmpC = '-';    
    char startBytes[10] = "";
    char endBytes[10] = "";
    char sentBytes[10] = "";
    int startLine = ftell(fptr);
    memset(timeStamp, 0, strlen(timeStamp));
    memset(msgDot, 0, strlen(msgDot));


    //>>> [16:38.12.334 RADIO] @ T+14666ms: 53 bytes, 2 packets, 24 sync bytes, 0 manifest bytes, 0 body bytes, 0 colissions, 26.9% channel utilisation.
    sscanf(line, "%s %c %[^R]", &tmp, &tmpC, timeStamp);
    fix_lbard_timestamp(timeStamp);

    int hasMessage = 0;
    
    // By default move to next line
    fgets(buffer, bufferLength, fptr);
    int maxLines = 0;
    while(!strstr(buffer, ">>>") && (maxLines < 50)){
        // Process the line
        //printf("line: %s\n", buffer);
        // Every message ends with "There are [x] filter rules"
        if (hasMessage && strstr(buffer, "There are")){

            if (strcmp(messageType, "unknown") != 0){
                // Unknown types are broadcast and specific to fakeradio
                // Since we're already logging after they're broadcast this is redundant
                if (strstr(messageType, "Bundle piece")){
                    if (strcmp(startBytes, "")){
                        sprintf(msg, "%sFAKERADIO %c -> %c [partial bundle]  bid=%s [%s to %s. Total: %s bytes]\n", timeStamp, sendNode, destNode, bundleID, startBytes, endBytes, byteLength);
                        // The transfer message that will be displayed in the DOT file
                        sprintf(msgDot, "[partial bundle]  bid=%s [%s to %s. Length: %s bytes]", bundleID, startBytes, endBytes, byteLength);
                    }else{
                        sprintf(msg, "%sFAKERADIO %c -> %c [bundle piece]  bid=%s [%s bytes]\n", timeStamp, sendNode, destNode, bundleID, byteLength);
                        // The transfer message that will be displayed in the DOT file
                        sprintf(msgDot, "[bundle piece]  bid=%s [%s bytes]", bundleID, byteLength);
                    }
                }else{
                    sprintf(msg, "%sFAKERADIO %c -> %c [%s] [%s bytes]\n", timeStamp, sendNode, destNode, messageType, byteLength);
                    // The transfer message that will be displayed in the DOT file
                    sprintf(msgDot, "[%s] [%s bytes]", messageType, byteLength);
                }
                if (createDot){
                    struct Events currentEvent;
                    // Make sure to convert to the alpha representation of the node - NOTE the number
                    currentEvent.destinationNode = destNode;
                    currentEvent.sendingNode = sendNode;
                    strcpy(currentEvent.majorTime, timeStamp);
                    strcpy(currentEvent.transferDetails, msgDot);
                    //printf("current event: %s\ntimestamp: %s \n\n", currentEvent.transferDetails, currentEvent.majorTime);
                    listEvents[currentEventIndex] = currentEvent;
                    currentEventIndex += 1;
                    //currentEvent.numMinorEvents = 0;
                    memset(msgDot, 0, strlen(msgDot));
                    //memset(timeStamp, 0, strlen(timeStamp));
                }
                writeLine(msg, outputFile);
                hasMessage = 0;
            }
        }else{
            if (strstr(buffer, "Filter and enqueue")){
                // Start of a new transfer so clear these
                readyToSend = 0;
                sendNode = '-';
                destNode = '-';
                memset(bundleID, 0, strlen(bundleID));
                memset(messageType, 0, strlen(messageType));
                memset(byteLength, 0, strlen(byteLength));
                memset(tmp, 0, strlen(tmp));
                //memset(timeStamp, 0, strlen(timeStamp));

                //printf("line11 %s", buffer);
                // Filter and enqueue 52 bytes from 3 -> 2
                // clear the message variable
                memset(msg, 0, bufferLength);
                sscanf(buffer, "%s %s %s %s %s %s %c %s %c",
                    &tmp, &tmp, &tmp, &byteLength, &tmp, &tmp, &sendNode, &tmp, &destNode
                );
                // The nodes are integer values starting from 0 in the fakeradio section.
                // But we start from A. So just add 17 to the char value because ASCII table
                // Ascii char '0' is at value 48. A is at value 65.. 17 int difference
                sendNode = ((int)sendNode + 17);
                destNode = ((int)destNode + 17);

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
                printf("Sent bytes: %s to %s | total: %s\n", startBytes, endBytes, sentBytes);
            }


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
char timeStampConvert[20];
char *timestamp_str(long tmpLong){
    memset(timeStampConvert, 0, strlen(timeStampConvert));

    time_t time = tmpLong /1000;
    struct tm tm;
    gmtime_r(&time, &tm);
    
    long milli = tmpLong %1000;

    snprintf(timeStampConvert,50,"%02d:%02d:%02d.%03ld",
        tm.tm_hour, tm.tm_min, tm.tm_sec, milli);
    return timeStampConvert;
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
    //strcpy(timeStamp, timeStampPtr);
    //timeStamp = timeStampPtr;
    // Yes, I could just use &timeStampPtr in sprintf.. but I like consistency.

    // Annoyingly. The LBARD timestamp has a . instead of a : in the timestamp..
    char* dot = strstr(timeStampPtr, ".");
    *dot = ':';
    timeStamp = timeStampPtr;
    // Alters the original string so no need to return
}

/**
 *  Converts a LBARD timestamp (T+[x]ms) to a long of milliseconds since
 *  00:00:00.000 that day.
 *  NOTE: May break around midnight, this is untested 
 */
long timestamp_to_long(const char timeStamp[20]){
    // Why do we do it like this and not just change LBARD?
    /// Well, hypothetical question, this is because we are going to be comparing these logs
    // against existing topologies and we don't want to have to update LBARD on them too!
    
    //char timeStamp[20] = line;
    //sscanf(line, "%s", &timeStamp);
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

/**
 *  Utility function. Simply returns the single character identifier (A/B/C/etc.) of a given node based
 *  off of its SID.
 */
char get_node_from_sid(char sid[]){
    for (int i = 0; i < numSidArray; i++){
        if (strstr(sidArray[i], sid)){
            return i+65;
        }
    }
    return '-';
}

/**
 *  Utility function. Allows us to easily add lines to the minor event list
 */
void add_to_minor_events(const char line[]){
    strcpy(globalMinorEventArray[numGlobalMinorEvents], line);
    numGlobalMinorEvents += 1;
}

/**
 *  Serves as a comparator for Event objets. Simply compares their timestamps so they are sorted
 *  in chronological order
 */
int eventSort(const void* a, const void* b){
    const struct Events *elem1 = a;    
    const struct Events *elem2 = b;
    char timeStampA[20] = "";
    char timeStampB[20] = "";

    strcpy(timeStampA, elem1->majorTime);
    strcpy(timeStampB, elem2->majorTime);
    long timeStampALong = timestamp_to_long(timeStampA);
    long timeStampBLong = timestamp_to_long(timeStampB);

    if(timeStampALong < timeStampBLong){
        return -1;
    }else if (timeStampALong > timeStampBLong){
        return 1;
    }else{
        return 0;
    }
        
}

void writeLine(char line[], FILE* logFile){
    // TODO: Just call the global variable instead of passing it every time, unnecessary
    fputs(line, logFile);
}