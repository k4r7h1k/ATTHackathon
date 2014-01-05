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

#ifndef TESTMTSCIRCULARBUFFER_H
#define TESTMTSCIRCULARBUFFER_H

#include "MTSCircularBuffer.h"
#include "Vars.h"

/* unit tests for the circular buffer class */

using namespace mts;

int capacity = 0;
MTSCircularBuffer* buffer = new MTSCircularBuffer(5);

void callback()
{
    capacity = buffer->remaining();
}

int testMTSCircularBuffer()
{
    printf("Testing: MTSCircularBuffer\r\n");
    int failed = 0;
    char byte;


    //Test getSize method
    if (buffer->capacity() != 5) {
        printf("Failed: capacity()\r\n");
        failed++;
    }

    //Test clear function
    buffer->write("AT", 2);
    buffer->clear();
    if (buffer->size() != 0) {
        printf("Failed: clear()\r\n");
        failed++;
    }

    /* The next set of test all rely on an empty buffer!!! */

    //Test isEmpty method - empty buffer
    if (buffer->isEmpty() != true) {
        printf("Failed: isEmpty() - empty\r\n");
        failed++;
    }

    //Test isFull method - empty buffer
    if (buffer->isFull() == true) {
        printf("Failed: isFull() - empty\r\n");
        failed++;
    }

    //Test capacity method - empty buffer
    if (buffer->remaining() != 5) {
        printf("Failed: remaining() - empty\r\n");
        failed++;
    }

    //Test available method - empty buffer
    if (buffer->size() != 0) {
        printf("Failed: size() - empty\r\n");
        failed++;
    }

    /* The next set of tests all rely on a full buffer */

    //Test bulk write method
    int tmp = buffer->write("Test", 5);
    if (tmp != 5) {
        printf("Failed: bulk write()\r\n");
        failed++;
    }

    //Test isEmpty method - full buffer
    if (buffer->isEmpty() == true) {
        printf("Failed: isEmpty() - full\r\n");
        failed++;
    }

    //Test isFull method - full buffer
    if (buffer->isFull() == false) {
        printf("Failed: isFull() - full\r\n");
        failed++;
    }

    //Test capacity method - full buffer
    if (buffer->remaining() != 0) {
        printf("Failed: remaining() - full\r\n");
        failed++;
    }

    //Test available method - full buffer
    if (buffer->size() != 5) {
        printf("Failed: size() - full\r\n");
        failed++;
    }

    //Test single overwrite method
    if (buffer->write('A') != 0) {
        printf("Failed: write() - overwrite\r\n");
        failed++;
    }

    //Test bulk overwrite method
    if (buffer->write("Test", 5) != 0) {
        printf("Failed: bulk write() - overflow\r\n");
        failed++;
    }

    //Test single read method
    if ((buffer->read(byte) < 1 && byte != 'T') || buffer->remaining() != 1) {
        printf("Failed: single read()\r\n");
        failed++;
    }

    //Test bulk read method
    char data[5];
    if (buffer->read(data, 4) != 4 || data[0] != 'e' || data[1] != 's' || data[2] != 't' || data[3] != '\0') {
        printf("Failed: bulk read()\r\n");
        failed++;
    }

    //Test wrap write/read methods
    tmp = buffer->write("AT", 3);
    if (tmp != 3 || (buffer->read(byte) < 1 && byte != 'A') || (buffer->read(byte) < 1 && byte != 'T')) {
        printf("Failed: wrap write()/read()\r\n");
        failed++;
    }
    buffer->clear();

    /* The next set of test are focused all on the attach methods */

    //Test attach with greater than below level
    buffer->attach(&callback, 3, Vars::GREATER);
    buffer->write("ABC", 3);
    if (capacity != 0) {
        printf("Failed: attach() - greater/below\r\n");
        failed++;
    }

    //Test attach with greater than above level
    buffer->write('T');
    if (capacity != 1) {
        printf("Failed: attach() - greater/above\r\n");
        failed++;
    }
    buffer->clear();
    capacity = 0;

    //Test attach with greater equal than below level
    buffer->attach(&callback, 3, Vars::GREATER_EQUAL);
    buffer->write("AB", 2);
    if (capacity != 0) {
        printf("Failed: attach() - greater equal/below\r\n");
        failed++;
    }

    //Test attach with greater equal than above level
    buffer->write('T');
    if (capacity != 2) {
        printf("Failed: attach() - greater equal/above\r\n");
        failed++;
    }

    //Test attach with less than above level
    buffer->clear();
    buffer->write("ABC", 3);
    capacity = 0;
    buffer->attach(&callback, 2, Vars::LESS);
    buffer->read(byte);
    if (capacity != 0) {
        printf("Failed: attach() - less_equal/above\r\n");
        failed++;
    }

    //Test attach with less than below level
    buffer->read(byte);
    if (capacity != 4) {
        printf("Failed: attach() - less_equal/below%d\r\n", capacity);
        failed++;
    }

    //Test attach with less equal than above level
    buffer->clear();
    buffer->write("Test", 4);
    capacity = 0;
    buffer->attach(&callback, 2, Vars::LESS_EQUAL);
    buffer->read(byte);
    if (capacity != 0) {
        printf("Failed: attach() - less equal/above\r\n");
        failed++;
    }

    //Test attach with less equal than below level
    buffer->read(byte);
    if (capacity != 3) {
        printf("Failed: attach() - less equal/below%d\r\n", capacity);
        failed++;
    }

    //Test attach with less equal than above level
    buffer->clear();
    buffer->write("Test", 4);
    capacity = 0;
    buffer->attach(&callback, 2, Vars::EQUAL);
    buffer->read(byte);
    if (capacity != 0) {
        printf("Failed: attach() - equal/above\r\n");
        failed++;
    }

    //Test attach with less equal than below level
    buffer->read(byte);
    if (capacity != 3) {
        printf("Failed: attach() - equal/below%d\r\n", capacity);
        failed++;
    }

    //Test Ins and Outs
    {
        const char inData[] = "*ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890*";
        const int size = sizeof(inData); 
        char outData[size];
        
        int bytesWritten = 0;
        int bytesRead = 0;
        buffer->clear();
        
        Timer tmr;
        tmr.start();
        do {       
            int remaining = size - bytesRead;
            int readable = buffer->size();
            if(remaining) {
                if(readable) {
                    //printf("READABLE [%d]\r\n", readable);
                    int received = buffer->read(&outData[bytesRead], remaining);
                    bytesRead += received;
                    //printf("READ [%d]  TOTAL[%d]  REMAINING[%d]\r\n", received, bytesRead, size - bytesRead);
                }
            }
            
            remaining = size - bytesWritten;
            int writeable = buffer->remaining();
            if(remaining) {
                if(writeable) {
                    //printf("WRITEABLE [%d]\r\n", writeable);
                    int written = buffer->write(&inData[bytesWritten], remaining);   
                    bytesWritten += written;
                    remaining = size - bytesWritten;
                    //printf("WROTE [%d]  TOTAL[%d]  REMAINING[%d]\r\n", written, bytesWritten, size - bytesWritten);
                }
            }
            
        } while (tmr.read_ms() <= 5000 && bytesRead < size);
        
        printf("INPUT  [%d]: [%s]\r\n", size, inData);
        printf("OUTPUT [%d]: [%s]\r\n", bytesRead, outData);
        for(int i = 0; i < size - 1; i++) {
            if(inData[i] != outData[i]) {
                printf("Failed: Buffers do not match at index %d\r\n", i);
                failed++;
                break;   
            }   
        }
    }

    printf("Finished Testing: MTSCircularBuffer\r\n");
    return failed;
}

#endif /* TESTMTSCIRCULARBUFFER_H */
