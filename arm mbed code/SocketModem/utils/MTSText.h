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

#ifndef MTSTEXT_H_
#define MTSTEXT_H_

#include <string>
#include <vector>
#include <stddef.h>

namespace mts
{

class Text
{

public:
    /**
    *
    * @param source
    * @param start
    * @param cursor
    */
    static std::string getLine(const std::string& source, const size_t& start, size_t& cursor);

    /**
    *
    * @param str
    * @param delimiter
    * @param limit
    */
    static std::vector<std::string> split(const std::string& str, char delimiter, int limit = 0);

    /**
    *
    * @param str
    * @param delimiter
    * @param limit
    */
    static std::vector<std::string> split(const std::string& str, const std::string& delimiter, int limit = 0);

private:
    Text();
    Text(const Text& other);
    Text& operator=(const Text& other);
};

}
#endif

