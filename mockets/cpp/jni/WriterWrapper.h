/*
 * WriterWrapper.h
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#ifndef INCL_WRITER_WRAPPER_H
#define INCL_WRITER_WRAPPER_H

#include "jni.h"

#ifdef __cplusplus
    extern "C" {
#endif

#ifdef __cplusplus
    }
#endif

class WriterWrapper : public NOMADSUtil::Writer
{
    public:
        WriterWrapper (JNIEnv* pEnv, jobject _joOutputStream);
        
        ~WriterWrapper (void);
        
        // Returns 0 if successful or -1 in case of an error
        int writeBytes (const void *pBuf, unsigned long ulCount);

    private:
        JNIEnv* _pEnv;
        jobject _joOutputStream;
        jclass _jcOutputStream;
        jmethodID _jmWrite;

};

inline WriterWrapper::WriterWrapper (JNIEnv* pEnv, jobject joOutputStream)
{
    _pEnv = pEnv;
    _joOutputStream = joOutputStream;

    // Get a reference to OutputStream class and write method
    _joOutputStream = _pEnv->FindClass ("java/io/OutputStream");
    
    
    if (_jcOutputStream == NULL) {
        _pEnv->ThrowNew (_pEnv->FindClass ("java/lang/NullPointerException"), "java/io/OutputStream not found");
    }
    _jmWrite = _pEnv->GetMethodID (_jcOutputStream, "write", "([B)V");
    
    if (_jmWrite == NULL) {
        _pEnv->ThrowNew (_pEnv->FindClass ("java/lang/NullPointerException"), "OutputStream write(byte[]); not found");
    }
}

inline int WriterWrapper::writeBytes (const void* pBuf, unsigned long ulCount)
{
    // Create a Byte array to wrap pBuf
    jbyteArray jbaBuffer = _pEnv->NewByteArray (ulCount);
    _pEnv->SetByteArrayRegion (jbaBuffer, 0, ulCount, (jbyte *)pBuf);
    // Call writer on the java OutputStream passing the byte array
    _pEnv->CallVoidMethod (_joOutputStream, _jmWrite, jbaBuffer);

    return 0;
}

#endif   // #ifndef INCL_WRITER_WRAPPER_H
