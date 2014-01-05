/* Universal Socket Modem Interface Library
* Copyright (c) 2013 Multi-Tech Systems
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef VARS_H
#define VARS_H

#include <string>

namespace mts
{

#ifndef MAX
#define MAX(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#endif

#ifndef MIN
#define MIN(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })
#endif


/// An enumeration for common responses.
enum Code {
    SUCCESS, ERROR, FAILURE, NO_RESPONSE
};

/** A static method for getting a string representation for the Code
* enumeration.
*
* @param code a Code enumeration.
* @returns the enumeration name as a string.
*/
static std::string getCodeNames(Code code)
{
    switch(code) {
        case SUCCESS:
            return "SUCCESS";
        case ERROR:
            return "ERROR";
        case NO_RESPONSE:
            return "NO_RESPONSE";
        case FAILURE:
            return "FAILURE";
        default:
            return "UNKNOWN ENUM";
    }
}

const unsigned int PINGDELAY = 3; //Time to wait on each ping for a response before timimg out (seconds)
const unsigned int PINGNUM = 4; //Number of pings to try on ping command

//Special Payload Characters
const char ETX    = 0x03;  //Ends socket connection
const char DLE    = 0x10;  //Escapes ETX and DLE within Payload
const char CR     = 0x0D;
const char NL     = 0x0A;
const char CTRL_Z = 0x1A;


/** This class holds several enum types and other static variables
* that are used throughout the rest of the SDK.
*/
class Vars
{
public:
    /// Enumeration for different cellular radio types.
    enum Radio {NA, E1, G2, EV2, H4, EV3, H5};

    enum RelationalOperator {GREATER, LESS, EQUAL, GREATER_EQUAL, LESS_EQUAL};
};

}

//Test Commit!!!

#endif /* VARS_H */