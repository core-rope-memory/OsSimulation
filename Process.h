/** 
 *  @file    Process.h
 *  @author  Ian Vanderhoff
 *  @date    5/2/2018 
 *  @version 2
 *  
 */

#ifndef _PROCESS
#define _PROCESS

#include <vector>

#include "MetaCommand.h"


/**
 * @brief      A class to store and retrieve information about a 
 *             process as well as the operations (MetaCommand objects) 
 *             of that process.
 */
class Process
{
public:
    // PCB enum for state of process control block
    enum PCB {START, READY, RUNNING, WAITING, EXIT};

    // GETTER FUNCTIONS
    int getNumberOfCommands() const;
    int getNumberOfIOops() const;
    int getOpIndex() const;
    MetaCommand getMetaCommand(int index) const;
    int getPCBstate() const;
    int getProcessTimeRemain() const;
    int getProcessNumber() const;

    // SETTER FUNCTIONS
    void insertCommand(MetaCommand cmdObj);
    void setNumIOops(int numOps);
    void setProcessNumber(int pNumber);
    void incrementOpIndex();
    void setPCBstate(PCB state);
    void addProcessTimeRemain(int time);
    void subtractProcessTimeRemain(int time);

    // Default Constructor
    Process() {
        numIOoperations_ = 0;
        operationIndex_ = 0;
        processTimeRemaining_ = 0;
    }

private:
    // PRIVATE DATA
    std::vector<MetaCommand> metaCmdVect_;
    int numIOoperations_;
    int operationIndex_;
    PCB state_;
    int processTimeRemaining_;
    int processNumber_;
};

//******************************************************************************
// IMPLEMENTATION
//******************************************************************************

int Process::getProcessNumber() const {
    return processNumber_;
}

/**
 * @brief      Gets the number of opeartions in the process.
 *
 * @return     (int) The number of operations.
 */
int Process::getNumberOfCommands() const {
    return metaCmdVect_.size();
}


/**
 * @brief      Gets the number of I/O operations in the process.
 *
 * @return     (int) The number of I/O operations.
 */
int Process::getNumberOfIOops() const {
    return numIOoperations_;
}


/**
 * @brief      Gets the current operation index (for context switching).
 *
 * @return     (int) The operation index.
 */
int Process::getOpIndex() const {
    return operationIndex_;
}

/**
 * @brief      Get the MetaCommand object holding the information about the command.
 *
 * @param[in]  index  (int) The index of the object in the vector
 *
 * @return     (MetaCommand) The MetaCommand object holding the information about the command
 */
MetaCommand Process::getMetaCommand(int index) const {
    MetaCommand tempMetaCmd = metaCmdVect_.at(index); // at(i) throws exception if i out-of-bounds

    return tempMetaCmd;
}

int Process::getPCBstate() const {
    return state_;
}

int Process::getProcessTimeRemain() const {
    return processTimeRemaining_;
}

void Process::setProcessNumber(int pNumber) {
    processNumber_ = pNumber;
}

/**
 * @brief      Inserts a MetaCommand object into the rear of the MetaCommand vector.
 *
 * @param[in]  cmdObj (MetaCommand) The MetaCommand object.
 */
void Process::insertCommand(MetaCommand cmdObj) {
    // push_back MetaCommand object onto the vector.
    metaCmdVect_.push_back(cmdObj);
}


/**
 * @brief      Sets the number I/O operations in the process.
 *
 * @param[in]  numOps (int) The number of I/O operations.
 */
void Process::setNumIOops(int numOps) {
    numIOoperations_ = numOps;
}

void Process::incrementOpIndex() {
    operationIndex_++;
}

void Process::setPCBstate(PCB state) {
    state_ = state;
}

void Process::addProcessTimeRemain(int time) {
    processTimeRemaining_ += time;
}

void Process::subtractProcessTimeRemain(int time) {
    processTimeRemaining_ -= time;
}

#endif
