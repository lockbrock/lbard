#include <stdio.h>
#include <stdlib.h>
#include <regex.h>     
#include <pcre.h>
#include <string.h>

/*
    TODO:
    #- Get all the SID and instance letter(number) of each node
    - Get LBARD start time for T+ lines
    - Detect when Fakeradio logs
    #- Detect when ServalDna logs
    - Set writeline to use global variable file
    - Move fakeradio rules in to SETUP
    - Sort by timestamp but ignore setup section
    -   https://stackoverflow.com/questions/14562423/is-there-a-way-to-ignore-header-lines-in-a-unix-sort
*/

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
// TO BE SET WHEN PROCESSING - SO WE KNOW WHAT LOG/INSTANCE WE'RE ON
char instanceChar = '-';
char process[20] = "";
FILE* outputFile;
FILE* fptr;


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
    char filePrefix[64] = "";

    while(fgets(buffer, bufferLength, fptr)) {
    // If line matches one of our important lines then send it to a function to pretty
    // the output and write it to the simple log file.
        char msg[100] = "";
        char sid[64] = "";
        char instance[8] = "";
        char tmp[64] = ""; 
        int res = isLineRelevant(buffer);

        if(res != NULL){
            // TODO: Move each of these to a function to pretty the output & then write
            switch(res){

                case PROCESS:;
                // Process line
                    // Then the beginning of a process so don't print it
                    sprintf(msg, "File: %s:%c\n", process, instanceChar);
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
setProcess(const char proc[]){
    //printf("PROCESS: %s\n", proc);
    if (strcmp(process, "NONE") == 0){
        writeLine("====== BEGIN SETUP ======\n", outputFile);
    }else if (!strcmp(process, "SETUP")) {
        writeLine("======= END SETUP =======\n", outputFile);
    }
    strcpy(process, proc);
}

isLineRelevant(const char line[]){
    // If the line is one of our important/relevant lines, return 1

    char v[100] = "";
    char tmp[1000] = "";
    char instance[100] = "";
    

    // Parsing start of a log file
    if (strstr(line, "++++++++++") != NULL){
        instanceChar = '-';
        //printf("The substring is: %s\n", ret);

        sscanf(line, "%s %s %s", &tmp, &tmp, &v);

        // If LBARD
        if (strstr(v, "lbard") != NULL){
            // TODO: Get the instance number from here
                
            //printf("We have a LBARD line! %s \n", line);
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
    
    if (strstr(line, "mdprequests") && !(strstr(line, "debug.mdprequests"))){
        // TODO: Add more filters for mdprequest log lines

        if (strstr(line, "send_frame")){
            // This just shows packets, NOT when a bundle is sent. How do we find this?
            return RHIZOME_SEND_PACKET;
        }
        //return -1;
        // Send 
    }

    if (strstr(line, "sync_keys") && !(strstr(line, "debug.rhizome_sync_keys"))){
        return RHIZOME_RECEIVE_PACKET;
    }
    if (strstr(line, "ADD MANIFEST")){
        return RHIZOME_ADD_MANIFEST;
    }


    // Fakeradio Rules
    if (strstr(line, "RULE:")){
        return FAKERADIO_RULES;
    }

    // GET SID of each node
    if (strstr(line, "SID") && !(strstr(line, "My"))){
        return NODE_INFOMATION;
    }

    return 0;
}
print_rhizome_packets(char line[]){
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

print_rhizome_add_manifest(char line[]){
    //INFO: [287944] 13:10:19.844 [httpd/4] rhizome_database.c:1501:rhizome_add_manifest_to_store()  RHIZOME ADD MANIFEST service=file bid=3889B11DAFB5C6814EFB77EA2D4BE65F059D5006F08933D3C37CA1AD764D5FAA version=1595389191543
    char msg[1000] = "";
    char timeStamp[16];
    char tmp[100];
    char details[500];
    sscanf(line, "%s %s %s %s %s %[^\t\n]", &tmp, &tmp, &timeStamp, &tmp, &tmp, &details);
    sprintf(msg, "%s SERVALD:%c %s\n", timeStamp, instanceChar, details);
    writeLine(msg, outputFile);
}


print_node_info(char line[]){
    char instance[5] = "";
    char sid[100] = "";
    char msg[100] = "";
    char tmp[100] = "";

    if (strcmp(process, "NONE") == 0){
    setProcess("SETUP");
    } 
    
    // TODO: Extract SID infomation from line here
    sscanf(line, "%s %[^=]=%s", &tmp, &instance, &sid);
    if (strlen(instance) == 4){
        sprintf(msg, "%s : %s\n", instance, sid);
        writeLine(msg, outputFile);
    }
}


writeLine(char line[], FILE* logFile){
    // TODO: Just call the global variable instead of passing it every time, unnecessary
    fputs(line, logFile);
}