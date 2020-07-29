# OsSimulation
This program simulates the core components of an operating system.

## Setup (Ubuntu or Linux Subsystem for Windows)
1. Open Bash terminal window and navigate to the repository's root directory.
2. Install the C++ compiler: `sudo apt install g++`
3. Install `make`: `sudo apt install make`
4. Compile the program using the Makefile: `make`
5. Run the program on the provided config file: `./OsSim Config.conf`
6. View the results of the simulation: `cat logfile_1.lgf`

## Metadata File
These are the instructions that the simulation runs on. Change the metadata file to customize the simulation following the formatting guidelines below.

Metadata codes:
- `S` &ndash; Operating System, used with begin and finish
- `A` &ndash; Program Application, used with begin and finish
- `P` &ndash; Process, used with run
- `I` &ndash; used with Input operation descriptors such as hard drive, keyboard, scanner
- `O` &ndash; used with Output operation descriptors such as hard drive, monitor, projector
- `M` &ndash; Memory, used with block, allocate

Metadata descriptors:
`begin, finish, hard drive, keyboard, scanner, monitor, run, allocate, projector, block`

The meta-data will always follow the format:
`<META DATA CODE>(<META DATA DESCRIPTOR>)<NUMBER OF CYCLES>`

For example, an input keyboard operation that runs for 13 cycles would look like the following:
`I(keyboard)13`

Example Metadata File:

        Start Program Meta-Data Code:
        S{begin}0; A{begin}0; P{run}11; M{allocate}2; A{finish}0;
        A{begin}0; O{hard drive}6; A{finish}0; A{begin}0; P{run}4;
        I{keyboard}18; M{allocate}4; P{run}6; A{finish}0; S{finish}0;
        End Program Meta-Data Code.

## Configuration File
The configuration file sets up the parameters of the simulation. This will specify the various cycle times associated with each computer component, memory, and any other necessary information required to run the simulation correctly. All cycle times are specified in milliseconds. Log File Path is the name of the new file which will display the simulation's output. If the number of an input/output resource is not specified, it will be assumed as 1.

Example Configuration File:

        Start Simulator Configuration File
        Version/Phase: 4.0
        File Path: Test_5a.mdf
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
        Log File Path: logfile_1.lgf
        End Simulator Configuration File

## Memory Allocation Operation
Memory allocation is simulated by passing in a system memory size in the configuration file. Blocks of memory of a size specified in the metadata file are added to their appropriate memory addresses. If the allocation of a block of memory will overflow the specified system memory size, the memory address is reset to 0.

## Input/Output Operations
- The number of resources available for an input/output device (hard drives, keyboards, scanners, monitors, projectors) is set by the configuration file and access to these finite resources are limited by the use of counting semaphores.
- Each I/O opeartion is simulated by a seperate thread that counts down the I/O operation time. The simulation polls the I/O thread untile the I/O thread completes simulating a blocking I/O operation.

## Simulation Output
The results of the simulation are output to a logfile in the root directory of the repository. All operations are timestamped with microsecond resolution, and have a decription of the operation's action.

## Scheduling
The simulator can use two interruptible scheduling algorithms, Round Robin or Shortest Time Remaining. The metadata file is reloaded every 100ms 10 times to simulate a processes' arrive time.
