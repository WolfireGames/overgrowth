//-----------------------------------------------------------------------------
//           Name: sample_interfaces.h
//      Developer: Wolfire Games LLC
//         Author:
//    Description:
//        License: zlib
//-----------------------------------------------------------------------------

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include "sample_interfaces.h"
#include "Recast.h"
#include <cstring>
#include <Compat/fileio.h>

#ifdef WIN32
#define snprintf _snprintf
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////

BuildContext::BuildContext() : m_messageCount(0),
                               m_textPoolSize(0) {
    resetTimers();
}

// Virtual functions for custom implementations.
void BuildContext::doResetLog() {
    m_messageCount = 0;
    m_textPoolSize = 0;
}

void BuildContext::doLog(const rcLogCategory category, const char* msg, const int len) {
    if (!len) return;
    if (m_messageCount >= MAX_MESSAGES)
        return;
    char* dst = &m_textPool[m_textPoolSize];
    int n = TEXT_POOL_SIZE - m_textPoolSize;
    if (n < 2)
        return;
    char* cat = dst;
    char* text = dst + 1;
    const int maxtext = n - 1;
    // Store category
    *cat = (char)category;
    // Store message
    const int count = rcMin(len + 1, maxtext);
    memcpy(text, msg, count);
    text[count - 1] = '\0';
    m_textPoolSize += 1 + count;
    m_messages[m_messageCount++] = dst;
}

void BuildContext::doResetTimers() {
}

void BuildContext::doStartTimer(const rcTimerLabel label) {
}

void BuildContext::doStopTimer(const rcTimerLabel label) {
}

int BuildContext::doGetAccumulatedTime(const rcTimerLabel label) const {
    return 0;
}

void BuildContext::dumpLog(const char* format, ...) {
    // Print header.
    va_list ap;
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
    printf("\n");

    // Print messages
    const int TAB_STOPS[4] = {28, 36, 44, 52};
    for (int i = 0; i < m_messageCount; ++i) {
        const char* msg = m_messages[i] + 1;
        int n = 0;
        while (*msg) {
            if (*msg == '\t') {
                int count = 1;
                for (int j : TAB_STOPS) {
                    if (n < j) {
                        count = j - n;
                        break;
                    }
                }
                while (--count) {
                    putchar(' ');
                    n++;
                }
            } else {
                putchar(*msg);
                n++;
            }
            msg++;
        }
        putchar('\n');
    }
}

int BuildContext::getLogCount() const {
    return m_messageCount;
}

const char* BuildContext::getLogText(const int i) const {
    return m_messages[i] + 1;
}

FileIO::FileIO() : m_fp(0),
                   m_mode(-1) {
}

FileIO::~FileIO() {
    if (m_fp) fclose(m_fp);
}

bool FileIO::openForWrite(const char* path) {
    if (m_fp) return false;
    m_fp = my_fopen(path, "wb");
    if (!m_fp) return false;
    m_mode = 1;
    return true;
}

bool FileIO::openForRead(const char* path) {
    if (m_fp) return false;
    m_fp = my_fopen(path, "rb");
    if (!m_fp) return false;
    m_mode = 2;
    return true;
}

bool FileIO::isWriting() const {
    return m_mode == 1;
}

bool FileIO::isReading() const {
    return m_mode == 2;
}

bool FileIO::write(const void* ptr, const size_t size) {
    if (!m_fp || m_mode != 1) return false;
    fwrite(ptr, size, 1, m_fp);
    return true;
}

bool FileIO::read(void* ptr, const size_t size) {
    if (!m_fp || m_mode != 2) return false;
    size_t readLen = fread(ptr, size, 1, m_fp);
    return readLen == 1;
}
