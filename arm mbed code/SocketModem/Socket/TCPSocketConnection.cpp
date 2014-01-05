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

#include "TCPSocketConnection.h"
#include <algorithm>

TCPSocketConnection::TCPSocketConnection()
{
}

int TCPSocketConnection::connect(const char* host, const int port)
{
    if (!ip->open(host, port, mts::IPStack::TCP)) {
        return -1;
    }
    return 0;
}

bool TCPSocketConnection::is_connected(void)
{
    return ip->isOpen();
}

int TCPSocketConnection::send(char* data, int length)
{
    Timer tmr;

    if (!_blocking) {
        tmr.start();
        while (tmr.read_ms() < _timeout) {
            if (ip->writeable())
                break;
        }
        if (tmr.read_ms() >= _timeout) {
            return -1;
        }
    }
    return ip->write(data, length, 0);
}

// -1 if unsuccessful, else number of bytes written
int TCPSocketConnection::send_all(char* data, int length)
{
    if (_blocking) {
        return ip->write(data, length, -1);
    } else {
        return ip->write(data, length, _timeout);
    }
}

// -1 if unsuccessful, else number of bytes received
int TCPSocketConnection::receive(char* data, int length)
{
    Timer tmr;
    int time = -1;

    if (!_blocking) {
        tmr.start();
        while (time < _timeout + 20) {
            if (ip->readable()) {
                break;
            }
            time = tmr.read_ms();
        }
        if (time >= _timeout + 20) {
            return -1;
        }
    } else {
        while(!ip->readable());
    }

    return ip->read(data, length, 0);
}


// -1 if unsuccessful, else number of bytes received
int TCPSocketConnection::receive_all(char* data, int length)
{
    if (_blocking) {
        return ip->read(data, length, -1);
    } else {
        return ip->read(data, length, _timeout);
    }
}
