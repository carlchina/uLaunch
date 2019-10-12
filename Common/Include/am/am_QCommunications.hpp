
#pragma once
#include <q_Include.hpp>

namespace am
{
    enum class QMenuStartMode
    {
        Invalid,
        StartupScreen,
        MenuNormal,
        MenuApplicationSuspended,
        MenuLaunchFailure
    };

    enum class QMenuMessage
    {
        Invalid,
        HomeRequest,
    };

    enum class QDaemonMessage
    {
        Invalid,
        SetSelectedUser,
        LaunchApplication,
        ResumeApplication,
        TerminateApplication,
        GetSuspendedApplicationId
    };

    Result QDaemon_LaunchQMenu(QMenuStartMode mode);

    ResultWith<QMenuStartMode> QMenu_ProcessInput();
    Result QMenu_InitializeDaemonService();
    ResultWith<QMenuMessage> QMenu_GetLatestQMenuMessage();
    void QMenu_FinalizeDaemonService();

    Result QDaemon_QMenuWriteImpl(void *data, size_t size, bool wait);
    Result QDaemon_QMenuReadImpl(void *data, size_t size, bool wait);

    Result QMenu_QDaemonWriteImpl(void *data, size_t size, bool wait);
    Result QMenu_QDaemonReadImpl(void *data, size_t size, bool wait);

    typedef Result(*QCommandRWFunction)(void*, size_t, bool);

    struct QCommandCommonHeader
    {
        u32 magic;
        u32 val;
    };

    static constexpr u32 Magic = 0xAABBCCDD;
    static constexpr size_t BlockSize = 0x400;

    template<QCommandRWFunction WriteFn>
    class QCommandWriter
    {
        private:
            QCommandCommonHeader request;
            u8 *data_block;
            size_t data_pos;
            Result inner_rc;
            bool write_done;

        public:
            QCommandWriter(u32 value)
            {
                write_done = false;
                request.magic = Magic;
                request.val = value;
                data_pos = 0;
                data_block = new u8[BlockSize]();
                inner_rc = WriteFn(&request, sizeof(request), false);
            }

            ~QCommandWriter()
            {
                FinishWrite();
            }

            void FinishWrite()
            {
                if(!write_done)
                {
                    WriteFn(data_block, BlockSize, false);
                    delete[] data_block;
                    write_done = true;
                }
            }

            Result GetResult()
            {
                return inner_rc;
            }

            operator bool()
            {
                return R_SUCCEEDED(inner_rc);
            }

            template<typename T>
            void Write(T t)
            {
                memcpy(&data_block[data_pos], &t, sizeof(T));
                data_pos += sizeof(T);
            }
    };

    template<QCommandRWFunction ReadFn>
    class QCommandReader
    {
        private:
            QCommandCommonHeader response;
            u8 *data_block;
            size_t data_pos;
            Result inner_rc;
            bool read_done;
            bool fn_wait;

        public:
            QCommandReader(bool wait = false)
            {
                fn_wait = wait;
                read_done = false;
                data_pos = 0;
                data_block = new u8[BlockSize]();
                inner_rc = ReadFn(&response, sizeof(QCommandCommonHeader), fn_wait);
                if(R_SUCCEEDED(inner_rc)) inner_rc = ReadFn(data_block, BlockSize, fn_wait);
            }

            ~QCommandReader()
            {
                FinishRead();
            }

            void FinishRead()
            {
                if(!read_done)
                {
                    delete[] data_block;
                    read_done = true;
                }
            }

            u32 GetValue()
            {
                return response.val;
            }

            Result GetResult()
            {
                return inner_rc;
            }

            operator bool()
            {
                return R_SUCCEEDED(inner_rc);
            }

            template<typename T>
            T Read()
            {
                T t = *(T*)&data_block[data_pos];
                data_pos += sizeof(T);
                return t;
            }
    };

    class QDaemonCommandReader : public QCommandReader<QDaemon_QMenuReadImpl>
    {
        public:
            QDaemonMessage GetMessage()
            {
                return (QDaemonMessage)GetValue();
            }
    };

    class QDaemonCommandWriter : public QCommandWriter<QDaemon_QMenuWriteImpl>
    {
        public:
            QDaemonCommandWriter(QMenuMessage msg) : QCommandWriter((u32)msg)
            {
            }
    };

    class QDaemonCommandResultReader : public QCommandReader<QDaemon_QMenuReadImpl>
    {
        public:
            QDaemonCommandResultReader() : QCommandReader(true)
            {
            }

            Result GetReadResult()
            {
                return GetValue();
            }
    };

    class QDaemonCommandResultWriter : public QCommandWriter<QDaemon_QMenuWriteImpl>
    {
        public:
            QDaemonCommandResultWriter(Result rc) : QCommandWriter(rc)
            {
            }
    };

    class QMenuCommandReader : public QCommandReader<QMenu_QDaemonReadImpl>
    {
        public:
            QMenuMessage GetMessage()
            {
                return (QMenuMessage)GetValue();
            }
    };

    class QMenuCommandWriter : public QCommandWriter<QMenu_QDaemonWriteImpl>
    {
        public:
            QMenuCommandWriter(QDaemonMessage msg) : QCommandWriter((u32)msg)
            {
            }
    };

    class QMenuCommandResultReader : public QCommandReader<QMenu_QDaemonReadImpl>
    {
        public:
            QMenuCommandResultReader() : QCommandReader(true)
            {
            }

            Result GetReadResult()
            {
                return GetValue();
            }
    };

    class QMenuCommandResultWriter : public QCommandWriter<QMenu_QDaemonWriteImpl>
    {
        public:
            QMenuCommandResultWriter(Result rc) : QCommandWriter(rc)
            {
            }
    };
}