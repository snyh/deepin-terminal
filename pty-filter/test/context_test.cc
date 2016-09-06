#include <CppUTest/TestHarness.h>
#include <unistd.h>
#include <glib-unix.h>

#include "global.h"
#include "context.h"


TEST_GROUP(InputFilter)
{
};

static bool found = false;
void record_result(StreamContext* ctx)
{
    found = true;
    quit_loop();
}

static char sz_data[] = {0162,  0172,  040,  0167,  0141,  0151,  0164,  0151,
                    0156,  0147,  040,  0164,  0157,  040,  0162,  0145,
                    0143,  0145,  0151,  0166,  0145,  056,  052,  052,
                    030,  0102,  060,  061,  060,  060,  060,  060,  060,
                    060,  062,  063,  0142,  0145,  065,  060,  015
};
    
gboolean feed_data(gpointer _fd)
{
    int fd = GPOINTER_TO_INT(_fd);
    
    ssize_t r = write(fd, sz_data, sizeof(sz_data));

    CHECK_EQUAL(r, sizeof(sz_data));

    return FALSE;
}

TEST(InputFilter, Matcher)
{
    Matcher* m = new Matcher(RZ_START_FP, sizeof(RZ_START_FP));
    for (int i=0; i<sizeof(sz_data); i++) {
        if (m->Check(sz_data[i]) == Matcher::Found) {
            delete m;
            return;
        }
    }
    FAIL("Should be found");
}

TEST(InputFilter, Check)
{
    Matcher* matcher = new Matcher(RZ_START_FP, sizeof(RZ_START_FP));
    StreamContext* SZ = new StreamContext(matcher);
    SZ->connect(record_result);

    std::vector<StreamContext*> m;
    m.push_back(SZ);

    int fds[2] = {-1, -1}; 
    if (pipe(fds)) {
        FAIL("can't create pipe")
    }

    FilterContext* ctx = new FilterContext;
    ctx->outFd = 1;
    ctx->filters = m;

    g_unix_fd_add(fds[0], G_IO_IN, (GUnixFDSourceFunc)filter_input, ctx);

    g_idle_add(feed_data, GINT_TO_POINTER(fds[1]));
    
    run_loop();

    close(fds[0]);
    close(fds[1]);
    delete ctx;
    delete SZ;
    delete matcher;
    
    CHECK_EQUAL(found, true);
}
