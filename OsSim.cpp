/** 
 *  @file    OsSim.cpp
 *  @author  Ian Vanderhoff
 *  @date    5/2/2018 
 *  @version 4 
 *  
 *  @brief CS 446_1001 Assignment 5
 *
 */

#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>

#include "OSprocessRunner.h"
// MetaData.h included with OSprocessRunner.h
// Configuration.h included with MetaData.h
// CycleTime.h included with Configuration.h
// ReadyQueue.h included with MetaData.h
// Process.h included with ReadyQueue.h
// MetaCommand.h included with Process.h

//FUNCTION PROTOTYPES
void outputDisplay(std::chrono::duration<double> aDuration,
                   OSprocessRunner& anApplication,
                   std::ostream& out);
/**
 * @brief      Takes in Configuration file and outputs information
 *             from a process run by OSprocessRunner to the
 *             log file method stated in the config file.
 *
 * @param[in]  argc  (int) The number of command line arguments
 * @param      argv  (char const* []) Array of command line arguments
 *
 * @return     (int) 0
*/
int main(int argc, char const *argv[])
{
	// Set the starting timepoint for the simulation
    // This is used for timestamping
    // This gets passed to OSprocessRunner class constructor 
    // auto replaces the cumbersome return type
    auto begin = std::chrono::steady_clock::now(); 

    // get timestamp for start of simulation
    auto start = std::chrono::steady_clock::now();
    std::chrono::duration<double> aDuration;
    aDuration = (start - begin);

    // retrieve configuration file path
    std::string configFileName = argv[1];

    // create Configuration object
    Configuration configObj;

    // Parse the configuration file with the Configuration object
    configObj.parseConfigFile(configFileName);

    // Fill the resource deques (static member function)
    OSprocessRunner::fillResourceDeques(configObj.getResourceSize(0),
                                        configObj.getResourceSize(1),
                                        configObj.getResourceSize(2),
                                        configObj.getResourceSize(3),
                                        configObj.getResourceSize(4));

    // Retrieve log file method from the Configuration object.
    int logFileMethod = configObj.getLogFileMethod();

    // Create a OSprocessRunner object with the begin timepoint,
    // and the Configuration object.
    OSprocessRunner anApplication(begin, configObj);

    std::cout << std::endl << "Running Simulation..." << std::endl;

    // Run the OSprocessRunner
    anApplication.runProcesses();

    std::cout << std::endl << "Simulation Completed." << std::endl;

    // Logging
    if (logFileMethod == 0) {
        outputDisplay(aDuration, anApplication, std::cout);
    }
    if (logFileMethod == 1) {
        std::string logFilePath = configObj.getLogFilePath();
        std::ofstream outToFile(logFilePath.c_str());

        outputDisplay(aDuration, anApplication, outToFile);
        
        outToFile.close();
    }
    if (logFileMethod == 2) {
        std::string logFilePath = configObj.getLogFilePath();
        std::ofstream outToFile(logFilePath.c_str());

        outputDisplay(aDuration, anApplication, std::cout);
        outputDisplay(aDuration, anApplication, outToFile);

        outToFile.close();
   }
    return 0;
}


/**
 * @brief      Outputs the process information to the ostream object.
 *
 * @param[in]  (chrono duration) aDuration  Duration from 0 to simulation launch 
 * @param[in]  (OSprocessRunner&) anApplication   An OSprocessRunner object that holds
 *                                           the information about the operations
 *                                           executed.
 * @param[in]  (std::ostream&) out  An ostream reference object used to output
 *                                  the data.
 */
void outputDisplay(std::chrono::duration<double> aDuration,
                   OSprocessRunner& anApplication,
                   std::ostream& out) {

    // Output the starting time
    out << std::fixed << std::showpoint << std::setprecision(6) 
        << aDuration.count() << " - " << "Simulator program starting" << std::endl;

    // Output all of the operations in the OSprocessRunner object
    for (int index = 0; index < anApplication.getNumOperations(); index++) {
        out << std::fixed << std::showpoint << std::setprecision(6) 
            << anApplication.getTimeStamp(index) << " - "
            << anApplication.getActor(index) << ": "
            << anApplication.getOpDescription(index) << std::endl;
    }
}