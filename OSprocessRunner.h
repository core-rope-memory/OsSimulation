/** 
 *  @file    OSprocessRunner.h
 *  @author  Ian Vanderhoff
 *  @date    4/20/2018 
 *  @version 4
 */

#ifndef _OS_PROCESS_RUNNER
#define _OS_PROCESS_RUNNER

#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <vector>
#include <deque>
#include <pthread.h>
#include <semaphore.h>

#include "MetaData.h"
// ReadyQueue.h included with MetaData.h
// Configuration.h included with MetaData.h
// CycleTime.h included with Configuration.h
// Process.h included with ReadyQueue.h
// MetaCommand.h included with Process.h

/**
 * @brief      Class for executing multiple processes in an application, 
 *             storing their information, and returning the information 
 *             to the user.
 */ 
class OSprocessRunner
{
public:
    // Struct for operation output
    struct OperationInfo {
        double timeStamp;
        std::string actor;
        std::string opDescription;
    };

    // OVERLOADED CONSTRUCTOR
    OSprocessRunner(std::chrono::steady_clock::time_point begin,
                    Configuration configObj) {
        begin_ = begin;
        configObj_ = configObj;
        systemMemorySize_ = configObj.getSystemMemory();
        memoryBlockSize_ = configObj.getBlockSize();
        schedCode_ = configObj.getSchedCode();
        processCyclesExecuted_ = 0;
        quantumNumber_ = configObj_.getPQN();
        firstOperation_ = true;
        // set interrupt flags for sched alg.'s to false
        rrFlag_ = false;
        strFlag_ = false;
        waitForOP_ = true;

        // Set scheduling algorithm in ready queue
        readyQ_.setSchedAlgorithm(schedCode_);
    }

    // STATIC DECLARATIONS FOR MUTEX AND SEMAPHORES
    static pthread_mutex_t mutex1;
    static pthread_mutex_t rdyQLock;
    static pthread_mutex_t rrFlagLock;
    static pthread_mutex_t waitForOP;
    static pthread_mutex_t strFlagLock;

    static sem_t hardDriveSemaphore;
    static sem_t keyboardSemaphore;
    static sem_t scannerSemaphore;
    static sem_t monitorSemaphore;
    static sem_t projectorSemaphore;

    // STATIC DECLARATIONS FOR INTS TO HOLD NUMBER OF I/0 RESOURCES
    static int hardDriveResources, 
               keyboardResources, 
               scannerResources, 
               monitorResources, 
               projectorResources;

    // STATIC DECLARATIONS OF DEQUES TO HOLD SYSTEM RESOURCES
    static std::deque<std::string> hardDriveDeque;
    static std::deque<std::string> keyboardDeque;
    static std::deque<std::string> scannerDeque;
    static std::deque<std::string> monitorDeque;
    static std::deque<std::string> projectorDeque;

    // GETTER FUNCTIONS
    double getTimeStamp(const int index);
    std::string getActor(const int index);
    std::string getOpDescription(const int index);
    int getNumOperations();

    // PUBLIC FUNCTIONS
    void importMDF();
    void runProcesses();
    static void fillResourceDeques(const int, const int, const int, const int, const int);

private:
    // PRIVATE DATA
    ReadyQueue readyQ_; // ready queue of processes
    Configuration configObj_;
    bool timerComplete_; // flag for polling
    bool ioComplete_; // flag for polling
    bool rrFlag_; // flag for round robin interrupt after 50 ms; set in seperate thread
    bool waitForOP_; // used in quantum countdown thread.
    bool firstOperation_; // starts the quantum countdown thread on first operation.
    bool strFlag_; // flag for shortest time remaining; set after new processes added to ready queue
    int countDownTime_;
    int ioTime_;
    std::vector<OperationInfo> operationsVect_; // info structs for log output
    std::chrono::steady_clock::time_point begin_; // start time
    int systemMemorySize_;
    int memoryBlockSize_;
    long memAddress_;
    int schedCode_; // 0 for Round Robin, 1 for Shortest Time Remaining
    int processesInRdyQ_; // # processes in ready queue (avoids having to lock in while loop test)
    int processCyclesExecuted_; // number of cycles current process has executed since last interrupt
    int quantumNumber_; // Round Robin quantum time slice in milliseconds

    // PRIVATE FUNCTIONS
    void executeOperation(const MetaCommand metaCmdObj, int processIndex);
    void countDownThreadFunction();
    static void* countDownThreadHelper(void *obj);
    void countDown(int time);
    void quantumTimerThreadFunction();
    static void* quantumTimerHelper(void *obj);
    void quantumTimer();
    void loadMDFThreadFunction();
    static void* loadMDFHelper(void *obj);
    void loadMDF();
    void ioThreadFunction();
    static void* ioThreadHelper(void *obj);
    void inputOutput(int time, std::string aDescriptor, std::string& resource);
    void logInterruption(int processIndex);
};

//******************************************************************************
// IMPLEMENTATION
//******************************************************************************

/* Unlike regular class attributes, static class attributes are only declared in the
 * class header, they are not defined. They need to be defined in the implementation
 * file or section in order to avoid linker errors. Below are the definitions of the
 * static attributes.
 */

// STATIC DEFINITIONS FOR MUTEX AND SEMAPHORES
pthread_mutex_t OSprocessRunner::mutex1;
pthread_mutex_t OSprocessRunner::rdyQLock;
pthread_mutex_t OSprocessRunner::rrFlagLock;
pthread_mutex_t OSprocessRunner::waitForOP;
pthread_mutex_t OSprocessRunner::strFlagLock;

sem_t OSprocessRunner::hardDriveSemaphore;
sem_t OSprocessRunner::keyboardSemaphore;
sem_t OSprocessRunner::scannerSemaphore;
sem_t OSprocessRunner::monitorSemaphore;
sem_t OSprocessRunner::projectorSemaphore;

// STATIC DEFINITIONS FOR INTS TO HOLD NUMBER OF I/0 RESOURCES
int OSprocessRunner::hardDriveResources, 
           OSprocessRunner::keyboardResources, 
           OSprocessRunner::scannerResources, 
           OSprocessRunner::monitorResources, 
           OSprocessRunner::projectorResources;

// STATIC DEFINITIONS OF DEQUES TO HOLD RESOURCES
std::deque<std::string> OSprocessRunner::hardDriveDeque;
std::deque<std::string> OSprocessRunner::keyboardDeque;
std::deque<std::string> OSprocessRunner::scannerDeque;
std::deque<std::string> OSprocessRunner::monitorDeque;
std::deque<std::string> OSprocessRunner::projectorDeque;


/**
 * @brief      Imports the MDF
 * 
 * This function is called by the thread function loadMDFThreadFunction()
 * 
 */
void OSprocessRunner::importMDF() {
    // Create MetaData object
    MetaData metaDataObj;

    // Pass configuration object into MetaData object
    // for cycle time calculations
    metaDataObj.setConfigurationObject(configObj_);

    // Parse the meta data file with the MetaData object
    metaDataObj.parseMDF(configObj_.getMdfFilePath(), readyQ_);

    // get the size of ready queue while locked
    processesInRdyQ_ = readyQ_.getNumberOfProcesses();
}

/**
 * @brief      Public function that runs multiple processes in an application
 * 
 * This function determines which of the two scheduling algorithms are being used.
 * Round robin or shortest time remaining.
 *
 */
void OSprocessRunner::runProcesses() {

    // Initialize the memory allocation address to zero
    memAddress_ = 0;

    // Get ready queue mutex lock
    pthread_mutex_lock(&rdyQLock);

    // Import meta data file initially
    importMDF(); // this uses the ready queue/needs to be locked

    // if STR sort ready queue
    if (schedCode_ == 1)
        readyQ_.sortReadyQueue();

    // Release ready queue mutex lock
    pthread_mutex_unlock(&rdyQLock);

    // Set up load thread to load the metadata file into ready queue every 100 ms    
    loadMDF();

    while (processesInRdyQ_ > 0) {

        // Get the ready queue mutex lock
        pthread_mutex_lock(&rdyQLock);
        
        // remove process from front of ready queue
        Process currentProcess = readyQ_.removeProcess();

        // update processesInRdyQ_
        processesInRdyQ_ = readyQ_.getNumberOfProcesses();

        // Release ready queue mutex lock
        pthread_mutex_unlock(&rdyQLock);

        // loop while there are operations left in the process
        while (currentProcess.getOpIndex() < currentProcess.getNumberOfCommands()) {
            // Check for interrupt conditions
            if (schedCode_ == 0) { // Round Robin

                // Start quantum timer if first operation
                if (firstOperation_) {
                    firstOperation_ = false;
                    quantumTimer();
                }

                // Get the rrFlagLock mutex lock
                pthread_mutex_lock(&rrFlagLock);

                // check RR timer interrupt flag
                if (rrFlag_ == true) {

                    // reset the flag to false
                    rrFlag_ = false;

                    // Get the ready queue mutex lock
                    pthread_mutex_lock(&rdyQLock);

                    // push process onto back of ready queue
                    readyQ_.insertProcess(currentProcess);

                    // Release ready queue mutex lock
                    pthread_mutex_unlock(&rdyQLock);

                    logInterruption(currentProcess.getProcessNumber());

                    // Release rrFlagLock mutex lock
                    pthread_mutex_unlock(&rrFlagLock);

                    // break out of while loop
                    break;
                }

                // Release rrFlagLock mutex lock (outside of if statement)
                pthread_mutex_unlock(&rrFlagLock);

                // get waitForOP mutex lock
                pthread_mutex_lock(&waitForOP);

                // set flag
                waitForOP_ = false;

                // release waitForOP mutex lock
                pthread_mutex_unlock(&waitForOP);

                // execute operation
                executeOperation(currentProcess.getMetaCommand(currentProcess.getOpIndex()),
                                 currentProcess.getProcessNumber());

                // increment the current operation index of the process
                currentProcess.incrementOpIndex();
            } 

            else if (schedCode_ == 1) { // Shortest Time Remaining
                // Get the strFlagLock mutex lock
                pthread_mutex_lock(&strFlagLock);

                // check STR timer interrupt flag
                if (strFlag_ == true) {

                    // reset the flag to false
                    strFlag_ = false;

                    // Get the ready queue mutex lock
                    pthread_mutex_lock(&rdyQLock);

                    // push process onto back of ready queue
                    readyQ_.insertProcess(currentProcess);

                    // sort the ready quque
                    readyQ_.sortReadyQueue();

                    // Release ready queue mutex lock
                    pthread_mutex_unlock(&rdyQLock);

                    logInterruption(currentProcess.getProcessNumber());

                    // Release strFlagLock mutex lock
                    pthread_mutex_unlock(&strFlagLock);

                    // break out of while loop
                    break;
                }

                // Release strFlagLock mutex lock (outside of if statement)
                pthread_mutex_unlock(&strFlagLock);

                // execute operation
                executeOperation(currentProcess.getMetaCommand(currentProcess.getOpIndex()),
                                 currentProcess.getProcessNumber());

                // update time remaining in process by subtracting time for executed operation
                MetaCommand tempMetaCmdObj;
                tempMetaCmdObj = currentProcess.getMetaCommand(currentProcess.getOpIndex());
                int timeForOP = tempMetaCmdObj.getTime();
                currentProcess.subtractProcessTimeRemain(timeForOP);

                // increment the current operation index of the process
                currentProcess.incrementOpIndex();
            }
        }

        // Get ready queue mutex lock
        pthread_mutex_lock(&rdyQLock);

        // get updated number of processes in ready queue
        processesInRdyQ_ = readyQ_.getNumberOfProcesses();

        // Release ready queue mutex lock
        pthread_mutex_unlock(&rdyQLock);
    }    
}

/**
 * @brief      Executes a single MetaCommand object as a process operation.
 * 
 * This function determines which type of operation is being executed, stores
 * a timestamp of the operation, sets the actor performing
 * the operation, sets the operation description, and calls thread creating functions
 * to handle I/O and time based operations.
 *
 * @param[in]  (MetaCommand) metaCmdObj  The meta command object
 * @param[in]  (int) processIndex        The number of the process
 */

void OSprocessRunner::executeOperation(const MetaCommand metaCmdObj, int processIndex) {
    // get code
    char aCode = metaCmdObj.getCode();

    // get descriptor
    std::string aDescriptor = metaCmdObj.getDescriptor();

    // flag for timer thread function initialized to false
    timerComplete_ = false;

    switch (aCode) {
        case 'A':
        {
            // if starting operation
            if (aDescriptor == "begin") {
                // Create OperationInfo struct for preparing
                OperationInfo opAbeginInfo1;

                // timestamp preparing
                auto opAbeginTime1 = std::chrono::steady_clock::now();
                std::chrono::duration<double> aDuration1;
                aDuration1 = (opAbeginTime1 - begin_);
                opAbeginInfo1.timeStamp = aDuration1.count();

                // Assign actor
                opAbeginInfo1.actor = "OS";

                // Assign description
                opAbeginInfo1.opDescription = "preparing process " + std::to_string(processIndex);

                // Push operation information onto vector
                operationsVect_.push_back(opAbeginInfo1);

                // Create OperationInfo struct for start
                OperationInfo opAbeginInfo2;

                // timestamp start
                auto opAbeginTime2 = std::chrono::steady_clock::now();
                std::chrono::duration<double> aDuration2;
                aDuration2 = (opAbeginTime2 - begin_);
                opAbeginInfo2.timeStamp = aDuration2.count();

                // Assign actor
                opAbeginInfo2.actor = "OS";

                // Assign description
                opAbeginInfo2.opDescription = "starting process " + std::to_string(processIndex);

                // Push operation information onto vector
                operationsVect_.push_back(opAbeginInfo2);
            }

            // if finishing operation
            else if (aDescriptor == "finish") {
                // Create OperationInfo struct for end
                OperationInfo opAendInfo;

                // timestamp end
                auto opAendTime = std::chrono::steady_clock::now();
                std::chrono::duration<double> aDuration;
                aDuration = (opAendTime - begin_);
                opAendInfo.timeStamp = aDuration.count();

                // Assign actor
                opAendInfo.actor = "OS";

                // Assign description
                opAendInfo.opDescription = "End process " + std::to_string(processIndex);

                // Push operation information onto vector
                operationsVect_.push_back(opAendInfo);
            }
            break;
        }
        case 'P':
        case 'M':
        {
            // Create OperationInfo struct for start
            OperationInfo opPMstartInfo;

            // timestamp start
            auto opPMstartTime = std::chrono::steady_clock::now();
            std::chrono::duration<double> startDuration;
            startDuration = (opPMstartTime - begin_);
            opPMstartInfo.timeStamp = startDuration.count();

            // Assign actor
            opPMstartInfo.actor = "Process " + std::to_string(processIndex);

            // assign description
            if (aDescriptor == "run") {
                opPMstartInfo.opDescription = "start processing action";
            }
            else if (aDescriptor == "block") {
                opPMstartInfo.opDescription = "start memory blocking";
            }
            else if (aDescriptor == "allocate") {
                opPMstartInfo.opDescription = "allocating memory";
            }

            // Push operation onto vector
            operationsVect_.push_back(opPMstartInfo);

            // Open a coundown timer thread and wait for it to
            // finish counting down
            countDown(metaCmdObj.getTime());

            // Create OperationInfo struct for end
            OperationInfo opPMendInfo;

            // timestamp end
            auto opPMendTime = std::chrono::steady_clock::now();
            std::chrono::duration<double> endDuration;
            endDuration = (opPMendTime - begin_);
            opPMendInfo.timeStamp = endDuration.count();

            // Assign actor
            opPMendInfo.actor = "Process " + std::to_string(processIndex);

            if (aDescriptor == "run") {
                // Assign description
                opPMendInfo.opDescription = "end processing action";
            }
            else if (aDescriptor == "block") {
                // Assign description
                opPMendInfo.opDescription = "end memory blocking";
            }
            else if (aDescriptor == "allocate") {
                // assign the starting memory address for the block
                std::stringstream stream;
                stream << "0x" << std::hex << std::setw(8) << std::setfill('0') << memAddress_;
                std::string hexAddress = stream.str();

                // Assign description
                opPMendInfo.opDescription = "memory allocated at " + hexAddress;

                // check to see if the next block will overflow the bounds
                // of the system memory. If so, reset to zero
                if ((memAddress_ + memoryBlockSize_) > systemMemorySize_) {
                    memAddress_ = 0;
                }
                else {
                    // Set the address for the next memory allocation
                    memAddress_ += memoryBlockSize_;
                }
            }

            // Push operation information onto vector
            operationsVect_.push_back(opPMendInfo);

            break;
        }
        case 'I':
        case 'O':
        {
            // Create start struct
            OperationInfo opIOstartInfo;

            // Create end struct
            OperationInfo opIOendInfo;

            // Create string for resource
            std::string resource;

            // timestamp start
            auto opIOstartTime = std::chrono::steady_clock::now();
            std::chrono::duration<double> startDuration;
            startDuration = (opIOstartTime - begin_);
            opIOstartInfo.timeStamp = startDuration.count();

            // Open a IO thread and wait for it to finish
            inputOutput(metaCmdObj.getTime(), aDescriptor, resource);

            // timestamp end
            auto opIOendTime = std::chrono::steady_clock::now();
            std::chrono::duration<double> endDuration;
            endDuration = (opIOendTime - begin_);
            opIOendInfo.timeStamp = endDuration.count();

            // Assign actor and description for operation start
            opIOstartInfo.actor = "Process " + std::to_string(processIndex);

            if ((aDescriptor == "hard drive") && (aCode == 'I')) {
                opIOstartInfo.opDescription = "start hard drive input on " + resource;
            }
            else if ((aDescriptor == "hard drive") && (aCode == 'O')) {
                opIOstartInfo.opDescription = "start hard drive output on " + resource;
            }
            else if (aDescriptor == "keyboard") {
                opIOstartInfo.opDescription = "start keyboard input on " + resource;
            }
            else if (aDescriptor == "scanner") {
                opIOstartInfo.opDescription = "start scanner input on " + resource;
            }
            else if (aDescriptor == "monitor") {
                opIOstartInfo.opDescription = "start monitor output on " + resource;
            }
            else if (aDescriptor == "projector") {
                opIOstartInfo.opDescription = "start projector output on " + resource;
            }

            // Push operation start information onto vector
            operationsVect_.push_back(opIOstartInfo);

            // Assign actor and description for operation end
            opIOendInfo.actor = "Process " + std::to_string(processIndex);

            if ((aDescriptor == "hard drive") && (aCode == 'I')) {
                opIOendInfo.opDescription = "end hard drive input";
            }
            else if ((aDescriptor == "hard drive") && (aCode == 'O')) {
                opIOendInfo.opDescription = "end hard drive output";
            }
            else if (aDescriptor == "keyboard") {
                opIOendInfo.opDescription = "end keyboard input";
            }
            else if (aDescriptor == "scanner") {
                opIOendInfo.opDescription = "end scanner input";
            }
            else if (aDescriptor == "monitor") {
                opIOendInfo.opDescription = "end monitor output";
            }
            else if (aDescriptor == "projector") {
                opIOendInfo.opDescription = "end projector output";
            }

            // Push operation end intormation onto vector
            operationsVect_.push_back(opIOendInfo);

            break;
        }
    }
}


/**
 * @brief      Logs an interruption to the process.
 * 
 * The log is added to the OperationInfo vector.
 *
 * @param[in]  (int) processIndex  The process number
 */

void OSprocessRunner::logInterruption(int processIndex) {
    // Create OperationInfo struct for start
    OperationInfo opInterruptInfo;

    // timestamp start
    auto opInterruptstartTime = std::chrono::steady_clock::now();
    std::chrono::duration<double> startDuration;
    startDuration = (opInterruptstartTime - begin_);
    opInterruptInfo.timeStamp = startDuration.count();

    // Assign actor
    opInterruptInfo.actor = "Process " + std::to_string(processIndex);

    if (schedCode_ == 0) {
        // assign description
        opInterruptInfo.opDescription = "Process interrupted by round robin scheduling algorithm.";
    } else if (schedCode_ == 1) {
        // assign description
        opInterruptInfo.opDescription = "Process interrupted by STR scheduling algorithm.";
    }

    // Push operation onto vector
    operationsVect_.push_back(opInterruptInfo);
}

/**
 * @brief      A function that creates the I/O thread to handle
 *             an I/O event.
 *
 * @param[in]  (int) time  The total time taken by the I/O process 
 *                         in milliseconds.
 * @param[in]  (std::string) aDescriptor  Descriptor of operation
 * @param[in]  (std::string&) resource  The resource used for operation 
 */
void OSprocessRunner::inputOutput(int time, std::string aDescriptor, std::string& resource) {
    // Get IO time
    ioTime_ = time;

    // flag for i/o pperation completion initialized to false
    ioComplete_ = false;

    /*
     * Call semaphore wait function for resource.
     * When resource becomes available, lock the mutex protecting the resources.
     * Copy the resource value and pop it from front of deque.
     */
    if (aDescriptor == "hard drive") {
        sem_wait(&hardDriveSemaphore);
        pthread_mutex_lock(&mutex1);
        resource = hardDriveDeque.front();
        hardDriveDeque.pop_front();
    }
    else if (aDescriptor == "keyboard") {
        sem_wait(&keyboardSemaphore);
        pthread_mutex_lock(&mutex1);
        resource = keyboardDeque.front();
        keyboardDeque.pop_front();
    }
    else if (aDescriptor == "scanner") {
        sem_wait(&scannerSemaphore);
        pthread_mutex_lock(&mutex1);
        resource = scannerDeque.front();
        scannerDeque.pop_front(); 
    }
    else if (aDescriptor == "monitor") {
        sem_wait(&monitorSemaphore);
        pthread_mutex_lock(&mutex1);
        resource = monitorDeque.front();
        monitorDeque.pop_front(); 
    }
    else if (aDescriptor == "projector") {
        sem_wait(&projectorSemaphore);
        pthread_mutex_lock(&mutex1);
        resource = projectorDeque.front();
        projectorDeque.pop_front(); 
    }

    // unlock the mutex
    pthread_mutex_unlock(&mutex1);

    // Call IO thread
    pthread_t tid; /* the thread identifier */
    pthread_attr_t attr; /* set of thread attributes */
    /* get the default attributes */
    pthread_attr_init(&attr);
    /* create the thread with the static helper function */
    pthread_create(&tid, &attr, &ioThreadHelper, this);

    // // Change PCB state
    // state_ = WAITING;

    // Wait for IO thread to finish by polling shared
    // variable ioComplete_
    while(!ioComplete_) {}

    /*
     * Call semaphore post to signal that a resource has become available.
     * Lock the mutex protecting the resources.
     * Push the resource onto the back of the deque.
     */
    if (aDescriptor == "hard drive") {
        sem_post(&hardDriveSemaphore);
        pthread_mutex_lock(&mutex1);
        hardDriveDeque.push_back(resource);
    }
    else if (aDescriptor == "keyboard") {
        sem_post(&keyboardSemaphore);
        pthread_mutex_lock(&mutex1);
        keyboardDeque.push_back(resource);
    }
    else if (aDescriptor == "scanner") {
        sem_post(&scannerSemaphore);
        pthread_mutex_lock(&mutex1);
        scannerDeque.push_back(resource); 
    }
    else if (aDescriptor == "monitor") {
        sem_post(&monitorSemaphore);
        pthread_mutex_lock(&mutex1);
        monitorDeque.push_back(resource); 
    }
    else if (aDescriptor == "projector") {
        sem_post(&projectorSemaphore);
        pthread_mutex_lock(&mutex1);
        projectorDeque.push_back(resource); 
    }

    // unlock the mutex
    pthread_mutex_unlock(&mutex1);

    // // Change PCB state
    // state_ = READY;
}

/**
 * @brief      Helper function for I/O thread function.
 * 
 * Pthread.h was created for C not C++ so it has trouble accepting class
 * member functions. It cannot handle the 'this' pointer. This function
 * uses a static member function (which does not have a 'this' pointer)
 * to call the non-static member function used in the thread.
 *
 * @param      (void*) obj   Pointer to the OSprocessRunner class calling
 *                           the function.
 *
 * @return     (void*)
 */
void* OSprocessRunner::ioThreadHelper(void *obj) {
    OSprocessRunner *myObj = reinterpret_cast<OSprocessRunner *>(obj);
    myObj->ioThreadFunction();
    return myObj;
}

/** 
 * @brief      I/O thread function.
 * 
 * This function uses a thread to mimic an I/O process that is out
 * of the scope of the process calling it.
 */
void OSprocessRunner::ioThreadFunction() {
    // Get countdown time
    countDownTime_ = ioTime_;

    // Call countdown thread
    pthread_t tid; /* the thread identifier */
    pthread_attr_t attr; /* set of thread attributes */
    /* get the default attributes */
    pthread_attr_init(&attr);
    /* create the thread using static helper function */
    pthread_create(&tid, &attr, &countDownThreadHelper, this);

    // Wait for countdown thread to finish by polling shared
    // variable timerComplete_
    while(!timerComplete_) {}

    ioComplete_ = true;
}

/**
 * @brief      A function that creates a timer thread to countdown the 
 *             number of milliseconds that an operation will take
 * 
 *
 * @param[in]  (int) time  The total time of the cycles in milliseconds.
 */
void OSprocessRunner::countDown(int time) {
    // Get countdown time
    countDownTime_ = time;

    timerComplete_ = false;

    // Call countdown thread
    pthread_t tid; /* the thread identifier */
    pthread_attr_t attr; /* set of thread attributes */
    /* get the default attributes */
    pthread_attr_init(&attr);
    /* create the thread using the static helpera function */
    pthread_create(&tid, &attr, &countDownThreadHelper, this);

    // Wait for countdown thread to finish by polling shared
    // variable timerComplete_
    while(!timerComplete_) {}
}

/**
 * @brief      Helper function for countDown thread function.
 * 
 * Pthread.h was created for C not C++ so it has trouble accepting class
 * member functions. It cannot handle the 'this' pointer. This function
 * uses a static member function (which does not have a 'this' pointer)
 * to call the non-static member function used in the thread.
 *
 * @param      (void*) obj   Pointer to the OSprocessRunner class calling
 *                           the function.
 *
 * @return     (void*)
 */
void* OSprocessRunner::countDownThreadHelper(void *obj) {
    OSprocessRunner *myObj = reinterpret_cast<OSprocessRunner *>(obj);
    myObj->countDownThreadFunction();
    return myObj;
}

/**
 * @brief      Thread function used to countdown until operation completion.
 */
void OSprocessRunner::countDownThreadFunction() {
    std::chrono::steady_clock::time_point tend;
    tend = std::chrono::steady_clock::now() + std::chrono::milliseconds(countDownTime_);
    while (std::chrono::steady_clock::now() < tend){}

    timerComplete_ = true;
}

/**
 * @brief      A function that creates a timer thread to for the quantum
 *             time slice used in the round robin sched. alg.
 */
void OSprocessRunner::quantumTimer() {
    // Call quantumTimer thread
    pthread_t tid; /* the thread identifier */
    pthread_attr_t attr; /* set of thread attributes */
    /* get the default attributes */
    pthread_attr_init(&attr);
    /* create the thread using the static helpera function */
    pthread_create(&tid, &attr, &quantumTimerHelper, this);
}

/**
 * @brief      Helper function for quantum timer thread function.
 * 
 * Pthread.h was created for C not C++ so it has trouble accepting class
 * member functions. It cannot handle the 'this' pointer. This function
 * uses a static member function (which does not have a 'this' pointer)
 * to call the non-static member function used in the thread.
 *
 * @param      (void*) obj   Pointer to the OSprocessRunner class calling
 *                           the function.
 *
 * @return     (void*)
 */
void* OSprocessRunner::quantumTimerHelper(void *obj) {
    OSprocessRunner *myObj = reinterpret_cast<OSprocessRunner *>(obj);
    myObj->quantumTimerThreadFunction();
    return myObj;
}


/**
 * @brief      Thread function to set the quantum timer flag for round robin
 * 
 * This function has a waiting flag to ensure that the timer does not start
 * until an operation starts executing. When the timer is up, the round robin
 * flag is set.
 * 
 */
void OSprocessRunner::quantumTimerThreadFunction() {
    while (true) {
        // wait till start of operation
        while(true) {
            // Get the waitForOP mutex lock
            pthread_mutex_lock(&waitForOP);

            // check flag
            if (!waitForOP_) {
                // reset flag
                waitForOP_ = true;
                // Release the waitForOP mutex lock
                pthread_mutex_unlock(&waitForOP);
                // escape while loop
                break;
            }

            // Release the waitForOP mutex lock
            pthread_mutex_unlock(&waitForOP);
        }

        std::chrono::steady_clock::time_point tend;
        tend = std::chrono::steady_clock::now() + std::chrono::milliseconds(quantumNumber_);
        while (std::chrono::steady_clock::now() < tend){}

        // Get the rrFlagLock mutex lock
        pthread_mutex_lock(&rrFlagLock);

        rrFlag_ = true;

        // Release the rrFlagLock mutex lock
        pthread_mutex_unlock(&rrFlagLock);
    }
}


/**
 * @brief      A function that creates a thread to load the MDF file
 *             every 100 ms for 10 times.
 *              
 */
void OSprocessRunner::loadMDF() {
    // Call loadMDF thread
    pthread_t tid; /* the thread identifier */
    pthread_attr_t attr; /* set of thread attributes */
    /* get the default attributes */
    pthread_attr_init(&attr);
    /* create the thread using the static helper function */
    pthread_create(&tid, &attr, &loadMDFHelper, this);
}

/**
 * @brief      Helper function for loadMDF thread function.
 * 
 * Pthread.h was created for C not C++ so it has trouble accepting class
 * member functions. It cannot handle the 'this' pointer. This function
 * uses a static member function (which does not have a 'this' pointer)
 * to call the non-static member function used in the thread.
 *
 * @param      (void*) obj   Pointer to the OSprocessRunner class calling
 *                           the function.
 *
 * @return     (void*)
 */
void* OSprocessRunner::loadMDFHelper(void *obj) {
    OSprocessRunner *myObj = reinterpret_cast<OSprocessRunner *>(obj);
    myObj->loadMDFThreadFunction();
    return myObj;
}


/**
 * @brief      Loads the meta data file multiple times.
 * 
 * This function loads the metadata file 10 times, once every 100 ms.
 * When the metadata file is loaded onto the ready queue, the STR interrupt
 * flag is set.
 * 
 */
void OSprocessRunner::loadMDFThreadFunction() {
    int count = 0;

    while (count < 10) {
        std::chrono::steady_clock::time_point tend;
        tend = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
        while (std::chrono::steady_clock::now() < tend){}

        // Get the rdyQLock mutex lock
        pthread_mutex_lock(&rdyQLock);

        importMDF();

        // Release the rdyQLock mutex lock
        pthread_mutex_unlock(&rdyQLock);

        // If STR 
        if (schedCode_ == 1) {
            // Get the strFlagLock mutex lock
            pthread_mutex_lock(&strFlagLock);

            // set strInterruptFlag
            strFlag_ = true;

            // release the strFlagLock mutex lock
            pthread_mutex_unlock(&strFlagLock);
        }
        count++;
    }
}


/**
 * @brief      Static function used to assign the number of system resources allocated
 *             to each i/o operation. The number of resources is optained from the
 *             configuration file. This function is static because number of resources
 *             will be the same for all processes, and it is initialized before an
 *             OSprocessRunner object is created.
 *
 * @param[in]  rsc0  The rsc 0
 * @param[in]  rsc1  The rsc 1
 * @param[in]  rsc2  The rsc 2
 * @param[in]  rsc3  The rsc 3
 * @param[in]  rsc4  The rsc 4
 */
void OSprocessRunner::fillResourceDeques(const int rsc0, const int rsc1, const int rsc2, const int rsc3, const int rsc4) {
    // Initialize the static I/O mutex in OSprcessRunner class
    mutex1 = PTHREAD_MUTEX_INITIALIZER;

    // Initialize the static I/O semaphores in OSprcessRunner class
    sem_init(&hardDriveSemaphore, 0, rsc0);
    sem_init(&keyboardSemaphore, 0, rsc1);
    sem_init(&scannerSemaphore, 0, rsc2);
    sem_init(&monitorSemaphore, 0, rsc3);
    sem_init(&projectorSemaphore, 0, rsc4);

    // get the number of resources
    sem_getvalue(&hardDriveSemaphore, &hardDriveResources);
    sem_getvalue(&keyboardSemaphore, &keyboardResources);
    sem_getvalue(&scannerSemaphore, &scannerResources);
    sem_getvalue(&monitorSemaphore, &monitorResources);
    sem_getvalue(&projectorSemaphore, &projectorResources);

    // Create the deques for each resource type
    for (int i = 0; i < hardDriveResources; i++) {
        hardDriveDeque.push_back("HDD_" + std::to_string(i));
    }
    for (int i = 0; i < keyboardResources; i++) {
        keyboardDeque.push_back("KBRD_" + std::to_string(i));
    }
    for (int i = 0; i < scannerResources; i++) {
        scannerDeque.push_back("SCNR_" + std::to_string(i));
    }
    for (int i = 0; i < monitorResources; i++) {
        monitorDeque.push_back("MNTR_" + std::to_string(i));
    }
    for (int i = 0; i < projectorResources; i++) {
        projectorDeque.push_back("PROJ_" + std::to_string(i));
    }
}

/**
 * @brief      Gets the time stamp.
 *
 * @param[in]  (int) index  The index of the operation in the vector
 *                          that stores operation info.
 *
 * @return     (double) The time stamp.
 */
double OSprocessRunner::getTimeStamp(const int index) {
    return operationsVect_[index].timeStamp;
}


/**
 * @brief      Gets the actor.
 *
 * @param[in]  (int) index  The index of the operation in the vector
 *                          that stores operation info
 *
 * @return     (std::string) The actor.
 */
std::string OSprocessRunner::getActor(const int index) {
    return operationsVect_[index].actor;
}


/**
 * @brief      Gets the operation description.
 *
 * @param[in]  (int) index  The index of the operation in the vector
 *                          that stores operation info
 *
 * @return     (std::string) The operation description.
 */
std::string OSprocessRunner::getOpDescription(const int index) {
    return operationsVect_[index].opDescription;
}

/**
 * @brief      Gets the number operations stored in the vector.
 *
 * @return     (int) The number operations.
 */
int OSprocessRunner::getNumOperations() {
        return operationsVect_.size();
}

#endif