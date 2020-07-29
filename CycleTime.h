/** 
 *  @file    CycleTime.h
 *  @author  Ian Vanderhoff
 *  @date    2/1/2018 
 *  @version 1 
 *
 */

#ifndef _CYCLETIME
#define _CYCLETIME

#include <string>

/**
 * @brief      Class for the name of a cycle and its time per one cycle.
 */
class CycleTime
{
public:
    /**
     * @brief      Default Constructor sets description to empty string
     * 			   and milliseconds to zero.
     */
    CycleTime() {
        // For this object to be placed into the map, its data needs to be
        // initialized in a default constructor.
        description_ = " ";
        milliseconds_ = 0;
    }

    // OVERLOADED CONSTRUCTOR
    CycleTime(std::string description , int milliseconds);

    // GETTER FUNCTIONS
    std::string getDescription() const;
    int getTime() const;    

private:
    // PRIVATE DATA
    std::string description_;
    int milliseconds_;
};

//******************************************************************************
// IMPLEMENTATION
//******************************************************************************

/**
 * @brief      Constructs the object with description and milliseconds
 * 			   provided.
 *
 * @param[in]  description   The name of the cycle
 * @param[in]  milliseconds  The milliseconds per one cycle
 */
CycleTime::CycleTime(std::string description , int milliseconds) {
    description_ = description;
    milliseconds_ = milliseconds;
}

/**
 * @brief      Gets the name of the cycle.
 *
 * @return     The name of the cycle.
 */
std::string CycleTime::getDescription() const {
    return description_;
}

/**
 * @brief      Gets the time in milliseconds for one cycle.
 *
 * @return     The time in milliseconds for one cycle.
 */
int CycleTime::getTime() const {
    return milliseconds_;
}

#endif