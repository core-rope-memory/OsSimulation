# OsSimulation

## Overview
The goal of this project was to create a program that simulates the functions of an operating system, with an emphasis on how different scheduling algorithms handle running multiple processes concurrently, and how an operating system allocates limited resources to multiple processes. The program is written in C++. Input/output operations are simulated by separate threads. The thread counts down the amount of time appropriate for the I/O device, then returns to the main thread and releases its I/O resource.

The program uses two text files to run. The metadata file contains the sequence of instructions that the simulations will follow. An instruction contains a code, a descriptor, and the number of clock cycles the instruction requires. The configuration file sets the parameters of the simulation. The configuration specifies the cycle times of the OS and I/O resources, the scheduling algorithm used, the number of I/O resources available to the OS, the output log file path, etc.

The simulation outputs a log file detailing how the simulation unfolded. Each line of the log file contains an event. Each event consists of the timestamp when the event occured (milliseconds from the start of the simulation), the type of event (either an OS event or a specific process event), and a description about what occured during the event. The log file shows how the simulation will shuffle the execution of multiple process tasks depending on the scheduling algorithm used.

## Setup (Ubuntu or Linux Subsystem for Windows)
1. Open Bash terminal window and navigate to the repository's root directory.
2. Install the C++ compiler: `sudo apt install g++`
3. Install `make`: `sudo apt install make`
4. Compile the program using the Makefile: `make`
5. Run the program on the provided config file: `./OsSim Config.conf`
6. View the results of the simulation: `cat logfile.lgf`

## Metadata File
These are the instructions that the simulation runs on. Change the metadata file to customize the simulation following the formatting guidelines below.

Metadata codes:
- `S` &ndash; Operating System, used with `begin` and `finish`
- `A` &ndash; Program Application, used with `begin` and `finish`
- `P` &ndash; Process, used with `run`
- `I` &ndash; used with Input operation descriptors such as `hard drive`, `keyboard`, `scanner`
- `O` &ndash; used with Output operation descriptors such as `hard drive`, `monitor`, `projector`
- `M` &ndash; Memory, used with `block`, `allocate`

Metadata descriptors:
`begin`, `finish`, `hard drive`, `keyboard`, `scanner`, `monitor`, `run`, `allocate`, `projector`, `block`

The meta-data will always follow the format:
`<META DATA CODE>{<META DATA DESCRIPTOR>}<NUMBER OF CYCLES>`

For example, an input keyboard operation that runs for 13 cycles would look like the following:
`I{keyboard}13`

**Example Metadata File:**

        Start Program Meta-Data Code:
        S{begin}0; A{begin}0; P{run}11; M{allocate}2; A{finish}0;
        A{begin}0; O{hard drive}6; A{finish}0; A{begin}0; P{run}4;
        I{keyboard}18; M{allocate}4; P{run}6; A{finish}0; S{finish}0;
        End Program Meta-Data Code.

## Configuration File
The configuration file sets up the parameters of the simulation. This will specify the various cycle times associated with each computer component, memory, and any other necessary information required to run the simulation correctly. All cycle times are specified in milliseconds. `Log File Path` is the name of the new file which will display the simulation's output. If the number of an input/output resource is not specified, it will be assumed as 1.

Example Configuration File:

        Start Simulator Configuration File
        Version/Phase: 4.0
        File Path: TestMetadata.mdf
        Quantum Number {msec}: 50
        CPU Scheduling Code: STR
        Processor cycle time {msec}: 5
        Monitor display time {msec}: 22
        Hard drive cycle time {msec}: 150
        Projector cycle time {msec}: 550
        Keyboard cycle time {msec}: 60
        Memory cycle time {msec}: 10
        System memory {kbytes}: 2048
        Memory block size {kbytes}: 128
        Projector quantity: 4
        Hard drive quantity: 2
        Log: Log to File
        Log File Path: logfile.lgf
        End Simulator Configuration File

## Scheduling
The simulator can use two interruptible scheduling algorithms, Round Robin or Shortest Time Remaining. The metadata file is reloaded every 100ms 10 times to simulate a processes' arrive time.

## Memory Allocation Operation
Memory allocation is simulated by passing in a system memory size in the configuration file. Blocks of memory of a size specified in the metadata file are added to their appropriate memory addresses. If the allocation of a block of memory will overflow the specified system memory size, the memory address is reset to 0.

## Input/Output Operations
- The number of resources available for an input/output device (hard drives, keyboards, scanners, monitors, projectors) is set by the configuration file and access to these finite resources are limited by the use of counting semaphores.
- resources are pulled from their queues under the protection of a mutex.
- Each I/O opeartion is simulated by a seperate thread that counts down the I/O operation time. The simulation polls the I/O thread untill the I/O thread completes, simulating a blocking I/O operation.

## Simulation Output
The results of the simulation are output to a logfile in the root directory of the repository. All operations are timestamped with microsecond resolution, and have a decription of the operation's action.

**Example Log File:**

        0.000000 - Simulator program starting
        0.072131 - OS: preparing process 1
        0.072135 - OS: starting process 1
        0.072137 - Process 1: start processing action
        0.127241 - Process 1: end processing action
        0.127256 - Process 1: allocating memory
        0.147405 - Process 1: memory allocated at 0x00000000
        0.147430 - OS: End process 1
        0.147437 - OS: preparing process 2
        0.147437 - OS: starting process 2
        0.147446 - Process 2: start hard drive output on HDD_0
        1.047621 - Process 2: end hard drive output
        1.047684 - Process 2: Process interrupted by STR scheduling algorithm.
        1.047700 - OS: End process 2
        1.047710 - OS: preparing process 4
        1.047710 - OS: starting process 4
        1.047712 - Process 4: start processing action
        1.102790 - Process 4: end processing action
        1.102834 - Process 4: Process interrupted by STR scheduling algorithm.
        1.102844 - Process 4: allocating memory
        1.122926 - Process 4: memory allocated at 0x00000080
        1.122955 - OS: End process 4
        1.122974 - OS: preparing process 10
        1.122976 - OS: starting process 10
        1.122977 - Process 10: start processing action
        1.178376 - Process 10: end processing action
        1.178408 - Process 10: allocating memory
        1.198777 - Process 10: memory allocated at 0x00000100
        1.198799 - OS: End process 10
        1.198810 - OS: preparing process 13
        ...
