/** 
 *  @file    Configuration.h
 *  @author  Ian Vanderhoff
 *  @date    5/2/2018 
 *  @version 5 
 *
 */

#ifndef _CONFIGURATION
#define _CONFIGURATION

#include <string>
#include <fstream>
#include <unordered_map>
#include <regex>
#include <stdexcept>

#include "CycleTime.h"

/**
 * @brief      Global enum values used for encoding and interpreting a command
 *             string from the configuration file.
 */
enum Command {VERSION, MDF, PROJECTOR, PROCESSOR, KEYBOARD, 
              MONITOR, SCANNER, HARD_DRIVE, LOG, LOG_FILE, 
              MEMORY, SYSMEMORY, BLOCK_SIZE, RESOURCE, PQN, 
              SCHED_CODE, UNKNOWN};

/**
 * @brief      Glogal constant for the number of I/O resources
 */
const int NUM_IO_RESOURCES = 5;

/**
 * @brief      Class for parsing the configuration file, storing its 
 *  		   information, and returning the information to the user.
 */	
class Configuration
{
public:
    // GETTER FUNCTIONS
    double getVersion() const;
    std::string getMdfFilePath() const;
    std::string getLogFilePath() const;
    int getLogFileMethod() const;
    int getSystemMemory() const;
    int getBlockSize() const;
    // Indices: 0=hard drive, 1=keyboard, 2=scanner, 3=monitor, 4=projector
    int getResourceSize(int resourceIndex) const;
    // keys are: projector, processor, keyboard, monitor, scanner, hardDrive
    CycleTime getCycleTime(std::string key);
    // 0 for RR, 1 for STR
    int getSchedCode() const;
    int getPQN() const;

    // PUBLIC FUNCTIONS
    void parseConfigFile(std::string configFile);

private:
    // PRIVATE DATA
    double version_;
    std::string mdfFilePath_;
    std::string logFilePath_;
    // 0 for monitor, 1 for logfile, 2 for both
    int logFileMethod_;
    // 0 for RR, 1 for STR
    int schedCode_;
    int systemMemory_;
    int blockSize_;
    int pqn_;
    // unordered_map using keys store/access CycleTime objects
    // keys are: projector, processor, keyboard, monitor, scanner, hardDrive
    std::unordered_map<std::string, CycleTime> timeMap_;
    // Array for holding I/O process resources
    // Indices: 0=hard drive, 1=keyboard, 2=scanner, 3=monitor, 4=projector
    int ioResources_[NUM_IO_RESOURCES];

    // PRIVATE FUNCTIONS
    void parseCommandString(std::string cmdString);
    Command ClassifyCmd(std::string command, 
                        int& bytesSysMem, 
                        int& bytesBlockSize, 
                        int& resourceIndex);
    
    // PRIVATE COMMAND HANDLER FUNCTIONS
    void handleVersion(std::string value);
    void handleMdf(std::string value);
    void handleLog(std::string value);
    void handleLogFile(std::string value);
    void handleSystemMemory(std::string value, int bytesSysMem);
    void handleBlockSize(std::string value, int bytesBlockSize);
    void handleResource(std::string value, int resourceIndex);
    void handleTime(std::string value, Command cmd);
    void handlePQN(std::string value);
    void handleSchedCode(std::string value);
};

//******************************************************************************
// IMPLEMENTATION
//******************************************************************************

/**
 * @brief      Public function used to parse configuration file (path to config 
 * 			   file provided in command line)
 *
 * @param[in]  configFile  (std::string) The configuration file path
 */
void Configuration::parseConfigFile(std::string configFile) {
    std::ifstream inputFile(configFile.c_str());

    if (inputFile.fail()) {
        // Clear input stream following failure
        inputFile.clear();
        
        // throw exception
        throw std::logic_error("Configuration Class: Incorrect Configuration File Path");
    }

    // holds full line from file
    std::string cmdString;

    // Initialize I/O resources to 1
    for (int i = 0; i < NUM_IO_RESOURCES; i++) {
        ioResources_[i] = 1;
    }

    // Priming Read (to avoid errors)
    std::getline(inputFile, cmdString);

    // Read the rest of the file
    while (inputFile) {
        parseCommandString(cmdString);
        std::getline(inputFile, cmdString);
    }

    inputFile.close();
}

/**
 * @brief      Uses regular expressions to parse the full line for valid command 
 * 			   information. The information in the command is classified by 
 * 			   ClassifyCmd() then handled by the appropriate command handling 
 * 			   function.
 *
 * @param[in]  cmdString  (std::string) The full raw command string
 */
void Configuration::parseCommandString(std::string cmdString) {
    // Check for Start or End commands which don't have delimiters
    /* Regex:
            * means character can occur zero or more times
            + means character can occur one or more times
            ^ means start of string
            $ means end of string (^ and $ ignore line-terminators)
            [\\s] means any whitespace character
    */
    std::regex startCmdRE("^[\\s]*Start[\\s]+Simulator[\\s]+Configuration[\\s]+File[\\s]*$");
    std::regex endCmdRE("^[\\s]*End[\\s]+Simulator[\\s]+Configuration[\\s]+File[\\s]*$");

    // compare cmdString with regex for match
    if((std::regex_match(cmdString, startCmdRE)) || (std::regex_match(cmdString, endCmdRE))) {
        return;
    }

    /*	Regex:
    		([\\w\\{\\}/\\s]+) means one or more alpha-numeric characters, underscores,
    			open or closed curly brackets, forward-slashes, or whitespaces are stored in
    			the forst sub_match
    		[\\s]* means zero or more whitespaces are ignored
    		: is a required delimiter
    		([\\w\\.\\s]+.) means one or more alpha-numeric characters, underscores,
				periods, (see next line for .) are stored in second sub_match
		**note:
			I had to add the . at the end of the sub_match to avoid the sub_match
			from collecting a trailing carriage-return (ASCII code 13)
			The . means any character except a line-terminator
    */
    std::regex cmdStringRE("^[\\s]*([\\w\\{\\}/\\s]+)[\\s]*:[\\s]*([\\w\\.\\s]+.)[\\s]*$");
    std::smatch collectedInfo;
    // collectedInfo holds information collected by regex_match about items
    // in the parenthesis (submatches)
    std::regex_match(cmdString, collectedInfo, cmdStringRE);

    // integer reference for kB=1, MB=2, or GB=3. 0 for not applicable.
    int bytesSysMem, bytesBlockSize = 0;

    // integer for I/O resource index reference
    int resourceIndex = -1;

    Command thisCmd = ClassifyCmd(collectedInfo[1], bytesSysMem, bytesBlockSize, resourceIndex); // first sub_match is command

    // call command handling functions,
    // pass in remainder of command stringstream after delimiter (value for command)
    switch (thisCmd) {
        case VERSION:       handleVersion(collectedInfo[2]);
                            break;
        case MDF:           handleMdf(collectedInfo[2]);
                            break;
        case PROJECTOR:     handleTime(collectedInfo[2], PROJECTOR);
                            break;
        case PROCESSOR:     handleTime(collectedInfo[2], PROCESSOR);
                            break;
        case KEYBOARD:      handleTime(collectedInfo[2], KEYBOARD);
                            break;
        case MONITOR:       handleTime(collectedInfo[2], MONITOR);
                            break;
        case SCANNER:       handleTime(collectedInfo[2], SCANNER);
                            break;
        case MEMORY:      	handleTime(collectedInfo[2], MEMORY);
                            break;
        case HARD_DRIVE:    handleTime(collectedInfo[2], HARD_DRIVE);
                            break;
        case LOG:           handleLog(collectedInfo[2]);
                            break;
        case LOG_FILE:      handleLogFile(collectedInfo[2]);
                            break;
        case SYSMEMORY:     handleSystemMemory(collectedInfo[2], bytesSysMem);
                            break;
        case BLOCK_SIZE:    handleBlockSize(collectedInfo[2], bytesBlockSize);
                            break; 
        case RESOURCE:      handleResource(collectedInfo[2], resourceIndex);
                            break;
        case PQN:           handlePQN(collectedInfo[2]);
                            break;      
        case SCHED_CODE:    handleSchedCode(collectedInfo[2]);
                            break;            
        case UNKNOWN:       throw std::logic_error("Configuration Class: Invalid Command In Configuration File");
    }
}

/**
 * @brief      Classifies the command string and converts it to an enum value.
 *
 * @param[in]  command  (std::string) The command
 * @param[in]  bytesSysMem  (int&) integer reference for kB=1, MB=2, or GB=3. 0 for not applicable.
 * @param[in]  bytesSysMem  (int&) integer reference for kB=1, MB=2, or GB=3. 0 for not applicable.
 * @param[in]  resourceIndex  (int&) index of resource in ioResources_ array.
 *
 * @return     (enum) The corresponding enum value if found
 */
Command Configuration::ClassifyCmd(std::string command, int& bytesSysMem, int& bytesBlockSize, int& resourceIndex) {
    if (command == "Version/Phase") return VERSION;
    else if (command == "File Path") return MDF;
    else if (command == "Quantum Number {msec}") return PQN;
    else if (command == "CPU Scheduling Code") return SCHED_CODE;
    else if (command == "Projector cycle time {msec}") return PROJECTOR;
    else if (command == "Processor cycle time {msec}") return PROCESSOR;
    else if (command == "Keyboard cycle time {msec}") return KEYBOARD;
    else if (command == "Monitor display time {msec}") return MONITOR;
    else if (command == "Scanner cycle time {msec}") return SCANNER;
    else if (command == "Hard drive cycle time {msec}") return HARD_DRIVE;
    else if (command == "Memory cycle time {msec}") return MEMORY;
    else if (command == "Log") return LOG;
    else if (command == "Log File Path") return LOG_FILE;
    else if (command == "System memory {kbytes}") {
        bytesSysMem = 1;
        return SYSMEMORY;
    }
    else if (command == "System memory {Mbytes}") {
        bytesSysMem = 2;
        return SYSMEMORY;
    }
    else if (command == "System memory {Gbytes}") {
        bytesSysMem = 3;
        return SYSMEMORY;
    }
    else if (command == "Memory block size {kbytes}") {
        bytesSysMem = 1;
        return BLOCK_SIZE;
    }
    else if (command == "Memory block size {Mbytes}") {
        bytesSysMem = 2;
        return BLOCK_SIZE;
    }
    else if (command == "Memory block size {Gbytes}") {
        bytesSysMem = 3;
        return BLOCK_SIZE;
    }
    else if (command == "Hard drive quantity") {
        resourceIndex = 0;
        return RESOURCE;
    }
    else if (command == "Keyboard quantity") {
        resourceIndex = 1;
        return RESOURCE;
    }
    else if (command == "Scanner quantity") {
        resourceIndex = 2;
        return RESOURCE;
    }
    else if (command == "Monitor quantity") {
        resourceIndex = 3;
        return RESOURCE;
    }
    else if (command == "Projector quantity") {
        resourceIndex = 4;
        return RESOURCE;
    }

    return UNKNOWN;
}

/**
 * @brief      Handles version command.
 * 
 * Converts string to double, validates value of double, and stores the version
 * number.
 *
 * @param[in]  value  (std::string) raw string of value
 */
void Configuration::handleVersion(std::string value) {
    double version = atof(value.c_str()); // convert string to double

    if (version < 0) {
        // throw exception
        throw std::logic_error("Configuration Class: Version Number is Less Than Zero");
    }

    version_ = version;
}


/**
 * @brief      Handles process quantum number command.
 * 
 * Converts string to int, validates value of int, and stores number.
 *
 * @param[in]  value  (std::string) The value
 */
void Configuration::handlePQN(std::string value) {
    int aPQN = atoi(value.c_str()); // convert string to int

    if (aPQN < 0) {
        // throw exception
        throw std::logic_error("Configuration Class: PQN is Less Than Zero");
    }

    pqn_ = aPQN;
}

/**
 * @brief      Handles the meta data file path command.
 * 
 * Stores the string value of the file path from the config file.
 *
 * @param[in]  value  (std::string) raw string of value
 */
void Configuration::handleMdf(std::string value) {
    // Check for valid filepath when opening

    mdfFilePath_ = value;
}

/**
 * @brief      Handles the log file path command.
 * 
 * Stores the string value of the file path from the config. file.
 *
 * @param[in]  value  (std::string) raw string of value
 */
void Configuration::handleLogFile(std::string value) {
    logFilePath_ = value;
}

/**
 * @brief      Handles the log file method command.
 * 
 * Validates the value of the string. Converts the value to an integer for
 * easier interpretation later. Stores the integer value.
 *
 * @param[in]  value  (std::string) raw string of value
 */
void Configuration::handleLog(std::string value) {
    if (value == "Log to Both") {
        logFileMethod_ = 2;
    } else if (value == "Log to Monitor") {
        logFileMethod_ = 0;
    } else if (value == "Log to File") {
        logFileMethod_ = 1;
    } else {
        // throw exception
        throw std::logic_error("Configuration Class: Incorrect File Logging Method");
    }
}


/**
 * @brief      Handles the schedule code command.
 * 
 * Checks for the scheduling algorithm specified in the configuration
 * file. Stores the value as an integer for easier interpretation.
 * RR = round robin
 * STR = shortest time remaining (operations left * time per operations)
 *
 * @param[in]  value (std::string) String identifying the algorithm
 */
void Configuration::handleSchedCode(std::string value) {
    if (value == "RR") {
        schedCode_ = 0;
    } else if (value == "STR") {
        schedCode_ = 1;
    } else {
        // throw exception
        throw std::logic_error("Configuration Class: Invalid CPU Scheduling Code");
    }
}

/**
 * @brief      Handles the system memory command.
 *
 * @param[in]  value        (std::string) the raw string of value
 * @param[in]  bytesSysMem  (int) determines KB, MB, or GB
 */
void Configuration::handleSystemMemory(std::string value, int bytesSysMem) {
    int aMemory = atoi(value.c_str()); // convert string to int

    if (aMemory < 0) {
        // throw exception
        throw std::logic_error("Configuration Class: System Memory is Less Than Zero");
    }

    // convert MB to kB
    if (bytesSysMem == 2) {
        aMemory *= 1000;
    }
    // convert GB to kb
    if (bytesSysMem == 3) {
        aMemory *= 1000000;
    }

    systemMemory_ = aMemory;
}

/**
 * @brief      Handles the memory block size command.
 *
 * @param[in]  value        (std::string) the raw string of value
 * @param[in]  bytesBlockSize  (int) determines KB, MB, or GB
 */
void Configuration::handleBlockSize(std::string value, int bytesBlockSize) {
    int aMemory = atoi(value.c_str()); // convert string to int

    if (aMemory < 0) {
        // throw exception
        throw std::logic_error("Configuration Class: Block Size is Less Than Zero");
    }

    // convert MB to kB
    if (bytesBlockSize == 2) {
        aMemory *= 1000;
    }
    // convert GB to kb
    if (bytesBlockSize == 3) {
        aMemory *= 1000000;
    }

    blockSize_ = aMemory;
}

/**
 * @brief      Handles the resource number command.
 *
 * @param[in]  value          (std::string) the raw string of the value
 * @param[in]  resourceIndex  (int) the index of the resource in the array.
 */
void Configuration::handleResource(std::string value, int resourceIndex) {
    int aSize = atoi(value.c_str()); // convert string to int

    if (aSize < 1) {
        // throw exception
        throw std::logic_error("Configuration Class: Resource Size is Less Than One");
    }

    ioResources_[resourceIndex] = aSize;
}

/**
 * @brief      Handles the time-based commands for different processes.
 * 
 * Validates that the time is not less than zero. Creates a CycleTime object
 * and stores the name of the process and the time for the cycle in the object.
 * Then the CycleTime object is inserted into the unordered_map under the 
 * appropriate key.
 *
 * @param[in]  value  (std::string) raw string of value
 * @param[in]  cmd    (enum) The enum value of the interpreted command
 */
void Configuration::handleTime(std::string value, Command cmd) {
    int aTime = atoi(value.c_str()); // convert string to int

    if (aTime < 0) {
        // throw exception
        throw std::logic_error("Configuration Class: Time is Less Than Zero");
    }

    // Use Command enum value to sort the process
    // create CycleTime obj.
    // assign key value pair to map
    switch (cmd) {
        case PROJECTOR:
        {
            CycleTime aCycTime("Projector", aTime);
            timeMap_.insert({"projector", aCycTime});
            break;
        }
        case PROCESSOR:
        {
            CycleTime aCycTime("Processor", aTime);
            timeMap_.insert({"processor", aCycTime});
            break;
        }
        case KEYBOARD:
        {
            CycleTime aCycTime("Keyboard", aTime);
            timeMap_.insert({"keyboard", aCycTime});
            break;
        }
        case MONITOR:
        {
            CycleTime aCycTime("Monitor", aTime);
            timeMap_.insert({"monitor", aCycTime});
            break;
        }
        case SCANNER:
        {
            CycleTime aCycTime("Scanner", aTime);
            timeMap_.insert({"scanner", aCycTime});
            break;
        }
        case HARD_DRIVE:
        {
            CycleTime aCycTime("Hard Drive", aTime);
            timeMap_.insert({"hardDrive", aCycTime});
            break;
        }
        case MEMORY:
        {
            CycleTime aCycTime("Memory", aTime);
            timeMap_.insert({"memory", aCycTime});
            break;
        }
        default:
            break;
    }
}

/**
 * @brief      Gets the version.
 *
 * @return     (double) The version.
 */
double Configuration::getVersion() const {
    return version_;
}

/**
 * @brief      Get the meta data file path
 *
 * @return     (std::string) The mdf file path.
 */
std::string Configuration::getMdfFilePath() const {
    return mdfFilePath_;
}

/**
 * @brief      Gets the log file path.
 *
 * @return     (std::string) The log file path.
 */
std::string Configuration::getLogFilePath() const {
    return logFilePath_;
}

/**
 * @brief      Gets the log file method.
 *
 * @return     (int) The log file method (value of 0, 1, 2).
 */
int Configuration::getLogFileMethod() const {
    return logFileMethod_;
}


/**
 * @brief      Gets the system memory size.
 *
 * @return     (int) The system memory size in designated bytes.
 */
int Configuration::getSystemMemory() const {
    return systemMemory_;
}

/**
 * @brief      Gets the memory block size.
 *
 * @return     (int) The memory block size in designated bytes.
 */
int Configuration::getBlockSize() const {
    return blockSize_;
}


/**
 * @brief      Gets the number of resourses allocated to I/O device
 *
 * @param[in]  (int) resourceIndex  The index of the I/O resource in the array
 *                                  Indices:
                                        0 = hard drive
                                        1 = keyboard
                                        2 = scanner
                                        3 = monitor
                                        4 = projector
 *
 * @return     (int) The number of resourses allocated to I/O device.
 */
int Configuration::getResourceSize(int resourceIndex) const {
    return ioResources_[resourceIndex];
}


/**
 * @brief      Gets the integer representing the scheduling algorithm.
 * 
 * 0 = FIFO, 1 = SJF, 2 = PQ
 *
 * @return     (int) The scheduling code integer.
 */
int Configuration::getSchedCode() const {
    return schedCode_;
}

/**
 * @brief      Get the CycleTime object for a specific process.
 *
 * @param[in]  key   (std::string) The key for the appropriate process
 *
 * @return     (CycleTime) The CycleTime object holding the name and time of the process.
 */
CycleTime Configuration::getCycleTime(std::string key) {
    // If key not found in map iterator to end is returned
    if (timeMap_.find(key) == timeMap_.end()) {
        // throw exception
        throw std::logic_error("Configuration Class: Invalid Key Entered");
    }

    CycleTime tempObj = timeMap_[key];    // use key to return CycleTime object

    return tempObj;
}

int Configuration::getPQN() const {
    return pqn_;
}

#endif