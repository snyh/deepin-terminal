#include <stdlib.h>
#include <cassert>
#include "context.h"



Matcher::Matcher(const char* fp, size_t size)
    :m_fp(std::vector<char>(fp, fp+size)), m_fp_size(size)
{
    Reset();
}

#include <stdio.h>
Matcher::Status Matcher::Check(char d)
{
    extern int log_fd;
    dprintf(log_fd, "%d ?=? %d\n", d, m_fp[m_pos]);
    if (d == m_fp[m_pos])
        enter_maybe();
    else
        enter_mustnot();
    if (m_pos == m_fp_size)
        enter_found();
    return Query();
}


const std::vector<char> StreamContext::Filter(const char* buf, size_t len)
{
    const std::vector<char> r = this->filter(buf, len);
    if (m_matcher->Query() == Matcher::Found && m_slot != 0) {
        m_slot(this);
    }
    return r;
}


const std::vector<char> StreamContext::filter(const char* buf, size_t len)
{
    std::vector<char> result;
    for (int i=0; i<len; i++) {
        switch(m_matcher->Check(buf[i])) {
        case Matcher::Found:
            //remainder data will be drop.
            m_cached.clear();
            return result;
        case Matcher::MustNot:
            result.insert(result.end(), m_cached.begin(), m_cached.end());
            result.push_back(buf[i]);
            m_cached.clear();
            break;
        case Matcher::Maybe:
            assert(m_cached.size() < sizeof(SZ_START_FP));
            m_cached.push_back(buf[i]);
            break;
        }
    }
    return result;
}
