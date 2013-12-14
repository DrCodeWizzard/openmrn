#include <stdlib.h>

#include "utils/test_main.hxx"

#include "executor/allocator.hxx"
#include "executor/executor.hxx"
#include "nmranet_can.h"
#include "nmranet/ReadDispatch.hxx"

static void InvokeNotification(Notifiable* done)
{
    done->Notify();
}

using testing::Invoke;
using testing::StrictMock;
using testing::WithArg;
using testing::Return;
using testing::_;

namespace NMRAnet
{

typedef TypedDispatchFlow<uint32_t, struct can_frame> CanDispatchFlow;
typedef ParamHandler<struct can_frame> CanFrameHandler;

class MockCanFrameHandler : public CanFrameHandler
{
public:
    MOCK_METHOD0(get_allocator, AllocatorBase*());
    MOCK_METHOD2(handle_message,
                 void(struct can_frame* message, Notifiable* done));
};

class DispatcherTest : public ::testing::Test
{
protected:
    DispatcherTest() : f(&g_executor)
    {
    }

    ~DispatcherTest()
    {
        WaitForExecutor();
    }

    void WaitForExecutor()
    {
        while (!g_executor.empty()) {
            usleep(100);
        }
    }

    CanDispatchFlow f;
};

TEST_F(DispatcherTest, TestCreateDestroyEmptyRun)
{
    TypedSyncAllocation<CanDispatchFlow> alloc(f.allocator());
    alloc.result()->IncomingMessage(0);

    TypedSyncAllocation<CanDispatchFlow> b_alloc(f.allocator());
    alloc.result()->IncomingMessage(1);
}

TEST_F(DispatcherTest, TestAddAndNotCall)
{
    StrictMock<MockCanFrameHandler> h1;
    f.RegisterHandler(1, 0xFFFFFFFFUL, &h1);
    f.UnregisterHandler(1, 0xFFFFFFFFUL, &h1);
    f.RegisterHandler(1, 0xFFFFFFFFUL, &h1);

    TypedSyncAllocation<CanDispatchFlow> alloc(f.allocator());
    alloc.result()->IncomingMessage(0);
}

TEST_F(DispatcherTest, TestAddAndCall)
{
    StrictMock<MockCanFrameHandler> h1;
    f.RegisterHandler(1, 0xFFFFFFFFUL, &h1);

    EXPECT_CALL(h1, get_allocator()).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(h1, handle_message(_, _)).WillOnce(
        WithArg<1>(Invoke(&InvokeNotification)));

    TypedSyncAllocation<CanDispatchFlow> alloc(f.allocator());
    alloc.result()->IncomingMessage(1);

    TypedSyncAllocation<CanDispatchFlow> alloc2(f.allocator());
}

TEST_F(DispatcherTest, TestCallWithMask)
{
    StrictMock<MockCanFrameHandler> h1;
    f.RegisterHandler(1, 0xFFUL, &h1);

    EXPECT_CALL(h1, get_allocator()).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(h1, handle_message(_, _)).WillOnce(
        WithArg<1>(Invoke(&InvokeNotification)));

    TypedSyncAllocation<CanDispatchFlow> alloc(f.allocator());
    alloc.result()->IncomingMessage(257);

    TypedSyncAllocation<CanDispatchFlow> alloc2(f.allocator());
    alloc.result()->IncomingMessage(2);

    TypedSyncAllocation<CanDispatchFlow> alloc3(f.allocator());
}

TEST_F(DispatcherTest, TestMultiplehandlers)
{
    StrictMock<MockCanFrameHandler> h1;
    f.RegisterHandler(1, 0xFFUL, &h1);
    StrictMock<MockCanFrameHandler> h2;
    f.RegisterHandler(257, 0xFFFFFFFFUL, &h2);
    StrictMock<MockCanFrameHandler> h3;
    f.RegisterHandler(2, 0xFFUL, &h3);

    EXPECT_CALL(h1, get_allocator()).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(h1, handle_message(_, _)).Times(2).WillRepeatedly(
        WithArg<1>(Invoke(&InvokeNotification)));

    EXPECT_CALL(h2, get_allocator()).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(h2, handle_message(_, _)).Times(1).WillRepeatedly(
        WithArg<1>(Invoke(&InvokeNotification)));

    EXPECT_CALL(h3, get_allocator()).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(h3, handle_message(_, _)).Times(1).WillRepeatedly(
        WithArg<1>(Invoke(&InvokeNotification)));

    TypedSyncAllocation<CanDispatchFlow> alloc(f.allocator());
    alloc.result()->IncomingMessage(257);

    TypedSyncAllocation<CanDispatchFlow> alloc2(f.allocator());
    alloc.result()->IncomingMessage(2);

    TypedSyncAllocation<CanDispatchFlow> alloc3(f.allocator());
    alloc.result()->IncomingMessage(1);

    TypedSyncAllocation<CanDispatchFlow> alloc4(f.allocator());
}

TEST_F(DispatcherTest, TestAsync)
{
    StrictMock<MockCanFrameHandler> h1;
    StrictMock<MockCanFrameHandler> halloc;
    TypedAllocator<CanFrameHandler> alloc;
    alloc.Release(&h1);

    f.RegisterHandler(1, 0xFFUL, &halloc);

    EXPECT_CALL(halloc, get_allocator()).WillRepeatedly(Return(&alloc));

    EXPECT_CALL(h1, get_allocator()).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(h1, handle_message(_, _)).Times(1).WillRepeatedly(
        WithArg<1>(Invoke(&InvokeNotification)));

    TypedSyncAllocation<CanDispatchFlow> salloc(f.allocator());
    salloc.result()->IncomingMessage(257);

    TypedSyncAllocation<CanDispatchFlow> salloc2(f.allocator());
    salloc.result()->IncomingMessage(1);
    WaitForExecutor();

    EXPECT_CALL(h1, handle_message(_, _)).Times(1).WillRepeatedly(
        WithArg<1>(Invoke(&InvokeNotification)));
    alloc.Release(&h1);  // will release message for second call.
    WaitForExecutor();
}

TEST_F(DispatcherTest, TestParams)
{
    StrictMock<MockCanFrameHandler> h1;
    f.RegisterHandler(1, 0xFFFFFFFFUL, &h1);

    struct can_frame* frame = f.mutable_params();
    // Checks that the same pointer is sent to the argument.
    EXPECT_CALL(h1, get_allocator()).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(h1, handle_message(frame, _)).WillOnce(
        WithArg<1>(Invoke(&InvokeNotification)));

    TypedSyncAllocation<CanDispatchFlow> alloc(f.allocator());
    alloc.result()->IncomingMessage(1);

    TypedSyncAllocation<CanDispatchFlow> alloc2(f.allocator());
}

TEST_F(DispatcherTest, TestUnregister)
{
    StrictMock<MockCanFrameHandler> h1;
    f.RegisterHandler(1, 0xFFUL, &h1);

    EXPECT_CALL(h1, get_allocator()).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(h1, handle_message(_, _)).WillOnce(
        WithArg<1>(Invoke(&InvokeNotification)));

    TypedSyncAllocation<CanDispatchFlow> alloc(f.allocator());
    alloc.result()->IncomingMessage(257);

    TypedSyncAllocation<CanDispatchFlow> alloc2(f.allocator());
    f.UnregisterHandler(1, 0xFFUL, &h1);
    alloc.result()->IncomingMessage(1);

    TypedSyncAllocation<CanDispatchFlow> alloc3(f.allocator());
}

} // namespace NMRAnet
