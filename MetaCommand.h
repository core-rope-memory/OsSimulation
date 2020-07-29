/** 
 *  @file    MetaCommand.h
 *  @author  Ian Vanderhoff
 *  @date    2/28/2018 
 *  @version 2 
 *
 */

#ifndef _META_COMMAND
#define _META_COMMAND

#include <string>

/**
 * @brief      Class for the meta data command code, command descriptor, 
 *             the number of cycles required by the command, and the 
 *             time of the cycles.
 */
class MetaCommand
{
public:
    /**
     * @brief      Default constructor sets code to '.', descriptor to one 
     *             empty space, cycles to zero, and time to zero.
     */
    MetaCommand() {
        code_ = '.';
        descriptor_ = " ";
        cycles_ = 0;
        time_ = 0;
    }

    // OVERLOADED CONSTRUCTOR
    MetaCommand(char code, std::string descriptor, int cycles, int time);

    // GETTER FUNCTIONS
    char getCode() const;
    std::string getDescriptor() const;
    int getCycles() const;
    int getTime() const;

private:
    // PRIVATE DATA
    char code_;
    std::string descriptor_;
    int cycles_;
    int time_;
};

//******************************************************************************
// IMPLEMENTATION
//******************************************************************************

/**
 * @brief      Constructs the object with code, descriptor, cycles provided,
 *             and time.
 *
 * @param[in]  code        The single character code S|A|P|I|O|M
 * @param[in]  descriptor  The descriptor
 * @param[in]  cycles      The cycles
 * @param[in]  time        The total time of the cycles
 */
MetaCommand::MetaCommand(char code, std::string descriptor, int cycles, int time) {
    code_ = code;
    descriptor_ = descriptor;
    cycles_ = cycles;
    time_ = time;
}

/**
 * @brief      Gets the code.
 *
 * @return     (char) The code.
 */
char MetaCommand::getCode() const {
    return code_;
}

/**
 * @brief      Gets the descriptor.
 *
 * @return     (std::string) The descriptor.
 */
std::string MetaCommand::getDescriptor() const {
    return descriptor_;
}

/**
 * @brief      Gets the cycles.
 *
 * @return     (int) The cycles.
 */
int MetaCommand::getCycles() const {
    return cycles_;
}


/**
 * @brief      Gets the time.
 *
 * @return     (int) The time.
 */
int MetaCommand::getTime() const {
    return time_;
}

#endif