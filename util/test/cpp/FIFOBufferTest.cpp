
#include <stdio.h>
#include <stdlib.h>

#include "FIFOBuffer.h"


int main(int argc, char** argv)
{
    uint32 ui32Size = 10;
    NOMADSUtil::FIFOBuffer<int> *pTestBuf = new NOMADSUtil::FIFOBuffer<int> (ui32Size);
    int newData[5] = {1,2,3,4,5};
    int newData2[5] = {6,7,8,9,10};
    int res;
    const NOMADSUtil::FIFOBuffer<int>::Buffer *pResBuf;

    // TEST PEEK
    // Situation A) head=tail=offset=0 => fill the buffer and then peek
    printf ("Situation A) head=tail=offset=0 => fill the buffer and then peek\n");
    // Add some stuff
    res = pTestBuf->enqueue(newData, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    res = pTestBuf->enqueue(newData2, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    pResBuf = pTestBuf->peek();
    // Print size and space available
    printf ("Peek into the buffer. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }

    // Situation B) head=tail!=0 offset=0 => enqueue x, dequeue x, fill up and then peek
    printf ("Situation B) head=tail!=0 offset=0 => enqueue 3, dequeue 3, fill up and then peek\n");
    pTestBuf->clear();
    // Add some stuff
    res = pTestBuf->enqueue(newData, 3);
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(3);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }
    // Add some stuff
    res = pTestBuf->enqueue(newData, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    res = pTestBuf->enqueue(newData2, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    pResBuf = pTestBuf->peek();
    // Print size and space available
    printf ("Peek into the buffer. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }

    // Situation C1) head>tail tail=0 offset=0 => enqueue x and then peek
    printf ("Situation C1) head>tail tail=0 offset=0 => enqueue 3 and then peek\n");
    pTestBuf->clear();
    // Add some stuff
    res = pTestBuf->enqueue(newData, 3);
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // peek
    pResBuf = pTestBuf->peek();
    // Print size and space available
    printf ("Peek into the buffer. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }

    // Situation C2) head>tail tail!=0 offset=0 => enqueue x, dequeue x, enqueue y and then peek with x+y<bufSize
    printf ("Situation C2) head>tail tail!=0 offset=0 => enqueue 3, dequeue 3, enqueue 5 and then peek with 3+5<bufSize\n");
    pTestBuf->clear();
    // Add some stuff
    res = pTestBuf->enqueue(newData, 3);
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(3);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }
    // Add some stuff
    res = pTestBuf->enqueue(newData, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // peek
    pResBuf = pTestBuf->peek();
    // Print size and space available
    printf ("Peek into the buffer. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }
    
    // Situation D1) tail>head head=0 offset=0 => fill up, dequeue x<queueSize then peek
    printf ("Situation D1) tail>head head=0 offset=0 => fill up, dequeue 3<queueSize then peek\n");
    pTestBuf->clear();
    // Add some stuff
    res = pTestBuf->enqueue(newData, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    res = pTestBuf->enqueue(newData2, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(3);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }
    // peek
    pResBuf = pTestBuf->peek();
    // Print size and space available
    printf ("Peek into the buffer. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }

    // Situation D2) tail>head head!=0 offset=0 => fill up, dequeue x<queueSize, enqueue y<x then peek
    printf ("Situation D2) tail>head head!=0 offset=0 => fill up, dequeue 5<queueSize, enqueue 3<5 then peek\n");
    pTestBuf->clear();
    // Add some stuff
    res = pTestBuf->enqueue(newData, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    res = pTestBuf->enqueue(newData2, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(5);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }
    // Add some stuff
    res = pTestBuf->enqueue(newData, 3);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // peek
    pResBuf = pTestBuf->peek();
    // Print size and space available
    printf ("Peek into the buffer. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }

    // Situation E) head=tail=0 offset>0 => fill the buffer and then peek
    printf ("Situation E) head=tail=0 offset>0 => fill the buffer and then peek offset=2\n");
    pTestBuf->clear();
    // Add some stuff
    res = pTestBuf->enqueue(newData, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    res = pTestBuf->enqueue(newData2, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Peek
    pResBuf = pTestBuf->peek(2);
    // Print size and space available
    printf ("Peek into the buffer. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }

    // Situation F) head=tail!=0 offset>0 => enqueue x, dequeue x, fill up and then peek
    printf ("Situation F) head=tail!=0 offset>0 => enqueue 3, dequeue 3, fill up and then peek offset=2\n");
    pTestBuf->clear();
    // Add some stuff
    res = pTestBuf->enqueue(newData, 3);
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(3);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }
    // Add some stuff
    res = pTestBuf->enqueue(newData, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    res = pTestBuf->enqueue(newData2, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    pResBuf = pTestBuf->peek(2);
    // Print size and space available
    printf ("Peek into the buffer. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }

    // Situation G1) head>tail tail=0 offset>0 => enqueue x and then peek
    printf ("Situation G1) head>tail tail=0 offset>0 => enqueue 5 and then peek offset=2\n");
    pTestBuf->clear();
    // Add some stuff
    res = pTestBuf->enqueue(newData, 5);
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // peek
    pResBuf = pTestBuf->peek(2);
    // Print size and space available
    printf ("Peek into the buffer. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }

    // Situation G2) head>tail tail!=0 offset>0 => enqueue x, dequeue x, enqueue y and then peek with x+y<bufSize
    printf ("Situation G2) head>tail tail!=0 offset>0 => enqueue 3, dequeue 3, enqueue 5 and then peek with 3+5<bufSize offset=2\n");
    pTestBuf->clear();
    // Add some stuff
    res = pTestBuf->enqueue(newData, 3);
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(3);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }
    // Add some stuff
    res = pTestBuf->enqueue(newData, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // peek
    pResBuf = pTestBuf->peek(2);
    // Print size and space available
    printf ("Peek into the buffer. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }

    // Situation H1) tail>head head=0 offset>0 => fill up, dequeue x<queueSize then peek
    printf ("Situation H1) tail>head head=0 offset>0 => fill up, dequeue 3<queueSize then peek offset=2\n");
    pTestBuf->clear();
    // Add some stuff
    res = pTestBuf->enqueue(newData, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    res = pTestBuf->enqueue(newData2, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(3);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }
    // peek
    pResBuf = pTestBuf->peek(2);
    // Print size and space available
    printf ("Peek into the buffer. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }

    // Situation H2) tail>head head!=0 offset>0 => fill up, dequeue x<queueSize, enqueue y<x then peek
    printf ("Situation H2) tail>head head!=0 offset>0 => fill up, dequeue 5<queueSize, enqueue 3<5 then peek offset=2\n");
    pTestBuf->clear();
    // Add some stuff
    res = pTestBuf->enqueue(newData, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    res = pTestBuf->enqueue(newData2, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(5);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }
    // Add some stuff
    res = pTestBuf->enqueue(newData, 3);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // peek
    pResBuf = pTestBuf->peek(2);
    // Print size and space available
    printf ("Peek into the buffer. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }

    // Situation H3) tail>head head!=0 offset>0 with wrap around => fill up, dequeue x<queueSize, enqueue y<x then peek with offset that wraps around so at least queueSize-x
    printf ("Situation H3) tail>head head!=0 offset>0 => fill up, dequeue 5<queueSize, enqueue 3<5 then peek offset=5\n");
    pTestBuf->clear();
    // Add some stuff
    res = pTestBuf->enqueue(newData, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    res = pTestBuf->enqueue(newData2, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(5);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }
    // Add some stuff
    res = pTestBuf->enqueue(newData, 3);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // peek
    pResBuf = pTestBuf->peek(5);
    // Print size and space available
    printf ("Peek into the buffer. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }
    // Situation H4) tail>head head!=0 offset>0 => fill up, dequeue x<queueSize, enqueue y<x then peek with offset that wrap around past the beginning of the queue
    printf ("Situation H4) tail>head head!=0 offset>0 => fill up, dequeue 5<queueSize, enqueue 3<5 then peek offset=6\n");
    pTestBuf->clear();
    // Add some stuff
    res = pTestBuf->enqueue(newData, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    res = pTestBuf->enqueue(newData2, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(5);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }
    // Add some stuff
    res = pTestBuf->enqueue(newData, 3);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // peek
    pResBuf = pTestBuf->peek(6);
    // Print size and space available
    printf ("Peek into the buffer. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }


    // TEST DEQUEUE
    // Situation A1) head=tail=0 ui32Count=bufSize (get all the available elements) => fill up then dequeue all
    printf ("Situation A1) head=tail=0 ui32Count=bufSize (get all the available elements) => fill up then dequeue all\n");
    pTestBuf->clear();
    // Add some stuff
    res = pTestBuf->enqueue(newData, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    res = pTestBuf->enqueue(newData2, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(ui32Size);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }

    // Situation A2) head=tail!=0 ui32Count=bufSize (get all the available elements) => fill up, dequeue x, enqueue x then dequeue all
    printf ("Situation A2) head=tail!=0 ui32Count=bufSize (get all the available elements) => fill up, dequeue 3, enqueue 3 then dequeue all\n");
    pTestBuf->clear();
    // Add some stuff
    res = pTestBuf->enqueue(newData, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    res = pTestBuf->enqueue(newData2, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(3);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }
    // Add some stuff
    res = pTestBuf->enqueue(newData, 3);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(ui32Size);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }

    // Situation A3) head=tail=0 ui32Count<getQueueSize() => fill up then dequeue some data
    printf ("Situation A3) head=tail=0 ui32Count<getQueueSize() => fill up then dequeue 3\n");
    pTestBuf->clear();
    // Add some stuff
    res = pTestBuf->enqueue(newData, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    res = pTestBuf->enqueue(newData2, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(3);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }

    // Situation A4) head=tail!=0 ui32Count<getQueueSize() => fill up, dequeue x, enqueue x then dequeue some
    printf ("Situation A4) head=tail!=0 ui32Count<getQueueSize() => fill up, dequeue 3, enqueue 3 then dequeue 3\n");
    pTestBuf->clear();
    // Add some stuff
    res = pTestBuf->enqueue(newData, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    res = pTestBuf->enqueue(newData2, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(3);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }
    // Add some stuff
    res = pTestBuf->enqueue(newData, 3);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(3);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }

    // Situation B1) head>tail tail=0 ui32Count=bufSize => enqueue 5, dequeue all
    printf ("Situation B1) head>tail tail=0 ui32Count=bufSize => enqueue x, dequeue all\n");
    pTestBuf->clear();
    // Add some stuff
    res = pTestBuf->enqueue(newData, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(ui32Size);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }

    // Situation B2) head>tail tail!=0 ui32Count=bufSize => enqueue x, dequeue y>x, dequeue all
    printf ("Situation B2) head>tail tail!=0 ui32Count=bufSize => enqueue 5, dequeue 3, dequeue all\n");
    pTestBuf->clear();
    // Add some stuff
    res = pTestBuf->enqueue(newData, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(3);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(ui32Size);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }

    // Situation B3) head>tail tail=0 ui32Count<getQueueSize() => enqueue x, dequeue y<x
    printf ("Situation B3) head>tail tail=0 ui32Count<getQueueSize() => enqueue 5, dequeue 3: see above\n");


    // Situation B4) head>tail tail!=0 ui32Count<getQueueSize() => enqueue x, dequeue y, dequeue z
    printf ("Situation B4) head>tail tail!=0 ui32Count<getQueueSize() => enqueue 5, dequeue 3, dequeue 1\n");
    pTestBuf->clear();
    // Add some stuff
    res = pTestBuf->enqueue(newData, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(3);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(1);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }
    // Situation C1) tail>head head=0 ui32Count=bufSize => fill up, dequeue x, dequeue all
    printf ("Situation C1) tail>head head=0 ui32Count=bufSize => fill up, dequeue 5, dequeue all\n");
    pTestBuf->clear();
    // Add some stuff
    res = pTestBuf->enqueue(newData, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    res = pTestBuf->enqueue(newData2, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(5);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(ui32Size);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }

    // Situation C2) tail>head head!=0 ui32Count=bufSize => fill up, dequeue x, enqueue y<x, dequeue all
    printf ("Situation C2) tail>head head!=0 ui32Count=bufSize => fill up, dequeue 5, enqueue 3, dequeue all\n");
    pTestBuf->clear();
    // Add some stuff
    res = pTestBuf->enqueue(newData, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    res = pTestBuf->enqueue(newData2, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(5);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }
    // Add some stuff
    res = pTestBuf->enqueue(newData, 3);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(ui32Size);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }

    // Situation C3) tail>head head=0 ui32Count<getQueueSize() => fill up, dequeue x, dequeue y
    printf ("Situation C3) tail>head head=0 ui32Count<getQueueSize() => fill up, dequeue 5, dequeue 2\n");
    pTestBuf->clear();
    // Add some stuff
    res = pTestBuf->enqueue(newData, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    res = pTestBuf->enqueue(newData2, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(5);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(2);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }

    // Situation C4) tail>head head!=0 ui32Count<getQueueSize() => fill up, dequeue x, enqueue y<x, dequeue z
    printf ("Situation C4) tail>head head!=0 ui32Count<getQueueSize() => fill up, dequeue 5, enqueue 3, dequeue 7\n");
    pTestBuf->clear();
    // Add some stuff
    res = pTestBuf->enqueue(newData, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    res = pTestBuf->enqueue(newData2, 5);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Remove some stuff
    pResBuf = pTestBuf->dequeue(5);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }
    // Add some stuff
    res = pTestBuf->enqueue(newData, 3);
    // Print size and space available
    printf ("Enqueued %d elements. Queue size = %d, queue free space = %d \n", res, pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());

    // Remove some stuff
    pResBuf = pTestBuf->dequeue(7);
    // Print size and space available
    printf ("Dequeued elements. Queue size = %d, queue free space = %d \n", pTestBuf->getQueueSize(), pTestBuf->getFreeSpace());
    // Print the stuff extracted
    for (int k=0; k<pResBuf->ui32FirstBufLen; k++) {
        printf ("Element from first buf: %d\n", pResBuf->pFirstBuf[k]);
    }
    for (int j=0; j<pResBuf->ui32SecondBufLen; j++) {
        printf ("Element from second buff: %d\n", pResBuf->pSecondBuf[j]);
    }



    return (EXIT_SUCCESS);
}

