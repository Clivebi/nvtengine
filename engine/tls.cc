#include "tls.hpp"
#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>

#include <map>
#include <mutex>
#endif
namespace Interpreter {
#ifdef _WIN32
size_t TLS::Allocate() {
    return TlsAlloc();
}
void* TLS::GetValue(size_t index) {
    return TlsGetValue(index);
}
void TLS::SetValue(size_t index, void* value) {
    TlsSetValue(index, value);
}
#else
static std::mutex s_TlsLock;
static std::map<size_t, std::map<pthread_t, void*> > s_TlsSlot;
size_t TLS::Allocate() {
    size_t ret = 0;
    s_TlsLock.lock();
    ret = s_TlsSlot.size() + 1;
    s_TlsLock.unlock();
    return ret;
}
void* TLS::GetValue(size_t index) {
    void* result = NULL;
    pthread_t tid = pthread_self();
    std::lock_guard guard(s_TlsLock);
    auto values = s_TlsSlot.find(index);
    if (values != s_TlsSlot.end()) {
        auto iter = values->second.find(tid);
        if (iter != values->second.end()) {
            result = iter->second;
        }
    }
    return result;
}
void TLS::SetValue(size_t index, void* value) {
    pthread_t tid = pthread_self();
    std::lock_guard guard(s_TlsLock);
    auto values = s_TlsSlot.find(index);
    if (values != s_TlsSlot.end()) {
        values->second[tid] = value;
    }
}
#endif
} // namespace Interpreter
