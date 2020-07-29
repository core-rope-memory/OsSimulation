/** 
 *  @file    ReadyQueue.h
 *  @author  Ian Vanderhoff
 *  @date    5/2/2018 
 *  @version 1
 *  
 */

#ifndef _READY_QUEUE
#define _READY_QUEUE

#include <vector>

#include "Process.h"


/**
 * @brief      A class to store the processes to be executed.
 * 
 * Information about the processes in the queue can be retrieved. Queue
 * can be sorted based on scheduling algorithm used.
 * 
 */
class ReadyQueue
{
public:
    // GETTER FUNCTIONS
    MetaCommand getMetaCommand(int processIndex, int cmdIndex) const;
    int getNumberOfProcesses() const;
    int getNumProcessCmds(int processIndex) const;
    int getNumIOCmds(int processIndex) const;
    int getProcessArrivalIndx() const;

    // SETTER FUNCTIONS
    void insertProcess(Process aProcess);
    Process removeProcess();
    void setSchedAlgorithm(int schedCode);
    void sortReadyQueue();
    void incrementProcessArrivalIndx();

    // Default constructor
    ReadyQueue() {
        processArrivalIndx_ = 1;
    }

private:
    std::vector<Process> processVect_; // vector of processes
    int schedCode_; // 0 is RR, 1 is STR
    int processArrivalIndx_; // for the process index
};

//******************************************************************************
// IMPLEMENTATION
//******************************************************************************


/**
 * @brief      Get the MetaCommand object holding the information about the command.
 * 
 * The MetaCommand object of the process is retrieved from its process object.
 *
 * @param[in]  processIndex  (int) The index of the Process object
 * @param[in]  cmdIndex  (int) The index of MetaCommand object in the process
 *
 * @return     (MetaCommand) The MetaCommand object holding the information about the command
 */
MetaCommand ReadyQueue::getMetaCommand(int processIndex, int cmdIndex) const {
    Process tempProcessObj = processVect_.at(processIndex); // at(i) throws exception if i out-of-bounds
    MetaCommand tempMetaCmd = tempProcessObj.getMetaCommand(cmdIndex);

    return tempMetaCmd;
}

/**
 * @brief      Gets the number of processes stored in the process vector.
 *
 * @return     (int) The number of processes.
 */
int ReadyQueue::getNumberOfProcesses() const {
    return processVect_.size();
}

/**
 * @brief      Gets the number of operations (MetaCommand objects) stored in the process.
 *
 * @param[in]  processIndex (int)  The process index
 *
 * @return     (int) The number operations (MetaCommand objects) in the process.
 */
int ReadyQueue::getNumProcessCmds(int processIndex) const {
    return processVect_[processIndex].getNumberOfCommands();
}

/**
 * @brief      Gets the number I/O operations stored in the process.
 *
 * @param[in]  processIndex (int) The process index
 *
 * @return     (int) The number of I/O operations.
 */
int ReadyQueue::getNumIOCmds(int processIndex) const {
    return processVect_[processIndex].getNumberOfIOops();
}


/**
 * @brief      Gets the process arrival index.
 *
 * @return     The process arrival index.
 */
int ReadyQueue::getProcessArrivalIndx() const {
    return processArrivalIndx_;
}


/**
 * @brief      Increments the process arrival index.
 */
void ReadyQueue::incrementProcessArrivalIndx() {
    processArrivalIndx_++;
}


/**
 * @brief      Inserts a process into the back of the queue.
 *
 * @param[in]  aProcess  (Process) A process
 */
void ReadyQueue::insertProcess(Process aProcess) {
    processVect_.push_back(aProcess);
}


/**
 * @brief      Removes a process from the front of the ready queue.
 *
 * @return     (Process) A process.
 */
Process ReadyQueue::removeProcess() {
    // temp process object
    Process tempProcessObj;

    // Create iterator to first process in ready queue
    std::vector<Process>::iterator beginIter = processVect_.begin();

    // copy first process into temp by assigning dereferenced iterator
    tempProcessObj = *beginIter;

    // Delete the first process from the ready queue
    processVect_.erase(beginIter);

    // return the process object
    return tempProcessObj;
}


/**
 * @brief      Sets the sched algorithm.
 *
 * @param[in]  schedCode  (int) The code for the scheduling algorithm.
 */
void ReadyQueue::setSchedAlgorithm(int schedCode) {
    schedCode_ = schedCode;
}


/**
 * @brief      Sorts the ready queue by shortest time remaining in the processes.
 */
void ReadyQueue::sortReadyQueue() {
    // Sort depending on scheduling algorithm
    if (schedCode_ == 0) { // RR
        // no sorting necessary
    }
    else if (schedCode_ == 1) { // STR

        // STR: processes are executed in ascending order by the 
        // time remaining in the processes

        int numProceses = processVect_.size();

        // Selection sort in ascending order by time remaining in process
        int startScan, minIndex;

        Process tempMinObj;

        for (startScan = 0; startScan < (numProceses - 1); startScan++) {
            minIndex = startScan;
            tempMinObj = processVect_[startScan];
            
            for(int i = (startScan + 1); i < numProceses; i++) {
                if (processVect_[i].getProcessTimeRemain() < tempMinObj.getProcessTimeRemain()) {
                    tempMinObj = processVect_[i];
                    minIndex = i;
                }
            }
            processVect_[minIndex] = processVect_[startScan];
            processVect_[startScan] = tempMinObj;
        }
    }
}

#endif