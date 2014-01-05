/* Copyright (C) 2012 mbed.org, MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "Socket/Socket.h"
#include "Socket/Endpoint.h"
#include <cstring>

using std::memset;

Endpoint::Endpoint()
{
    reset_address();
}

Endpoint::~Endpoint()
{
}

void Endpoint::reset_address(void)
{
    _ipAddress[0] = '\0';
    _port = 0;
}

int Endpoint::set_address(const char* host, const int port)
{
    int length = strlen(host) + 1; // size of host including terminating character
    if (length > sizeof(_ipAddress)) {
        printf("[ERROR] could not set address because the hostname is too long\r\n");
        return -1;
    } else {
        strncpy((char*) _ipAddress, host, length);
        _ipAddress[length] = '\0';
        _port = port;
    }
    return 0;
}

char* Endpoint::get_address()
{
    return _ipAddress;
}

int   Endpoint::get_port()
{
    return _port;
}
