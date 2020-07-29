/** 
 *  @file    MetaData.h
 *  @author  Ian Vanderhoff
 *  @date    4/20/2018 
 *  @version 2
 *  
 */

#ifndef _META_DATA
#define _META_DATA

#include <string>
#include <fstream>
#include <regex>
#include <vector>
#include <stdexcept>

#include "Configuration.h"
#include "ReadyQueue.h"
// Process.h included with ReadyQueue.h
// MetaCommand.h included with Process.h
// CycleTime.h included with Configuration.h


// Arrays of keys and descriptions for cycle time calculations in handler()
const std::string DESC_ARR[] = {"run", "hard drive", "keyboard", "scanner", 
                               "monitor", "projector", "block", "allocate"};

const std::string KEY_ARR[] = {"processor", "hardDrive", "keyboard", "scanner", 
                              "monitor", "projector", "memory", "memory"};

const int NUM_DESC = 8; // the number of descriptions in DESC_ARR


/**
 * @brief      Class for parsing the meta data file, storing its processes,
 *             and returning the process operations and information to the user.
 */
class MetaData
{
public:
    //SETTER FUNCTIONS
    void setConfigurationObject(const Configuration configObj) {
        configObj_ = configObj;
    }

    // PUBLIC FUNCTIONS
    void parseMDF(std::string metaDataFilePath, ReadyQueue& readyQ);

    // Default Constructor
    MetaData() {
        // processIndex_ = 0;
        currentOpIndex_ = 0;
        sFinishFound_ = false;
        ioOpCountForProcess_ = 0;
    }
    
private:
    // PRIVATE DATA
    Process processObj_;
    Configuration configObj_;
    int currentOpIndex_; // index of current operation in process
    bool sFinishFound_; // found the S{finish} command
    int ioOpCountForProcess_;

    // PRIVATE FUNCTIONS
    void parseFullLine(std::string fullString, ReadyQueue& readyQ);
    void handleCodeDescErrors(char codeChar, std::string descriptor);
    bool checkSpecialCommands(char codeChar, std::string descriptor);

    // PRIVATE COMMAND HANDLER FUNCTIONS
    void handler(std::string code, 
                 std::string descriptor, 
                 std::string cycles, 
                 ReadyQueue& readyQ);
};

//******************************************************************************
// IMPLEMENTATION
//******************************************************************************

/**
 * @brief      Public function used to parse the meta data file (path to meta
 *             data file provided in the configuration file)
 *
 * @param[in]  metaDataFilePath  (std::string) The meta data file path
 * @param[in]  readyQ  (ReadyQueue&) Reference to the ready queue
 */
void MetaData::parseMDF(std::string metaDataFilePath, ReadyQueue& readyQ) {
    std::ifstream inputFile(metaDataFilePath.c_str());

    if (inputFile.fail()) {
        // Clear input stream following failure
        inputFile.clear();
        
        // throw exception
        throw std::logic_error("MetaData Class: Incorrect Meta Data File Path");
    }

    // holds full line from file
    std::string fullLine;

    // Priming Read:(to avoid errors)
    std::getline(inputFile, fullLine);

    // Read the rest of the file
    while (inputFile) {
        parseFullLine(fullLine, readyQ);
        std::getline(inputFile, fullLine);
    }

    inputFile.clear();

    inputFile.close();
}

/**
 * @brief      Uses regular expressions to parse a full line from the meta data
 *             file, then calls the handler() function to handle the command.
 *
 * @param[in]  fullLine  (std::string) raw string possibly containing multiple commands
 * @param[in]  readyQ  (ReadyQueue&) Reference to the ready queue
 */
void MetaData::parseFullLine(std::string fullLine, ReadyQueue& readyQ) {
    // Check for Start or End commands which don't have delimiters
    /* Regex:
            * means character can occur zero or more times
            + means character can occur one or more times
            ^ means start of string
            $ means end of string (^ and $ ignore line-terminators)
            [\\s]* means zero or more whitespace characters
    */
    std::regex startCmdRE("^[\\s]*Start[\\s]+Program[\\s]+Meta-Data[\\s]+Code[\\s]*$");
    std::regex endCmdRE("^[\\s]*End[\\s]+Program[\\s]+Meta-Data[\\s]+Code[\\s]*$");

    // compare fullLine with regex for match
    if((std::regex_match(fullLine, startCmdRE)) || (std::regex_match(fullLine, endCmdRE))) {
        return;
    }

    /* Regex:
            \\b is needed otherwise it only pulls the first command from fullLine
                \\b means word boundry
            (S|A|P|I|O|M) means one of these characters is required and is held as
                a sub_match
            \\{ and \\} means {} are required
            ([a-z ]+) means that one or more lowercase alpha chars or spaces are requires
                and held as a sub_match
            ([0-9]+) means that one or more digits are required and held as sub_match
            ; means that semicolon is required
    */
    std::regex cmdStringRE("\\b[\\s]*(S|A|P|I|O|M)\\{([a-z ]+)\\}([0-9]+);");

    // regex iterators pointing to beginning and end of fullLine to iterate over
    // the valid cmdStringRE matches
    std::regex_iterator<std::string::iterator> rit (fullLine.begin(), fullLine.end(), cmdStringRE);
    std::regex_iterator<std::string::iterator> rend;
    while ((rit!=rend) && (!sFinishFound_)) {
        handler(rit->str(1), rit->str(2), rit->str(3), readyQ);

        // increment iterator
        ++rit;
    }
}

/**
 * @brief      Handles the different individual commands coming from the meta data file.
 * 
 * Sorts the command by the single character command code. Receives and
 * validates the number of cycles provided for the command. Stores the information
 * for the command in a MetaCommand object. If a new process is being created,
 * a new Process object is created, metacommand object is stored in the Process
 * object, the number of operations in the process object is updated, the number
 * of i/o operations in the process object is updated.
 *
 * @param[in]  code        (std::string) The raw string of the code value
 * @param[in]  descriptor  (std::string) The raw string of the descriptor value
 * @param[in]  cycles      (std::string) The raw string of the cycles value
 * @param[in]  readyQ      (ReadyQueue&) Reference to the ready queue
 */
void MetaData::handler(std::string code, 
                       std::string descriptor, 
                       std::string cycles, 
                       ReadyQueue& readyQ) {


    char codeChar = code.at(0); // convert code to char
    int numCycles = atoi(cycles.c_str()); // convert string to int

    if (numCycles < 0) {
        // throw exception
        throw std::logic_error("MetaData Class: Number of Cycles is Less Than Zero");
    }

    // Call function to handle errors of code descroptor mismatch
    handleCodeDescErrors(codeChar, descriptor);

    // Check for Special Command errors and return circumstances
    if (checkSpecialCommands(codeChar, descriptor)) {
        return;
    }

    // Check for A{begin} and create Process object if so
    if ((codeChar == 'A') && (descriptor == "begin")) {
        Process tempProcessObj;

        // Reset I/O operation count to zero
        ioOpCountForProcess_ = 0;

        // Overwrite old process object with new one
        processObj_ = tempProcessObj;
    }

    // Calculate time for cycles
    int tempTime = 0;
    // DESC_ARR[] and KEY_ARR[] are global constants at top of file
    for (int index = 0; index < NUM_DESC; index++) {
        if (descriptor == DESC_ARR[index]) {
            CycleTime tempObj = configObj_.getCycleTime(KEY_ARR[index]);
            tempTime = (tempObj.getTime() * numCycles);
        }
    }

    // add operation time to total process time remaining
    processObj_.addProcessTimeRemain(tempTime);

    // Create MetaCommand object and fill with info
    MetaCommand tempMetaCmdObj(codeChar, descriptor, numCycles, tempTime);

    // Check for I/O operation
    if ((codeChar == 'I') || (codeChar == 'O')) {

        // increment number of I/O ops for this process
        ioOpCountForProcess_++;
    }

    // Insert MetaCommand object into the Process object.
    processObj_.insertCommand(tempMetaCmdObj);

    // Check for Process end and increment processIndex_ if so
    if ((codeChar == 'A') && (descriptor == "finish")) {

        // Add number of I/O ops to the process object
        processObj_.setNumIOops(ioOpCountForProcess_);

        // set process number
        processObj_.setProcessNumber(readyQ.getProcessArrivalIndx());

        // increment process arrival index in ready queue
        readyQ.incrementProcessArrivalIndx();

        // Add process to ready queue
        readyQ.insertProcess(processObj_);

        // // Sort the ready queue
        // readyQ.sortReadyQueue();
    }

    // increment current overall read operation index
    currentOpIndex_++;
}


/**
 * @brief      Validates that the descriptor for the command is valid
 *             for the code provided.
 *
 * @param[in]  codeChar (char)   The code character
 * @param[in]  descriptor (std::string) The descriptor string
 */
void MetaData::handleCodeDescErrors(char codeChar, std::string descriptor) {
    // handle errors for incorrect descriptors for given codes
    switch (codeChar) {
        case 'S':
        {
            if ((descriptor != "begin") && (descriptor != "finish")) {
                // throw exception
                throw std::logic_error("MetaData Class: Incorrect Descriptor for 'S' Command");
            }
            break;
        }
        case 'A':
        {
            if ((descriptor != "begin") && (descriptor != "finish")) {
                // throw exception
                throw std::logic_error("MetaData Class: Incorrect Descriptor for 'A' Command");
            }
            break;
        }
        case 'P':
        {
            if (descriptor != "run") {
                // throw exception
                throw std::logic_error("MetaData Class: Incorrect Descriptor for 'P' Command");
            }
            break;
        }
        case 'I':
        {
            if ((descriptor != "hard drive") && (descriptor != "keyboard") && (descriptor != "scanner")) {
                std::cout << descriptor << std::endl;
                // throw exception
                throw std::logic_error("MetaData Class: Incorrect Descriptor for 'I' Command");
            }
            break;
        }
        case 'O':
        {
            if ((descriptor != "hard drive") && (descriptor != "monitor") && (descriptor != "projector")) {
                // throw exception
                throw std::logic_error("MetaData Class: Incorrect Descriptor for 'O' Command");
            }
            break;
        }
        case 'M':
        {
            if ((descriptor != "block") && (descriptor != "allocate")) {
                // throw exception
                throw std::logic_error("MetaData Class: Incorrect Descriptor for 'M' Command");
            }
            break;
        }
    }
}


/**
 * @brief      Checks for special command return conditions.
 *
 * @param[in]  codeChar    (char) The code character
 * @param[in]  descriptor  (std::string) The descriptor
 *
 * @return     true for return false for don't return.
 */
bool MetaData::checkSpecialCommands(char codeChar, std::string descriptor) {

    // Check that S{begin} is first operation
    if (currentOpIndex_ == 0) {
        if ((codeChar != 'S') || (descriptor != "begin")) {
            // throw exception
            throw std::logic_error("MetaData Class: S{begin} must be first operation");
        }
        else {
            // increment current overall read operation index
            currentOpIndex_++;
            // ignore S operation as it is not part of the process.
            return true;
        }
    }

    // Check that A{begin} is second operation
    if (currentOpIndex_ == 1) {
        if ((codeChar != 'A') || (descriptor != "begin")) {
            // throw exception
            throw std::logic_error("MetaData Class: A{begin} must be second operation");
        }
    }

    // Stop parsing if S{finish} is encountered
    if ((codeChar == 'S') && (descriptor == "finish")) {
        // set flag
        sFinishFound_ = true;

        // increment current overall read operation index
        currentOpIndex_++;

        // ignore and escape to finish parsing (S not part of process)
        return true;
    }
    return false;
}

#endif