#ifndef _STREAM_CONTEXT_
#define _STREAM_CONTEXT_

#include <vector>

const char SZ_START_FP[] = {0162, 0172, 015, 052, 052, 030, 0102, 060, 060, 060, 060, 060, 060, 060, 060, 060};
const char RZ_START_FP[] = {052,  052,  030, 102, 060, 061, 060,  060, 060, 060, 060, 060, 062, 063, 0142, 0145};

class Matcher {
public:
    enum Status {
        Unknown,
        Found,
        MustNot,
        Maybe
    };
    Status Check(char d);
    Status Query() const { return m_status; }
    Matcher(const char* fp, size_t size);
    void Reset() { enter_mustnot(); };
private:
    void enter_found() { m_pos = 0; m_status = Found; };
    void enter_maybe() { m_pos++; m_status = Maybe; };
    void enter_mustnot() { m_pos = 0; m_status = MustNot; };

    const std::vector<char> m_fp;
    const size_t m_fp_size;
    size_t m_pos;
    Status m_status;
};



class StreamContext {
public:
    typedef void (*Slot)(StreamContext*);
    
    void Init();

    const std::vector<char> Filter(const char* buf, size_t len);
    
    void Reset() { m_matcher->Reset(); }
    void connect(Slot slot) { m_slot = slot; }

    Matcher::Status QueryStatus() { return m_matcher->Query(); };
    StreamContext(Matcher* matcher) : m_matcher(matcher) {};    
private:

    const std::vector<char> filter(const char* buf, size_t len);
        
    Matcher* m_matcher;

    std::vector<char> m_cached;

    Slot m_slot;
};


struct FilterContext {
    int outFd;
    std::vector<StreamContext*> filters;
};


#endif
