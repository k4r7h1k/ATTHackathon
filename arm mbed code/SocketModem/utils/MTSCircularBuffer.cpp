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

#include "MTSCircularBuffer.h"

using namespace mts;

MTSCircularBuffer::MTSCircularBuffer(int bufferSize) : bufferSize(bufferSize), readIndex(0), writeIndex(0), bytes(0), _threshold(-1), _op(Vars::GREATER)
{
    buffer = new char[bufferSize];
}

MTSCircularBuffer::~MTSCircularBuffer()
{
    delete[] buffer;
}

int MTSCircularBuffer::capacity()
{
    return bufferSize;
}

int MTSCircularBuffer::read(char* data, int length)
{
    int i = 0;
    while ((i < length) && (bytes > 0)) {
        if (readIndex == bufferSize) {
            readIndex = 0;
        }
        data[i++] = buffer[readIndex++];
        bytes--;
        checkThreshold();
    }
    return i;
}

int MTSCircularBuffer::read(char& data)
{
    if (bytes == 0) {
        return 0;
    }
    if (readIndex == bufferSize) {
        readIndex = 0;
    }
    data = buffer[readIndex++];
    bytes--;
    checkThreshold();
    return 1;
}

int MTSCircularBuffer::write(const char* data, int length)
{
    int i = 0;
    while((i < length) && (bytes < bufferSize)) {
        if(writeIndex == bufferSize) {
            writeIndex = 0;
        }
        buffer[writeIndex++] = data[i++];
        bytes++;
        checkThreshold();
    }
    return i;
}

int MTSCircularBuffer::write(char data)
{
    if (bytes == bufferSize) {
        return 0;
    }
    if(writeIndex == bufferSize) {
        writeIndex = 0;
    }
    buffer[writeIndex++] = data;
    bytes++;
    checkThreshold();
    return 1;
}

int MTSCircularBuffer::remaining()
{
    return bufferSize - bytes;
}

int MTSCircularBuffer::size()
{
    return bytes;
}

bool MTSCircularBuffer::isFull()
{
    if (bytes == bufferSize) {
        return true;
    } else {
        return false;
    }
}

bool MTSCircularBuffer::isEmpty()
{
    if (bytes == 0) {
        return true;
    } else {
        return false;
    }
}

void MTSCircularBuffer::clear()
{
    writeIndex = readIndex = bytes = 0;
}

void MTSCircularBuffer::checkThreshold()
{
    if (_threshold == -1) {
        return;
    }
    switch (_op) {
        case Vars::GREATER:
            if (bytes > _threshold) {
                notify.call();
            }
            break;
        case Vars::LESS:
            if (bytes < _threshold) {
                notify.call();
            }
            break;
        case Vars::GREATER_EQUAL:
            if (bytes >= _threshold) {
                notify.call();
            }
            break;
        case Vars::LESS_EQUAL:
            if (bytes <= _threshold) {
                notify.call();
            }
            break;
        case Vars::EQUAL:
            if (bytes == _threshold) {
                notify.call();
            }
            break;
    }
}

