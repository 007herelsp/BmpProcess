#include "core.h"
#include "misc.h"
#include <stdio.h>
typedef struct StackRecord
{
    const char *file;
    int line;
} StackRecord;

typedef struct Context
{
    int err_code;
    int err_mode;
    SysErrorCallback error_callback;
    void *userdata;
    char err_msg[4096];
    StackRecord err_ctx;
} Context;

#define VOS_DEFAULT_ERROR_CALLBACK StdErrReport

static Context *
iCreateContext(void)
{
    Context *context = (Context *)malloc(sizeof(*context));

    context->err_mode = VOS_ErrModeLeaf;
    context->err_code = VOS_StsOk;

    context->error_callback = VOS_DEFAULT_ERROR_CALLBACK;
    context->userdata = NULL;

    return context;
}

static void
iDestroyContext(Context *context)
{
    free(context);
}

static Context *
iGetContext(void)
{
    /* static single-thread library case */
    static Context *context = NULL;
    if (!context)
        context = iCreateContext();
    return context;
}

 int
StdErrReport(int code, const char *func_name, const char *err_msg,
               const char *file, int line, void *)
{
    if (code == VOS_StsBackTrace || code == VOS_StsAutoTrace)
        fprintf(stderr, "\tcalled from ");
    else
        fprintf(stderr, "ERROR: %s (%s)\n\tin function ",
                SysErrorStr(code), err_msg ? err_msg : "no description");

    fprintf(stderr, "%s, %s(%d)\n", func_name ? func_name : "<unknown>",
            file != NULL ? file : "", line);


        fprintf(stderr, "Terminating the application...\n");
        return 1;

}

 const char *SysErrorStr(int status)
{
    static char buf[256];

    switch (status)
    {
    case VOS_StsOk:
        return "No Error";
    case VOS_StsBackTrace:
        return "Backtrace";
    case VOS_StsError:
        return "Unspecified error";
    case VOS_StsInternal:
        return "Internal error";
    case VOS_StsNoMem:
        return "Insufficient memory";
    case VOS_StsBadArg:
        return "Bad argument";
    case VOS_StsAutoTrace:
        return "Autotrace call";
    case VOS_StsBadSize:
        return "Incorrect size of input array";
    case VOS_StsNullPtr:
        return "Null pointer";
    case VOS_StsDivByZero:
        return "Divizion by zero occured";
    case VOS_BadStep:
        return "Image step is wrong";
    case VOS_BadDepth:
        return "Input image depth is not supported by function";
    case VOS_StsUnmatchedFormats:
        return "Formats of input arguments do not match";
    case VOS_StsUnmatchedSizes:
        return "Sizes of input arguments do not match";
    case VOS_StsOutOfRange:
        return "One of arguments\' values is out of range";
    case VOS_StsUnsupportedFormat:
        return "Unsupported format or combination of formats";
    case VOS_BadCOI:
        return "Input COI is not supported";
    case VOS_BadNumChannels:
        return "Bad number of channels";
    case VOS_StsBadFlag:
        return "Bad flag (parameter or structure field)";

    case VOS_StsNotImplemented:
        return "The function/feature is not implemented";
    };

    sprintf(buf, "Unknown %s code %d", status >= 0 ? "status" : "error", status);
    return buf;
}



 int GetErrStatus()
{
    return iGetContext()->err_code;
}

 void SetErrStatus(int code)
{
    iGetContext()->err_code = code;
}

 void SysError(int code, const char *func_name,
                      const char *err_msg,
                      const char *file_name, int line)
{
    if (code == VOS_StsOk)
        SetErrStatus(code);
    else
    {
        Context *context = iGetContext();

        if (code != VOS_StsBackTrace && code != VOS_StsAutoTrace)
        {
            char *message = context->err_msg;
            context->err_code = code;

            strcpy(message, err_msg);
            context->err_ctx.file = file_name;
            context->err_ctx.line = line;
        }

            int terminate = context->error_callback(code, func_name, err_msg,
                                                    file_name, line, context->userdata);
            if (terminate)
            {
                exit(-abs(terminate));
            }
    }
}
/* End of file */
