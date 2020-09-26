/*
    Written as part of a 2020 Honours thesis by Lachlan Brock for
    a Bachelor of Software Engineering(Honours).
    For more information, see: https://github.com/lockbrock/flinders-thesis 

    Creates a simple log file from a given test file. 
    Optionally, create DOT/Image/PDF files of network traffic during test
*/
#include "topologyDiagrams.h"

enum OutputFormat{
    OUT_NONE = 0,
    OUT_ASCII = 1,
    OUT_ASCII_MAJOR = 2,
    OUT_DOT = 3,
    OUT_IMAGES = 4,
    OUT_TEX = 5,
    OUT_PDF = 6,
};
int outputType = 0;
int cleanOldFiles = 1;  // Remove all files that are used to build the output format by default
int isEmulation = 0; // Controls if we use fakeradio or LBARD major events, set by detection of fakeradio
char outputFolder[150] = "";
struct TestDetails testDetails;

int main(int argc, char **argv){
    // =============== Process arguments
    char inputFilename[150];
    char outputFilename[150] = "";

    if (argc < 3){
        fprintf(stderr, "Invalid number of arguments.\n");
        usage();
        exit(-1);
    }else if (argc >= 4){
        if (strcmp(argv[3], "dot") == 0){
            outputType = OUT_DOT;
        }else if (strcmp(argv[3], "ascii") == 0){
            outputType = OUT_ASCII;
        }else if (strcmp(argv[3], "asciiMajor") == 0){
            outputType = OUT_ASCII_MAJOR;
        }else if (strcmp(argv[3], "images") == 0){
            outputType = OUT_IMAGES;
        }else if (strcmp(argv[3], "tex") == 0){
            outputType = OUT_TEX;
        }else if (strcmp(argv[3], "pdf") == 0){
            outputType = OUT_PDF;
        }else{
            fprintf(stderr, "Invalid option: %s\n", argv[3]);
            usage();
            exit(-1);
        }
        if (argc >= 5){
            if (strcmp(argv[4], "noclean") == 0){
                cleanOldFiles = 0;
            }else{
                usage();
                exit(-1);
            }
        }
    }

    strcpy(inputFilename, argv[1]);
    strcpy(outputFolder, argv[2]);
    sprintf(outputFilename, "%s/simpleLog.txt", outputFolder);    


    if ((fptr = fopen(inputFilename,"r")) == NULL){
        fprintf(stdout, "Error opening log file: %s\n", inputFilename);
        // Program exits if the file pointer returns NULL.
        exit(1);
    }else{
        fprintf(stdout, "\nInput file: %s\n", inputFilename);
    }

    if ((outputFile = fopen(outputFilename,"w")) == NULL){
        fprintf(stdout, "Error creating/opening output file: %s\n", outputFilename);
        // Program exits if the file pointer returns NULL.
        usage();
        exit(1);
    }else{
        fprintf(stdout, "Output file: %s\n\n", outputFilename);
    }
    if (outputType >= OUT_DOT){
        fprintf(stdout, "Creating a simple log file and dot files\n\n");
    }

    setProcess("NONE");
    // TODO: Move this to a new function

    // =============== Process input log file
    int bufferLength = 500;
    char buffer[bufferLength];


    while(fgets(buffer, bufferLength, fptr)) {
    // If line matches one of our important lines then send it to a function to pretty
    // the output and write it to the simple log file.
        char msg[100] = "";
        int res = isLineRelevant(buffer);
        if(res){
            switch(res){
                case PROCESS:
                    // Do nothing, doesn't need to be printed.
                    break;
                case RHIZOME_SEND_PACKET:
                    writeRhizomeSendPacket(buffer);
                    break;
                case RHIZOME_RECEIVE_PACKET:
                    writeRhizomePacket(buffer);
                    break;
                case RHIZOME_ADD_MANIFEST:
                    writeRhizomeAddManifest(buffer);
                    break;
                case NODE_INFOMATION:
                    writeNodeInfo(buffer);
                    break;
                case LBARD_RECEIVE_PACKET:
                case LBARD_NEW_BUNDLE:
                    writeLBARDBundle(buffer);
                    break;
                case LBARD_SEND_PACKET:
                    writeLBARDSend(buffer);
                    break;
                case FAKERADIO_RULES:
                    // So that once sorted fakeradio rules is up the top in the setup section
                    sprintf(msg, "#%03d %s", setupLineNumber, buffer);
                    setupLineNumber += 1;
                    if (strstr(msg, "allow between")){
                        int nodeA = 0;
                        int nodeB = 0;
                        sscanf(buffer, "RULE: 'allow between %d,%d", &nodeA, &nodeB);
                        //printf("nodeA: %c, nodeB: %c\n", nodeA+65, nodeB+65);
                        struct FakeradioLink fl;
                        fl.node1 = nodeA+65;
                        fl.node2 = nodeB+65;
                        allFakeradioLinks[numFakeradioLinks] = fl;
                        numFakeradioLinks += 1;
                    }
                    writeLineToLog(msg);
                    break;
                case FAKERADIO_BUNDLE:
                    writeFakeradioBundle(buffer);
                    break;
                case RHIZOME_LAYOUT:
                    writeRhizomeLayout(buffer);
                    break;
                case LBARD_SYNC_MESSAGE:
                    writeLBARDSync(buffer);
                    break;
                case -1:
                    printf("Error processing line: %s", buffer);
                    break;
                default:
                    printf("Did not find a match for line: %s", buffer);
                    writeLineToLog(buffer);
            }
        }
    }
    fclose(fptr);
    fclose(outputFile);
    // Sort the file in-place
    char sortCommand[500] = "";
    // Sort the file in-place, AND make sure the output is unique
    sprintf(sortCommand, "sort %s -u -o %s", outputFilename, outputFilename);
    system(sortCommand);


    if (outputType >= OUT_ASCII){
        printf("Finished reading input file\n");
        createDotFile();
    }

    return 0;
}
/**
 *  Provides an error message and exits the program.
 *  Called when malformed arguments are passed to the program
 */
void usage(){
    char *msg = "\n\
Syntax: topologyDiagrams <input file> <output folder> [createDot|createImage|createPDF] [noclean]\n\n\
    input file:     path to the log file to be simplified \n\
    output folder:  the folder where all outputs will be saved.\n\n\
    ascii:          optional: create a pipeable ASCII representation of the traffic\n\
    asciiMajor:     optional: create a pipeable ASCII representation of the traffic but only\n\
                    outputting the major events\n\
    dot:            optional: specifies if dot files are to be made\n\
    images:         optional: specifies if images are to be made\n\
    tex:            optional: creates an output LaTex file but does NOT render it and never deletes images\n\
    pdf:            optional: specifies if a PDF is to be made\n\n\
    noclean:        optional: don't delete files. ie. if rendering images\n\
                    then don't delete the DOT files";
    fprintf(stderr, "%s\n", msg);
}

/**
 *  The method used when creating dot files.
 *  Writes n DOT files where n is the number of major events.
 *  Creates an UNDIRECTED graph of the topology, with a DIRECTED edge between two nodes
 *  from the sending node to the receiving node
 */
void createDotFile(){
    char startBuffer[1000] = "";

    if (outputType >= OUT_IMAGES){
        if (system("neato -V &> /dev/null") != 0){
            printf("\nError:    neato does not appear to be installed.\n          The program can not render .DOT files without this.\n");
            exit(-1);
        }
        if (outputType >= OUT_PDF){
            if (system("pdflatex -v &> /dev/null") != 0){
                printf("\nError:    pdflatex does not appear to be installed.\n          The program can not create PDF files without this.\n");
                exit(-1);
            }
        }
    }
    // ============================== SORT EVENTS
    // Current event index /should/ be at the end
    listEventsLength = currentEventIndex;
    // Sort the major event array
    qsort(listEvents, listEventsLength, sizeof(struct Events), eventSort);

    listEventsLength = currentEventIndex;
    qsort(globalMinorEventArray, numGlobalMinorEvents, sizeof(struct MinorEvents), minorEventSort);

    int minorEventIndex = 0;
    // All of this is just linking the (now sorted) minor events
    // to the relevant major event
    for (int i = 0; i < listEventsLength; i++){
        struct Events ev = listEvents[i];
        char tmpTimeMajor[20] = "";
        strcpy(tmpTimeMajor, ev.majorTime);
        long timeStampMajor = timestampToLong(tmpTimeMajor);

        int majorBeforeMinor = 1;
        while(majorBeforeMinor && (minorEventIndex < numGlobalMinorEvents)){
            // TODO: Add a max number of events before moving to a NEW/next major event
            struct MinorEvents minorEvent = globalMinorEventArray[minorEventIndex];

            // Strtok changes the variables passed. So just copy the content to a new var
            // Seems a little hacky but it works?
            char tmpTimeMinor[20] = "";
            strcpy(tmpTimeMinor, minorEvent.majorTime);
            long timeStampMinor = timestampToLong(tmpTimeMinor);

            if (timeStampMinor > timeStampMajor){
                majorBeforeMinor = 0;
                //printf("moving to next major\n");
            }else{
                strcpy(ev.minorEventList[ev.numMinorEvents].majorTime, minorEvent.majorTime);
                strcpy(ev.minorEventList[ev.numMinorEvents].message, minorEvent.message);
                //printf("%s | %s\n", ev.minorEventList[ev.numMinorEvents].majorTime, ev.minorEventList[ev.numMinorEvents].message);
                ev.numMinorEvents += 1;
                minorEventIndex++;
            }
        }
        // Now replace it back in the array
        listEvents[i] = ev;
    }

    if (outputType == OUT_ASCII || outputType == OUT_ASCII_MAJOR){
        for(int i = 0; i < listEventsLength; i++){
            struct Events ev = listEvents[i];
            // If broadcast, do something different
            if(ev.eventType == 'S'){
                printf("\n%d: [ %s] %s\n     %c -> BROADCAST\n", (i+1), ev.majorTime, ev.transferDetails, 
                ev.sendingNode);
            }else{
                printf("\n%d: [ %s] %s\n     %c -> %c\n", (i+1), ev.majorTime, ev.transferDetails, 
                ev.sendingNode, ev.destinationNode);
            }
            if (outputType == OUT_ASCII){
                // Print minor events only if not requested for major events only
                for(int j = 0; j< ev.numMinorEvents; j++){
                    struct MinorEvents minor = ev.minorEventList[j];
                    printf("- %s : %s\n", minor.majorTime, minor.message); 
                }
            }

        }
        return;
    }

    // =============================== CREATE DOT FILE
    // I'm keeping it all nicely formatted when it's produced so this bit is a little ugly
    const char *startText =  "\
// Auto-generated by topologyDiagrams.c \n\
strict digraph A {\n\
    ratio=fill\n\
    mode=KK\n\
    pad=0.5\n\
    mindist=0.5\n";
    sprintf(startBuffer, startText);

    char nodesBuffer[1000];
    sprintf(nodesBuffer, "\
    subgraph Nodes {\n\
      node [pin=true, shape=circle]\n");
    for(int i = 0; i < numSidArray; i++){
        sprintf(nodesBuffer, "%s      %c\n", nodesBuffer, i+65);
    }
    const char *endNodes = "\
    }\n";
    sprintf(nodesBuffer, "%s%s", nodesBuffer, endNodes);


    char filename[200] = "";
    int numFile = 1;
    char transferBuffer[500] = "";
    // =============== Create dot files ===============
    printf("\n");
    for (int i = 0; i < listEventsLength; i++){
        sprintf(filename, "%sdotfile%03d.dot", outputFolder, numFile);

        if ((outputFile = fopen(filename, "w")) == NULL){
            printf("Error creating/opening output file: %s\n", filename);
            exit(1);
        }else{

            struct Events ev = listEvents[i];

            // =============== Definition and such ===============  
            fputs(startBuffer, outputFile);
    
            char nodeStartLabel[1000] = "";
            sprintf(nodeStartLabel, "\
    label=\"%s\"\n\
    labeljust=n\n\
    labelloc=t\n\
    nodesep=1\n", ev.transferDetails);

            fputs(nodeStartLabel, outputFile);
            fputs(nodesBuffer, outputFile);

            // =============== Layout ===============  
            // Put the initial bit: digraph, and nodes section

            char endBuffer[1000] = "";
            const char *layoutText = "\
    subgraph Layout {\n\
      edge [dir=none, penwidth=3, len=1 style=dashed]\n";
            sprintf(endBuffer, "%s%s", endBuffer, layoutText);


            // =============== Transfer ===============  
            // Add the 'Transfer' section of the .DOT file
            if (ev.eventType == 'W'){
            // WIFI
                sprintf(transferBuffer, "\
    subgraph Transfer {\n\
      edge [penwidth=3, len=1, style=bold]\n\
      %c -> %c [color=\"blue\"]\n\
    }\n", ev.sendingNode, ev.destinationNode, ev.transferDetails);                
            
            }else if (ev.eventType == 'F'){
            // Fakeradio/LBARD
                sprintf(transferBuffer, "\
    subgraph Transfer {\n\
      edge [penwidth=3]\n\
      %c -> %c [color=red]\n\
    }\n", ev.sendingNode, ev.destinationNode, ev.transferDetails);

            }else if (ev.eventType == 'S'){
                // TODO: HANDLE THIS FOR LBARD (NO FAKERADIO)
                // We need to find out what nodes a certain node is a neighbour of..

                // Need to get all nodes that this node talks to
                // We can assume it's just LBARD broadcasts at the moment
                char lbardNeighbours[26] = "";
                int numNeighbours = 0;
                
                sprintf(transferBuffer, "\
    subgraph Transfer {\n\
      edge [penwidth=3]\n", ev.sendingNode, ev.destinationNode, ev.transferDetails);

                for(int i = 0; i < numFakeradioLinks; i ++){
                    if (allFakeradioLinks[i].node1 == ev.sendingNode){
                        lbardNeighbours[numNeighbours] = allFakeradioLinks[i].node2;
                        numNeighbours += 1;
                    }else if(allFakeradioLinks[i].node2 == ev.sendingNode){
                        lbardNeighbours[numNeighbours] = allFakeradioLinks[i].node1;
                        numNeighbours += 1;
                    }
                }
                //printf("\nNode %c has (%d) neighbour(s): %s\n", ev.sendingNode, numNeighbours,lbardNeighbours);
                // Now, for each neighbour add a link
                for (int i = 0; i < numNeighbours; i++){
                    char msg[48] = "";
                    sprintf(msg, "      %c -> %c [color=red]\n", ev.sendingNode, lbardNeighbours[i]);
                    sprintf(transferBuffer, "%s%s", transferBuffer, msg);
                }
                // Add the final end brace
                sprintf(transferBuffer, "%s    }\n", transferBuffer);
            }else{
                fprintf(stderr, "Invalid transfer type (%c) for event: %s\n", ev.eventType, ev.transferDetails);
                exit(1); 
            }

            // =============== Fakeradio Layout ===============  
            // FAKERADIO
            // TODO: Add to struct the type of fakeradio link so we can colour differently
            // ie. red for RFD900, green for Codan, etc.
            for (int i = 0; i < numFakeradioLinks; i++){
                // This just checks that we're not re-printing the same link. Otherwise, we'd have double links when there is a transfer AND a FR/Wifi link
                if (ev.eventType == 'S'){
                    // Don't show a link IF it's a broadcast between the relevant nodes
                    if (!(allFakeradioLinks[i].node1 == ev.sendingNode || allFakeradioLinks[i].node2 == ev.sendingNode)){
                        char msg[48] = "";
                        sprintf(msg, "      %c -> %c [color=red]\n", allFakeradioLinks[i].node1, allFakeradioLinks[i].node2);
                        sprintf(endBuffer, "%s%s", endBuffer, msg);
                    }

                } else if (!(((allFakeradioLinks[i].node1 == ev.sendingNode) && (allFakeradioLinks[i].node2 == ev.destinationNode)) ||
                             ((allFakeradioLinks[i].node2 == ev.sendingNode) && (allFakeradioLinks[i].node1 == ev.destinationNode)))){
                        char msg[48] = "";
                        sprintf(msg, "      %c -> %c [color=red]\n", allFakeradioLinks[i].node1, allFakeradioLinks[i].node2);
                        sprintf(endBuffer, "%s%s", endBuffer, msg);
                }
            }

            // =============== Wifi Layout ===============  
            // For each interface, add the link
            /**
             *  This bit looks bad but let's explain.
             *  Firstly, we loop through every single WiFi interface.
             *  Then, we create ALL possible combinations (not permutations) of two nodes in this interface.
             *  We do this because any one link can only exist between two nodes, however there
             *  may be any number of unique combinations between nodes in an interface.
             *  Then, we write this link to the file IFF it is not also the same link as was already printed
             *  as a DIRECTED transfer for a major event.
             *  This prevents double links between nodes.
             */
            for(int interface = 0; interface < MAX_LINKS; interface++){
                char listNodes[26] = "";
                strcpy(listNodes, RhizomeInterfaces[interface]);

                if (strlen(listNodes) > 1){
                    for (int i = 0; i < strlen(listNodes) -1; i++){
                        char charA = listNodes[i];
                        for (int j = i +1; j < strlen(listNodes); j++){
                            char charB = listNodes[j];
                            // This just checks that we're not re-printing the same link. Otherwise, we'd have double links when there is a transfer AND a FR/Wifi link
                            if (!(((charA == ev.sendingNode) && (charB == ev.destinationNode)) || ((charB == ev.sendingNode) && (charA == ev.destinationNode)))){
                                char msg[48] = "";
                                sprintf(msg, "      %c -> %c [color=blue]\n", charA, charB);
                                sprintf(endBuffer, "%s%s", endBuffer, msg);
                            }
                        }
                    }
                }
            }
            // =============== End of the file ===============  
            const char *endLayout = "\
    }\n";
            sprintf(endBuffer, "%s%s", endBuffer, endLayout);

            const char *endBrace = "}";
            sprintf(endBuffer, "%s%s", endBuffer, endBrace);

            // =============== Write to file ===============  
            fputs(transferBuffer, outputFile);
            fputs(endBuffer, outputFile);
            fclose(outputFile);

            // =============== Render image ===============
            char outputImageName[250] = "";
            sprintf(outputImageName, "%soutput%03d.png", outputFolder, numFile);
            if (outputType >= OUT_IMAGES){
                // printf("attempting to make image: %d\n", numFile);
                char renderCommand[250] = "";
                sprintf(renderCommand, "neato -Tpng %s -o %s", filename, outputImageName);
                //sprintf(renderCommand, "neato -Tpng %s -o %s", filename, outputImageName);
                system(renderCommand);
                if (cleanOldFiles){
                    // Delete the DOT files if they're not wanted
                    remove(filename);        
                }
            }

            // =============== Add to LaTex template ===============
            if (outputType >= OUT_TEX){
                // LaTeX needs a directory in the import (ie. outputFolder) AND a file name (ie. output001.png)
                // annoyingly, this means we can't just reuse outputImageName..
                char shortImageName[50] = "";
                sprintf(shortImageName, "output%03d.png", numFile);

                if (i == listEventsLength -1){
                    // If last one, pass a 1 to add the 'end{document} bit'
                    createLATEXFile(ev, shortImageName, 1);
                }else{
                    createLATEXFile(ev, shortImageName, 0);
                }
            }
            if (outputType >= OUT_IMAGES){
                printf("\rCreated %3d of %d images", numFile, listEventsLength);
            }else{
                printf("\rCreated %3d of %d DOT files", numFile, listEventsLength);
            }
            fflush(stdout);
            numFile++;
        }
    }
    // New line since we were doing our progress bar on just the one line
    printf("\n");


    if (outputType >= OUT_PDF){
        // Render latex file here
        char renderPDF[200];
        printf("Rendering LaTeX PDF\n");
        sprintf(renderPDF, "pdflatex --output-directory=%s %sdiagrams.tex &>/dev/null", outputFolder, outputFolder);
        system(renderPDF);
        printf("Finished rendering LaTeX PDF\n");

        // Do NOT clean images if just creating a TeX file, as then it can't compile
        if(cleanOldFiles){
            // Remove all excess images after rendering
            char imageFilename[250] = "";
            for (int i = 0; i <= listEventsLength; i++){
                sprintf(imageFilename, "%soutput%03d.png", outputFolder, i);
                remove(imageFilename);
            }
            char cleanLatex[200] = "";
            sprintf(cleanLatex, "rm  %sdiagrams.tex %sdiagrams.aux %sdiagrams.log", outputFolder, outputFolder, outputFolder);
            system(cleanLatex);
        } 
    }
    
}

/**
 *  Write the LaTeX file for each of the events.
 *  Creates one portrait A4 page per event. 
 *  Layout:
 *  |---------------|
 *  |Test details   |
 *  |Event time     |
 *  |        Diagram|
 *  |Each log line  |
 *  |---------------|
 *  Diagram is right aligned so that any text (which will extend the page to the left)
 *  /should/ not skew the diagram - meaning that the resultant diagrams /should/ be all in the same spot
 *  Frankly, this is a little hacky, however I have no clue how to ensure that the diagram shape doesn't change
 *  when the text length changes for a transfer
 * 
 *  Should be self explanatory. Just formatting strings then writing them to the file
 */
int firstLine = 1;
void createLATEXFile(const struct Events ev, const char *outputImageName, const int isLastEvent){
    char latexFilename[250] = "";
    sprintf(latexFilename, "%sdiagrams.tex", outputFolder);


    // ============== Open file, setup/import section
    if (firstLine){
        if ((latexFile = fopen(latexFilename, "w")) == NULL){
            printf("Error creating/opening output file: %s\n", latexFilename);
            exit(-1);
        }else{
            char importMsg[500] = "";
            sprintf(importMsg, "\
\\documentclass[12pt]{report}\n\
\\usepackage[margin=0.5in]{geometry}\n\
\\usepackage{wrapfig}\n\
\\usepackage{graphicx}\n\
\\usepackage{blindtext}\n\
\\graphicspath{ {%s} }\n\
\\parindent0pt\n\
\\begin{document}\n", outputFolder);
            fputs(importMsg, latexFile);
        }
        firstLine = 0;
    }
    
    // ============== Test details
    char testDetailsText[500] = "";
    sprintf(testDetailsText, "\
\\begin{flushleft}\n\
    {\\large\n\
    Name:       %s\\linebreak\n\
    Result:     %s\\linebreak\n\
    Started:    %s\\linebreak\n\
    Finished:   %s\\linebreak}\n\n\
    {\\huge \n\
    Time:       %s}\n\
\\end{flushleft}\n", testDetails.testName, testDetails.result, testDetails.timeStarted, testDetails.timeFinished, ev.majorTime);
    fputs(testDetailsText, latexFile);
    
    // ============== Adding Image
    char imageText[500] = "";
    sprintf(imageText, "\
\\hrulefill \\linebreak\n\
\\begin{centering} \n\
    \\includegraphics[height=10cm, keepaspectratio]{%s}\\\\\n\
\\end{centering}\n\
\\raggedright\n\
\\hrulefill \\linebreak\n\n", outputImageName);
    fputs(imageText, latexFile);

    // ============== Adding Minor Events
    //printf("%s has %d minor events\n", ev.transferDetails, ev.numMinorEvents);
    for (int i = 0; i < ev.numMinorEvents; i++){
        char minorEventText[500] = "";
        struct MinorEvents minorEvent = ev.minorEventList[i];
        sprintf(minorEventText, "\\textbf{%s} %s\\linebreak\n", minorEvent.majorTime, minorEvent.message);
        fputs(minorEventText, latexFile);
    }

    if (isLastEvent){
        // Add the \end document stuff and close the file
        const char *endDocumentText = "\\end{document}";
        fputs(endDocumentText, latexFile);
        fclose(latexFile);
    }else{
        const char *newPageText = "\n\n\\newpage\n";
        fputs(newPageText, latexFile);
    }
}

/**
 *  Set the process variable. Do it in this function so that we can do 
 *  specific things on certain process changes (ie. begin/end setup)
 */
void setProcess(const char proc[]){
    char buffer[50] = "";
    if (strcmp(process, "NONE") == 0){
        if (strcmp(proc, "SETUP") != 0){
            // Is currently none, and NOT going into a setup, end it
            sprintf(buffer, "#%03d ======= END SETUP =======\n", 100);
            writeLineToLog(buffer);    
        }

        // Should ALWAYS be the first line
        sprintf(buffer, "#%03d ====== BEGIN SETUP ======\n", 0);
        setupLineNumber += 1;
        writeLineToLog(buffer);
    }else if (strcmp(process, "SETUP") == 0) {
        // We're in SETUP but we're being switching away from it

        // Ideally we don't have more than 99 lines in the setup. If so, just increase the 100
        sprintf(buffer, "#%03d ======= END SETUP =======\n", 100);
        writeLineToLog(buffer);
    }
    strcpy(process, proc);
}




/**
 *  Checks if the line sent to it is a line relevant to the simple log file
 *  Simply checks if it contains specific key words (it's good enough)
 *  Returns an enum value signifying which type of line it is
 *  May in some cases change the process value depending on the line.
 */
int isLineRelevant(char line[]){
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
            isEmulation = 1;
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
        if (strstr(line, "Name:")){
            char tmpName[100] = "";
            sscanf(line, "Name: %[^\n\t]", tmpName);
            if (strstr(tmpName, "_") || strstr(tmpName, "\\")){
                for (int i = 0; i < strlen(tmpName); i++){
                    if (tmpName[i] == '_'){
                        tmpName[i] = '-';
                    }else if (tmpName[i] == '\\'){
                        tmpName[i] = '/';
                    }
                }
            }
            strcpy(testDetails.testName, tmpName);
            setupLineNumber += 1;
            sprintf(tmp, "#%03d %s", setupLineNumber, line);
            writeLineToLog(tmp);
        } else if (strstr(line, "Result:")){
            char tmpResult[10] = "";
            sscanf(line, "Result: %[^\n\t]", tmpResult);
            strcpy(testDetails.result, tmpResult);
            setupLineNumber += 1;
            sprintf(tmp, "#%03d %s", setupLineNumber, line);
            writeLineToLog(tmp);
        }else if (strstr(line, "Started:")){
            char tmpStart[25] = "";
            sscanf(line, "Started: %[^\n\t]", tmpStart);
            strcpy(testDetails.timeStarted, tmpStart);
            setupLineNumber += 1;
            sprintf(tmp, "#%03d %s", setupLineNumber, line);
            writeLineToLog(tmp);
        }else if (strstr(line, "Finished:")){
            char tmpFinish[25] = "";
            sscanf(line, "Finished: %[^\n\t]", tmpFinish);
            strcpy(testDetails.timeFinished, tmpFinish);
            setupLineNumber += 1;
            sprintf(tmp, "#%03d %s", setupLineNumber, line);
            writeLineToLog(tmp);
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
        sscanf(instance, "var/servald/instance/%s", &instance);
        // Get the last character of the string 'LBARDX'
        if (strlen(instance) > 1){
            instanceChar = instance[0];
            return PROCESS;
        }
        return -1;
    }
    //DEBUG:[143596] 14:47:38.192 rhizome_sync_keys.c:541:sync_peer_does_not_have()  {rhizome_sync_keys} Neighbour F4FB12D2083C49F62D753CEB5ACA846024AA2CF4CF8E09FAB48E7F9699F30C4D does not have 951F4302854BFB92 that we do
    //DEBUG:[143596] 14:47:38.248 rhizome_sync_keys.c:560:sync_peer_now_has()  {rhizome_sync_keys} Neighbour F4FB12D2083C49F62D753CEB5ACA846024AA2CF4CF8E09FAB48E7F9699F30C4D has now received 951F4302854BFB92
    //DEBUG:[143749] 14:47:38.169 rhizome_sync_keys.c:528:sync_peer_has()  {rhizome_sync_keys} Neighbour BD149D5799D768E8CD72FF797611BF3C154C92F961D2D1E0662E02DFBBDB6573 has 951F4302854BFB92 that we need

    if (strcmp(process, "SERVALD") == 0){
        if (strstr(line, "mdprequests") && !(strstr(line, "debug.mdprequests"))){
            if (strstr(line, "send_frame")){
                return RHIZOME_SEND_PACKET;
            }
        }
        if (strstr(line, "sync_keys") && !(strstr(line, "debug.rhizome_sync_keys"))){
            if (strstr(line, "Queued transfer message")){
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
            lbardInitialTime = timestampToLong(line);
        }
        if(strstr(line, "HAS some bundle")){
            return LBARD_RECEIVE_PACKET;
        }
        if (strstr(line, "We have the entire bundle")){
            return LBARD_RECEIVE_PACKET;
        }
        if (strstr(line, "Progress receiving")){
            return LBARD_RECEIVE_PACKET;
        }
        if (strstr(line, "We have new bundle")){
            return LBARD_NEW_BUNDLE;
        }
        if (strstr(line, "Sending length")){
            return LBARD_SEND_PACKET;
        }
        if (strstr(line, "has finished receiving")){
            return LBARD_SEND_PACKET;
        }
        if (strstr(line, "Resending bundle")){
            return LBARD_SEND_PACKET;
        }
        if (strstr(line, "I just sent")){
            return LBARD_NEW_BUNDLE;
        }
        if (strstr(line, "Calling message handler")){
            return LBARD_NEW_BUNDLE;
        }
        if (strstr(line, "Registering peer")){
            return LBARD_NEW_BUNDLE;
        }
        if (strstr(line, "Decoding message")){
            return LBARD_NEW_BUNDLE;
        }
        if (strstr(line, "Sending sync message")){
            return LBARD_SYNC_MESSAGE;
        }
        if (strstr(line, "now has bundle")){
            return LBARD_SYNC_MESSAGE;
        }
        if (strstr(line, "is missing bundle")){
            return LBARD_SYNC_MESSAGE;
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
    if (strcmp(process, "NONE") == 0 || strcmp(process, "SETUP") == 0){
        if (strstr(line, "SID")){
            return NODE_INFOMATION;
        }
    }
    if (strstr(line, "My SID as hex")){
        return NODE_INFOMATION;
    }
    return 0;
}


// Store the SID of the destination in the mdprequests
char rhizomeDestination[65] = "";
char frameMessage[100] = "";
char frameBID[65] = "";
char dotMsg[256] = "";
int readyToSend = 0;
/**
 *  Writes the message to the log file.
 *  Formats the lines in a nice way first though.
 *  NOTE: above global variables are because Send packet lines go over multiple lines so we need
 *  to be able to store them between function calls 
 */
void writeRhizomeSendPacket(char line[]){
    //14:47:38.214 SERVALD:B Sending 951F4302854BFB92 50 bytes (now 50 of 50)
    char msg[1000] = "";
    char timeStamp[20];
    char tmp[100];
    char details[500];
    //printf("uhh? %s\n", line);

    sscanf(line, "%s %s %s %s %[^\t\n]", &tmp, &timeStamp, &tmp, &tmp, &details);
    // format: [timestamp] SERVALD:[X] [details]
    sprintf(msg, "%s SERVALD:%c %s\n", timeStamp, instanceChar, details);
    
    if (strstr(line, "Attempting to queue mdp packet from") && ! strstr(line, "broadcast")){
        // We need to get the destination of a packet. That's all this bit is
        // broadcast ones are to anyone on the network, not super relevant. We're kinda just ignoring it
        // TODO: Add this to a broadcast WIFI event later
        if(outputType >= OUT_ASCII){
            // TODO: Shorten the SIDs to 8 chars and then (Node A)
            char tempMsg[300] = "";
            //DEBUG:[584836] 15:23:17.870 overlay_mdp.c:817:_overlay_send_frame()  {mdprequests} Attempting to queue mdp packet from 8AEF473837C24E56ADD9C864228A0B453BF70161639D851A5FBC150E15977052:18 to 2BEF74429C78AEB1347B6182E8689F9B9644EDF74379D7C0635F5649D85A2F39:18
            //printf("test %s\n", line);
            memset(tempMsg, 0, strlen(tempMsg));
            memset(rhizomeDestination, 0, strlen(rhizomeDestination));
            sscanf(line, "%s %s %s %s %[^\t\n]", &tmp, &tmp, &tmp, &tmp, &tempMsg);
            sscanf(tempMsg, "%s %s %s %s %s %s %s %s %[^:]", &tmp, &tmp, &tmp, &tmp, &tmp, &tmp, &tmp, &tmp, &rhizomeDestination);
            readyToSend = 1;
        }
        writeLineToLog(msg);
    }
    if (strstr(line, "Send frame")){
        if(outputType >= OUT_ASCII && readyToSend && strcmp(frameMessage, "") != 0){
            // We need to get the frame size
            char numBytes[5] = "";
            char timeStamp[20] = "";
            //memset(timeStamp, 0, strlen(timeStamp));

            //DEBUG:[584682] 15:22:58.209 overlay_mdp.c:859:_overlay_send_frame()  {mdprequests} Send frame 9 bytes
            sscanf(line, "%s %s %s %s %s %s %s", &tmp, &timeStamp, &tmp, &tmp, &tmp, &tmp, &numBytes);
            // Now we make this into a major event
            
            //[SEND_MANIFEST] Send frame 464 bytes to 8AEF473837C24E56ADD9C864228A0B453BF70161639D851A5FBC150E15977052
            char sidChar = getNodeFromSid(rhizomeDestination);
            struct Events currentEvent;
            currentEvent.destinationNode = sidChar;
            currentEvent.sendingNode = instanceChar;
            currentEvent.eventType = 'W';
            sprintf(dotMsg, "[%s] [%s bytes]", frameMessage, numBytes);
            strcpy(currentEvent.transferDetails, dotMsg);
            strcpy(currentEvent.majorTime, timeStamp);
            currentEvent.numMinorEvents = 0;


            listEvents[currentEventIndex] = currentEvent;
            currentEventIndex += 1;

            //memset(timeStamp, 0, strlen(timeStamp));
            memset(dotMsg, 0, strlen(dotMsg));
            memset(frameMessage, 0, strlen(frameMessage));
            readyToSend = 0;
        }else if (readyToSend){
            writeLineToLog(msg);
        }else{
            return;
        }
    }
    if (strstr(line, "Queued transfer message")){
        if(outputType >= OUT_ASCII){
            // We're saving all of these so that they're classified as a major event
            char tempTransferMessage[255] = "";
            //DEBUG:[584682] 15:23:17.868 rhizome_sync_keys.c:157:find_and_update_transfer()  {rhizome_sync_keys} Queued transfer message SEND_MANIFEST 15EB502E2D2D9521
            memset(tempTransferMessage, 0, strlen(tempTransferMessage));
            memset(frameMessage, 0, strlen(frameMessage));
            
            sscanf(line, "%s %s %s %s %[^\t\n]", &tmp, &tmp, &tmp, &tmp, &tempTransferMessage);
            sscanf(tempTransferMessage, "%s %s %s %s %[^\t\n]", &tmp, &tmp, &tmp, &frameMessage, &frameBID);
        }
        writeLineToLog(msg);
    }
}

/**
 *  For when Rhizome send/receives a packet
 *  Simply prints the relevant lines in a nicely formatted and consistent 
 *  manner
 */
void writeRhizomePacket(char line[]){
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

    if(outputType >= OUT_ASCII){
        // Filter MUCH more
        if (strstr(msg, "Ignoring added")){
            addToMinorEvents(msg);
            writeLineToLog(msg);
        }else if (strstr(msg, "Sending message")){
            addToMinorEvents(msg);
            writeLineToLog(msg);
        }else if (strstr(msg, "Queueing next message")){
            addToMinorEvents(msg);
            writeLineToLog(msg);
        }else if (strstr(msg, "Send frame")){
            addToMinorEvents(msg);
            writeLineToLog(msg);
        }else if (strstr(msg, "Connection closed")){
            // Format; change SID to 8 char and Node Char, otherwise takes up too much room on the diagram
            // 14:47:44.263 SERVALD:B Connection closed F4FB12D2083C49F62D753CEB5ACA846024AA2CF4CF8E09FAB48E7F9699F30C4D
            char longSID[65] = "";
            char newMsg[1000] = "";
            char process[20] = "";
            sscanf(msg, "%s %s %s %s %s", &timeStamp, &process, &tmp, &tmp, &longSID);
            char shortSID[10] = "";
            char nodeChar = shortenSID(longSID, shortSID);
            sprintf(newMsg, "%s %s Connection closed %s* (Node %c)", timeStamp, process, shortSID, nodeChar);
            addToMinorEvents(newMsg);
            writeLineToLog(newMsg);
        }else if (strstr(msg, "Processing message from")){
            //Do the same as above (but different format)
            //14:47:29.058 SERVALD:B Processing message from F4FB12D2083C49F62D753CEB5ACA846024AA2CF4CF8E09FAB48E7F9699F30C4D
            char longSID[65] = "";
            char newMsg[1000] = "";
            char process[20] = "";
            sscanf(msg, "%s %s %s %s %s %s", &timeStamp, &process, &tmp, &tmp, &tmp, &longSID);
            char shortSID[10] = "";
            char nodeChar = shortenSID(longSID, shortSID);
            sprintf(newMsg, "%s %s Processing message from %s* (Node %c)\n", timeStamp, process, shortSID, nodeChar);
            addToMinorEvents(newMsg);
        }else if (strstr(msg, "Neighbour")){
            writeLineToLog(msg);
            addToMinorEvents(msg);
        }else if (strstr(msg, "Wrote to")){
            writeLineToLog(msg);
            addToMinorEvents(msg);
        }else if (strstr(msg, "Closing idle connection")){
            writeLineToLog(msg);
            addToMinorEvents(msg);
        }else if (strstr(msg, "manifest")){
            writeLineToLog(msg);
            addToMinorEvents(msg);
        }else if (strstr(msg, "Adding") && strstr(msg, "to tree")){
            addToMinorEvents(msg);
            writeLineToLog(msg);
        }else{
            //printf("default: %s\n", msg);
        }
    }else{
        writeLineToLog(msg);
    }
}

/**
 *  For when a bundle is added to Rhizome
 *  Simply prints the relevant lines in a nicely formatted and consistent 
 *  manner
 */
void writeRhizomeAddManifest(char line[]){
    //INFO: [287944] 13:10:19.844 [httpd/4] rhizome_database.c:1501:rhizome_add_manifest_to_store()  RHIZOME ADD MANIFEST service=file bid=3889B11DAFB5C6814EFB77EA2D4BE65F059D5006F08933D3C37CA1AD764D5FAA version=1595389191543
    char msg[1000] = "";
    char timeStamp[16];
    char tmp[100];
    char details[500];
    sscanf(line, "%s %s %s %s %s %[^\t\n]", &tmp, &tmp, &timeStamp, &tmp, &tmp, &details);
    sprintf(msg, "%s SERVALD:%c %s\n", timeStamp, instanceChar, details);
    writeLineToLog(msg);
}

/**
 *  For printing node SIDs
 *  Simply prints the relevant lines in a nicely formatted and consistent 
 *  manner
 *  Saves each SID to an array so we can convert SIDs to node chars easily
 */
void writeNodeInfo(char line[]){
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
    if (strcmp(process, "SETUP") == 0){
        //printf("line: %s", line);
        sscanf(line, "%s %[^=]=%s", &tmp, &instance, &sid);
        if (strlen(instance) == 4){
            // TODO: Make work smarter and use the instance as a number
            //      this means that we can re-assign instead of just appending
            //      important for when we have real world devices which don't have setup
            // Add it to the array so we can compare things to it later
            int instNum = instance[3] - 65;
            strcpy(sidArray[instNum], sid);
            numSidArray = instNum;
            sprintf(msg, "#%03d %s : %s\n", setupLineNumber, instance, sid);
            setupLineNumber += 1;
            writeLineToLog(msg);
        }
    }else if (strstr(line, "My SID as hex")){
        //printf("SID for this is: %s", line);
        //469:My SID as hex is CEE162C5A2A6647B7AE5EDEB9944F177E6815C49D5C7FD88B6087DABDD747571
        sscanf(line, "%s SID as hex is %s", &tmp, &sid);
        int instNum = instanceChar - 65;

        // Check if this SID already exists in the array
        // ie. we detected it in the setup stage (not real world)
        if (strlen(sidArray[instNum]) < 1){
            strcpy(sidArray[instNum], sid);
            sprintf(msg, "#%03d SID%c : %s\n", setupLineNumber, instanceChar, sid);
            setupLineNumber += 1;
            numSidArray = instNum +1;
            writeLineToLog(msg);
        }
        return;
    }else{
        //printf("nah %s", line);
    }

}

/**
 *  For when LBARD receives a bundle OR a bundle is added (ie. via
 *  Rhizome)
 *  Simply prints the relevant lines in a nicely formatted and consistent 
 *  manner
 */
void writeLBARDBundle(char line[]){
    //>>> [13:10.21.821 7408*] Peer 7a43982622b4* HAS some bundle that we don't have (key prefix=48CC*).
    //>>> [18:45.25.432 3F76*] We have the entire bundle 92aefeabda1687ad*/1595582106764 now.
    char msg[1000] = "";
    char timeStamp[20] = "";
    char tmp[100];
    char details[500];
    char t = '=';

    if (!strstr(line, ">>>")){
        //printf("Bad line: %s\n", line);        
        return;
    }

    sscanf(line, "%s %c %s %s %[^\t\n]", &tmp, &t, &timeStamp, &tmp, &details);
    if (strlen(timeStamp) != 0){
        fixLBARDTimestamp(timeStamp);
    }else{
        printf("No timestamp for line: \n", line);
        return;
    }
    char longBID[65] = "";

    if (strstr(line, "We have new bundle")){
        // We have new bundle EB7E8ABC39C548D939C5AB1E0015B7D5389D7C3FFBD1BE44439DC7C0A5F6730C/1596604638358
        sscanf(details, "We have new bundle %[^/]", &longBID);
        char shortBID[10] = "";
        shortenBID(longBID, shortBID);
        sprintf(details, "We have new bundle %s", shortBID);
    }else if (strstr(line, "We have the entire bundle")){
        // We have the entire bundle eb7e8abc39c548d9*/1596604638358 now.
        sscanf(details, "We have the entire bundle %[^/]", &longBID);
        char shortBID[10] = "";
        shortenBID(longBID, shortBID);
        sprintf(details, "We have the entire bundle %s", shortBID);
    }else if (strstr(line, "HAS some bundle")){
        // Peer be69b56b901e* HAS some bundle that we don't have (key prefix=484C*).
        char longSID[66] = "";
        char shortSID[25] = "";
        sscanf(details, "Peer %s", longSID);
        shortenSID(longSID, shortSID);
        char nodeChar = getNodeFromSid(shortSID);
        sprintf(details, "Neighbour Node %c (%s) has some bundles that we don't have", nodeChar, shortSID);

    }else if (strstr(line, "Calling message handler")){
        // TODO: Change the type letters to be more specific
    }else if (strstr(line, "Registering peer")){
        // Registering peer 7400fb28899b*
        char tmpSID[20] = "";
        sscanf(details, "Registering peer %s", tmpSID);
        char nodeChar = getNodeFromSid(tmpSID);
        sprintf(details, "Registering peer %s (Node %c)", tmpSID, nodeChar);
    }
    else if (strstr(line, "Progress receiving")){
        // Progress receiving BID=1f1d0d66ee60aecf* version 1600142462897: manifest is 258 bytes long (RX bitmap fcff), and body 50 bytes long. Bitmap p=    0 : Y
        char tmpBID[66] = "";
        char shortBID[25] = "";
        char tmpVersion[25] = "";
        char manifestLength[25] = "";
        char bodyLength[25] = "";
        sscanf(details, "Progress receiving BID=%s version %s manifest is %s bytes long %[^,] , and body %s",
            &tmpBID, &tmpVersion, &manifestLength, &tmp, &bodyLength);
        shortenBID(tmpBID, shortBID);
        sprintf(details, "Progress receiving %s: Manifest is %s bytes, and body is %s bytes", shortBID, manifestLength, bodyLength);
        // TODO: Shorten BID
    }
    sprintf(msg, "%s LBARD:%c %s\n", timeStamp, instanceChar, details);
    writeLineToLog(msg);

    if(outputType >= OUT_ASCII){
        if (strstr(line, "We have new bundle")){
            addToMinorEvents(msg);
        }else if (strstr(line, "We have the entire bundle")){
            addToMinorEvents(msg);
        }else if (strstr(line, "HAS some bundle")){
            addToMinorEvents(msg);
        }else if (strstr(msg, "I just sent")){
            // ADD TO MAJOR EVENTS
            //16:48:23.416396 LBARD:A I just sent manifest piece [0,128) of DEF9953BA06F72F1E5B15207AFB3F9D62D4A64D073BBEE12C3A84F0FAA22F713* for cee162c5a2a6* (filling 149 of the 192 remaining packet bytes).
            //19:37:27.796695 LBARD:B I just sent body piece [320,384) of 3AC0A0F66043D55EEB595FD42A8E4559F3888A06918F829942F393BBD3ADB301* for af61752c0382* (filling 85 of the 140 remaining packet bytes).
            char msgDest = '-';
            char msgDestSID[66] = "";
            char msgSend = '-';
            char msgDot[256] = "";
            char msgType[50] = "";
            char msgBidLong[65] = "";
            char msgShortBid[16] = "";
            // TODO: Do something more with these byte values
            sscanf(msg, "%s LBARD:%c I just sent %s piece %s of %s for %s %[^\n\t]",
                    &timeStamp, &msgSend, &msgType, &tmp, &msgBidLong, &msgDestSID, &tmp);
            msgDest = getNodeFromSid(msgDestSID);

            if (msgDest == '-'){
                // Sometimes lines get all kinds of mangled from memory issues in LBARD
                // These /tend/ to occur later in the message so the SID gets mangled
                // Simply put, there's nothing we can do about it so just dump them.
                printf("Error processing line: %s", msg);
                return;
            }
            struct Events currentEvent;
            currentEvent.destinationNode = msgDest;
            currentEvent.sendingNode = msgSend;
            currentEvent.eventType = 'F';
            strcpy(currentEvent.majorTime, timeStamp);
            shortenBID(msgBidLong, msgShortBid);
            sprintf(msgDot, "Sent %s piece of %s", msgType, msgShortBid);
            strcpy(currentEvent.transferDetails, msgDot);
            // printf("[%s] %c -> %c | %s\n", timeStamp, msgSend, msgDest, currentEvent.transferDetails);
            currentEvent.numMinorEvents = 0;
            listEvents[currentEventIndex] = currentEvent;
            currentEventIndex += 1;
            addToMinorEvents(msg);
        }else{
            // printf("default: %s", msg);
            addToMinorEvents(msg);
        }
    }
}


/**
 *  For when LBARD sends/resends a bundle
 *  Simply prints the relevant lines in a nicely formatted and consistent 
 *  manner
 */
void writeLBARDSend(char line[]){
    //T+27471ms : Sending length of bundle 92AEFEABDA1687ADC16BBECFD5B09A66296DE14F6D2007474212601FA0933B5C (bundle #0, version 1595582106764, cached_version 1595582106764)
    char msg[1000] = "";
    char timeStamp[50] = "";
    char tmp[100] = "";
    char details[500] = "";
    sscanf(line, "%s %s %[^\t\n]", &timeStamp, &tmp,  &details);
    const char* timestamp = convertLBARDTime(lbardInitialTime, timeStamp);

    if (strstr(line, "length of bundle")){
        // Format so the BID is 8 chars
        // Sending length of bundle EB7E8ABC39C548D939C5AB1E0015B7D5389D7C3FFBD1BE44439DC7C0A5F6730C (bundle #0, version 1596604638358, cached_version 1596604638358)
        char longBID[65] = "";
        char tmpDetails[500] = "";
        sscanf(details, "%s %s %s %s %s %[^\n\t]", &tmp, &tmp, &tmp, &tmp, &longBID, &details);
        char shortBID[10] = "";
        shortenBID(longBID, shortBID);
        sprintf(details, "Sending length of bundle %s %s", shortBID, tmpDetails);
    }else if (strstr(line, "Resending bundle")){
        // Also format so BID is 8 chars (diff. format)
        // Resending bundle EB7E8ABC39C548D939C5AB1E0015B7D5389D7C3FFBD1BE44439DC7C0A5F6730C from the start.
        char longBID[65] = "";
        sscanf(details, "%s %s %s", &tmp, &tmp, &longBID);
        char shortBID[10] = "";
        shortenBID(longBID, shortBID);
        sprintf(details, "Resending bundle %s from the start.", shortBID);
    }else if (strstr(details, "has finished receiving")){
        // SYNC FIN: 49eb1e150256* has finished receiving 5F89FBA833864024 version 1600145057775 (bundle #0)
        char longSID[65] = "";
        char shortSID[65] = "";
        char bid[10] = "";
        char nodeChar = '-';
        sscanf(details, "SYNC FIN: %s has finished receiving %s", longSID, bid);
        shortenSID(longSID, shortSID);
        nodeChar = getNodeFromSid(shortSID);
        sprintf(details, "Node %c (%s) has finished receiving bundle: %s", nodeChar, longSID, bid);
        // Node C (SID) has finished receiving bundle: BID
    }

    //printf("THING: %s\n", line);

    sprintf(msg, "%s LBARD:%c %s\n", timestamp,
        instanceChar, details);
    writeLineToLog(msg);
    if(outputType >= OUT_ASCII){
        if (strstr(line, "Resending bundle")){
            addToMinorEvents(msg);
        }else if (strstr(line, "Sending length")){
            addToMinorEvents(msg);
        }else if (strstr(line, "has finished receiving")){
            addToMinorEvents(msg);
        }else{
            //printf("nah: %s\n", line);
        }
    }
}

void writeLBARDSync(char line[]){
    // Sending a sync is a major event
    char msg[1000] = "";
    char tmp[100] = "";
    char timeStamp[20] = "";

    // Sending a sync message is a major event
    if (strstr(line, "Sending")){
        // >>> [19:21.37.380 EAB2*] Sending sync message (length now = $14, used 10)

        sscanf(line, ">>> [%s Sending sync message (length now = ", &timeStamp);
        if (strlen(timeStamp) != 0){
            fixLBARDTimestamp(timeStamp);
        }else{
            return;
        }
        sprintf(msg, "%s LBARD:%c Broadcast tree sync message\n", timeStamp, instanceChar);

        if(outputType >= OUT_ASCII){
            char msgDot[500] = "";
            sprintf(msgDot, "Tree sync message");

            char msgDest = '-';
            struct Events currentEvent;
            currentEvent.destinationNode = msgDest;
            currentEvent.sendingNode = instanceChar;
            currentEvent.eventType = 'S';
            strcpy(currentEvent.majorTime, timeStamp);
            strcpy(currentEvent.transferDetails, msgDot);
            currentEvent.numMinorEvents = 0;
            listEvents[currentEventIndex] = currentEvent;
            currentEventIndex += 1;
        }
        writeLineToLog(msg);
    }else{
        if (strstr(line, "now has bundle")){
            // >>> [13:31.22.510 BE69*] Peer ac800a758c78* now has bundle 1F1D0D66EE60AECFB9A430AE54FA328DE1621768F45FAC734A679F67B5368C6F* (key prefix=484c*), service=file, version=1600142462897
            char longBID[66] = "";
            char shortBID[25] = "";
            char sid[66] = "";
            char shortSID[25] = "";
            char nodeChar = '-';

            sscanf(line, ">>> [%s %s Peer %s now has bundle %s", &timeStamp, &tmp, &sid, &longBID);
            fixLBARDTimestamp(timeStamp);
            shortenBID(longBID, shortBID);
            shortenSID(sid, shortSID);
            nodeChar = getNodeFromSid(shortSID);
            sprintf(msg, "%s LBARD:%c Neighbour Node %c (%s) now has bundle %s", timeStamp, instanceChar, nodeChar, shortSID, shortBID);
            // Neighbour Node C (SID) now has bundle BID

        }else if(strstr(line, "is missing bundle")){
            char longSID[66] = "";
            char longBID[66] = "";
            char shortSID[32] = "";
            char shortBID[32] = "";
            // >>> [13:31.22.727 7400*] Peer 8bdfc6eff862* is missing bundle 1F1D0D66EE60AECFB9A430AE54FA328DE1621768F45FAC734A679F67B5368C6F* (key prefix=484C*), service=file, version=1600142462897, sender=null, recipient=null
            sscanf(line, ">>> [%s %s Peer %s is missing bundle %s", &timeStamp, &tmp, &longSID, &longBID);
            fixLBARDTimestamp(timeStamp);
            shortenBID(longBID, shortBID);
            shortenSID(longSID, shortSID);
            char nodeChar = getNodeFromSid(shortSID);
            sprintf(msg, "%s LBARD:%c Neighbour Node %c (%s) is missing bundle %s", timeStamp, instanceChar, nodeChar, shortSID, shortBID);
        }
        if (outputType >= OUT_ASCII){
            addToMinorEvents(msg);
        }
        writeLineToLog(msg);

    }    
}


/**
 *  For printing the rhizome layout of a network test.
 *  Just filters the given line for the instance ID, and what wifi interface it is connected to
 */
void writeRhizomeLayout(char line[]){
    char msg[1000] = "";
    char tmp[100] = "";
    char instChar = '-';
    int interface = 0;
    //printf("line: %s\n", line);
    //16:04:25.418 Servald instance B is being connected to Wifi interface {1}
    sscanf(line, "%s %s %s %c %s %s %s %s %s %s {%d}", \
        &tmp, &tmp, &tmp, &instChar, &tmp, &tmp, &tmp, &tmp, &tmp, &tmp, &interface);

    sprintf(msg, "#%03d %c connected to wifi interface %d\n", setupLineNumber, instChar, interface);
    if(outputType >= OUT_ASCII){
        int indx = indexRhizomeInterfaces[interface];
        RhizomeInterfaces[interface][indx] = instChar;
        indexRhizomeInterfaces[interface] += 1;        
    }
    setupLineNumber += 1;
    writeLineToLog(msg);
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
void writeFakeradioBundle(char line[]){
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
    fixLBARDTimestamp(timeStamp);

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
                if (strstr(messageType, "Bundle")){
                    // Decrease the BID to just the first 8 characters for the DOT file
                    // (otherwise, we have very wide diagrams)
                    char shortBID[10] = "";
                    shortenBID(bundleID, shortBID);

                    if (strcmp(startBytes, "")){
                        sprintf(msg, "%sFAKERADIO %c -> %c [partial bundle]  bid=%s [%s to %s. Total: %s bytes]\n", timeStamp, sendNode, destNode, bundleID, startBytes, endBytes, byteLength);
                        // // The transfer message that will be displayed in the DOT file
                        // sprintf(msgDot, "[partial bundle]  bid=%s [%s to %s. Length: %s bytes]", shortBID, startBytes, endBytes, byteLength);
                    }else{
                        sprintf(msg, "%sFAKERADIO %c -> %c [bundle piece]  bid=%s [%s bytes]\n", timeStamp, sendNode, destNode, bundleID, byteLength);
                        // // The transfer message that will be displayed in the DOT file
                        // sprintf(msgDot, "[bundle piece] bid=%s [%s bytes]", shortBID, byteLength);
                    }
                }else{
                    sprintf(msg, "%sFAKERADIO %c -> %c [%s] [%s bytes]\n", timeStamp, sendNode, destNode, messageType, byteLength);
                    // The transfer message that will be displayed in the DOT file
                    sprintf(msgDot, "[%s] [%s bytes]", messageType, byteLength);
                    // printf("fakeradio event: %s \n", msg);
                }
                if(outputType >= OUT_ASCII){
                    if (strlen(msgDot) > 0){
                        struct Events currentEvent;
                        currentEvent.destinationNode = destNode;
                        currentEvent.sendingNode = sendNode;
                        currentEvent.eventType = 'F';
                        currentEvent.numMinorEvents = 0;
                        strcpy(currentEvent.majorTime, timeStamp);
                        strcpy(currentEvent.transferDetails, msgDot);
                        listEvents[currentEventIndex] = currentEvent;
                        currentEventIndex += 1;
                    }
                    memset(msgDot, 0, strlen(msgDot));
                    // memset(timeStamp, 0, strlen(timeStamp));
                }
                writeLineToLog(msg);
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
char *longToTimestamp(long tmpLong){
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
char *convertLBARDTime(long initialTime, char *timeStamp){
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
    char* buf = longToTimestamp(timeStampMilli);
    return buf;
}

/**
 * Fixes LBARD timestamps so they are consistent with servald timestamps
 * Simply a matter of removing a '[' and changing a '.' to a ':'
 */
void fixLBARDTimestamp(char timeStamp[]){
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
long timestampToLong(char timeStamp[20]){
    // Why do we do it like this and not just change LBARD?
    /// Well, hypothetical question, this is because we are going to be comparing these logs
    // against existing topologies and we don't want to have to update LBARD on them too!
    
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
char getNodeFromSid(char sid[]){
    // Convert to upper just in case
    if (strlen(sid) == 0){
        fprintf(stderr, "Received an empty SID\n");
        return '-';
    } 
    for (int i = 0; i < strlen(sid); i++){
        sid[i] = toupper(sid[i]);
    }
    if (strstr(sid, "*")){
        // Assume it's in the last place..
        int index = strlen(sid) -1;
        sid[index] = '\0';
    }

    for (int i = 0; i < 26; i++){
        if (strstr(sidArray[i], sid)){
            return i+65;
        }
    }
    printf("Could not find sid for: %s sid");
    return '-';
}

/**
 *  Utility function. Allows us to easily add lines to the minor event list
 */
void addToMinorEvents(const char line[]){
    char timestamp[20] = "";
    char restOfMessage[256] = "";
    sscanf(line, "%s %[^\t\n]", timestamp, restOfMessage);
    struct MinorEvents minorEvent;
    strcpy(minorEvent.majorTime, timestamp);
    strcpy(minorEvent.message, restOfMessage);

    globalMinorEventArray[numGlobalMinorEvents] = minorEvent;
    numGlobalMinorEvents += 1;
}

void shortenBID(const char longBID[], char shortBID[]){
    memcpy(shortBID, &longBID[0], 8);
    shortBID[8] = '*';
    shortBID[9] = '\0';
    // Make it uppercase
    for (int i = 0; i < 9; i++){
        shortBID[i] = toupper(shortBID[i]);
    }
}

char shortenSID(const char longSID[], char shortSID[]){
    memcpy(shortSID, &longSID[0], 8);
    // NOTE: We don't put a * because it makes the matching not work 
    shortSID[9] = '\0';
    for (int i = 0; i < 9; i++){
        shortSID[i] = toupper(shortSID[i]);
    }
    return getNodeFromSid(shortSID);
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
    long timeStampALong = timestampToLong(timeStampA);
    long timeStampBLong = timestampToLong(timeStampB);

    if(timeStampALong < timeStampBLong){
        return -1;
    }else if (timeStampALong > timeStampBLong){
        return 1;
    }
    return 0;  
}

/**
 *  Serves as a comparator for MinorEvent objets. Simply compares their timestamps so they are sorted
 *  in chronological order
 */
int minorEventSort(const void* a, const void* b){
    const struct MinorEvents *elem1 = a;    
    const struct MinorEvents *elem2 = b;
    char timeStampA[20] = "";
    char timeStampB[20] = "";

    strcpy(timeStampA, elem1->majorTime);
    strcpy(timeStampB, elem2->majorTime);
    long timeStampALong = timestampToLong(timeStampA);
    long timeStampBLong = timestampToLong(timeStampB);

    if(timeStampALong < timeStampBLong){
        return -1;
    }else if (timeStampALong > timeStampBLong){
        return 1;
    }
    return 0;  
}

/**
 * Utility function, just writes the given line to the output log file
 */
void writeLineToLog(char line[]){
    fputs(line, outputFile);
}