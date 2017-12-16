#include "core.precomp.hpp"


#include <stdarg.h>

namespace cv
{

Exception::Exception() { code = 0; line = 0; }

Exception::Exception(int _code, const string& _err, const string& _func, const string& _file, int _line)
: code(_code), err(_err), func(_func), file(_file), line(_line)
{
    formatMessage();
}

Exception::~Exception() throw() {}

/*!
 \return the error description and the context as a text string.
 */
const char* Exception::what() const throw() { return msg.c_str(); }

void Exception::formatMessage()
{
    if( func.size() > 0 )
        msg = format("%s:%d: error: (%d) %s in function %s\n", file.c_str(), line, code, err.c_str(), func.c_str());
    else
        msg = format("%s:%d: error: (%d) %s\n", file.c_str(), line, code, err.c_str());
}

struct HWFeatures
{
    enum { MAX_FEATURE = CV_HARDWARE_MAX_FEATURE };

    HWFeatures(void)
     {
        memset( have, 0, sizeof(have) );
        x86_family = 0;
    }

    static HWFeatures initialize(void)
    {
        HWFeatures f;
        int cpuid_data[4] = { 0, 0, 0, 0 };

    #if defined _MSC_VER && (defined _M_IX86 || defined _M_X64)
        __cpuid(cpuid_data, 1);
    #elif defined __GNUC__ && (defined __i386__ || defined __x86_64__)
        #ifdef __x86_64__
        asm __volatile__
        (
         "movl $1, %%eax\n\t"
         "cpuid\n\t"
         :[eax]"=a"(cpuid_data[0]),[ebx]"=b"(cpuid_data[1]),[ecx]"=c"(cpuid_data[2]),[edx]"=d"(cpuid_data[3])
         :
         : "cc"
        );
        #else
        asm volatile
        (
         "pushl %%ebx\n\t"
         "movl $1,%%eax\n\t"
         "cpuid\n\t"
         "popl %%ebx\n\t"
         : "=a"(cpuid_data[0]), "=c"(cpuid_data[2]), "=d"(cpuid_data[3])
         :
         : "cc"
        );
        #endif
    #endif

        f.x86_family = (cpuid_data[0] >> 8) & 15;
        if( f.x86_family >= 6 )
        {
            f.have[CV_CPU_MMX]    = (cpuid_data[3] & (1 << 23)) != 0;
            f.have[CV_CPU_SSE]    = (cpuid_data[3] & (1<<25)) != 0;
            f.have[CV_CPU_SSE2]   = (cpuid_data[3] & (1<<26)) != 0;
            f.have[CV_CPU_SSE3]   = (cpuid_data[2] & (1<<0)) != 0;
            f.have[CV_CPU_SSSE3]  = (cpuid_data[2] & (1<<9)) != 0;
            f.have[CV_CPU_SSE4_1] = (cpuid_data[2] & (1<<19)) != 0;
            f.have[CV_CPU_SSE4_2] = (cpuid_data[2] & (1<<20)) != 0;
            f.have[CV_CPU_POPCNT] = (cpuid_data[2] & (1<<23)) != 0;
            f.have[CV_CPU_AVX]    = (((cpuid_data[2] & (1<<28)) != 0)&&((cpuid_data[2] & (1<<27)) != 0));//OS uses XSAVE_XRSTORE and CPU support AVX
        }

    #if defined _MSC_VER && (defined _M_IX86 || defined _M_X64)
        __cpuidex(cpuid_data, 7, 0);
    #elif defined __GNUC__ && (defined __i386__ || defined __x86_64__)
        #ifdef __x86_64__
        asm __volatile__
        (
         "movl $7, %%eax\n\t"
         "movl $0, %%ecx\n\t"
         "cpuid\n\t"
         :[eax]"=a"(cpuid_data[0]),[ebx]"=b"(cpuid_data[1]),[ecx]"=c"(cpuid_data[2]),[edx]"=d"(cpuid_data[3])
         :
         : "cc"
        );
        #else
        // We need to preserve ebx since we are compiling PIC code.
        // This means we cannot use "=b" for the 2nd output register.
        asm volatile
        (
         "pushl %%ebx\n\t"
         "movl $7,%%eax\n\t"
         "movl $0,%%ecx\n\t"
         "cpuid\n\t"
         "movl %%ebx,%1\n\t"
         "popl %%ebx\n\t"
         : "=a"(cpuid_data[0]), "=r"(cpuid_data[1]), "=c"(cpuid_data[2]), "=d"(cpuid_data[3])
         :
         : "cc"
        );
        #endif
    #endif

        if( f.x86_family >= 6 )
        {
            f.have[CV_CPU_AVX2] = (cpuid_data[1] & (1<<5)) != 0;
        }

        return f;
    }

    int x86_family;
    bool have[MAX_FEATURE+1];
};

static HWFeatures  featuresEnabled = HWFeatures::initialize(), featuresDisabled = HWFeatures();
static HWFeatures* currentFeatures = &featuresEnabled;




volatile bool useOptimizedFlag = true;

volatile bool USE_SSE2 = featuresEnabled.have[CV_CPU_SSE2];
volatile bool USE_SSE4_2 = featuresEnabled.have[CV_CPU_SSE4_2];
volatile bool USE_AVX = featuresEnabled.have[CV_CPU_AVX];

void setUseOptimized( bool flag )
{
    useOptimizedFlag = flag;
    currentFeatures = flag ? &featuresEnabled : &featuresDisabled;
    USE_SSE2 = currentFeatures->have[CV_CPU_SSE2];
}

bool useOptimized(void)
{
    return useOptimizedFlag;
}

int64 getTickCount(void)
{
	return 0;
}

double getTickFrequency(void)
{
	return 0;
}





string format( const char* fmt, ... )
{
    char buf[1 << 16];
    va_list args;
    va_start( args, fmt );
    vsprintf( buf, fmt, args );
    return string(buf);
}


static CvErrorCallback customErrorCallback = 0;
static void* customErrorCallbackData = 0;
static bool breakOnError = false;

bool setBreakOnError(bool value)
{
    bool prevVal = breakOnError;
    breakOnError = value;
    return prevVal;
}

void error( const Exception& exc )
{
    if (customErrorCallback != 0)
        customErrorCallback(exc.code, exc.func.c_str(), exc.err.c_str(),
                            exc.file.c_str(), exc.line, customErrorCallbackData);
    else
    {
        const char* errorStr = cvErrorStr(exc.code);
        char buf[1 << 16];

        sprintf( buf, "OpenCV Error: %s (%s) in %s, file %s, line %d",
            errorStr, exc.err.c_str(), exc.func.size() > 0 ?
            exc.func.c_str() : "unknown function", exc.file.c_str(), exc.line );
        fprintf( stderr, "%s\n", buf );
        fflush( stderr );

    }

    if(breakOnError)
    {
        static volatile int* p = 0;
        *p = 0;
    }

    throw exc;
}

CvErrorCallback
redirectError( CvErrorCallback errCallback, void* userdata, void** prevUserdata)
{
    if( prevUserdata )
        *prevUserdata = customErrorCallbackData;

    CvErrorCallback prevCallback = customErrorCallback;

    customErrorCallback     = errCallback;
    customErrorCallbackData = userdata;

    return prevCallback;
}

}



CV_IMPL int cvUseOptimized( int flag )
{
    int prevMode = cv::useOptimizedFlag;
    cv::setUseOptimized( flag != 0 );
    return prevMode;
}

CV_IMPL int64  cvGetTickCount(void)
{
    return cv::getTickCount();
}

CV_IMPL double cvGetTickFrequency(void)
{
    return cv::getTickFrequency()*1e-6;
}

CV_IMPL CvErrorCallback
cvRedirectError( CvErrorCallback errCallback, void* userdata, void** prevUserdata)
{
    return cv::redirectError(errCallback, userdata, prevUserdata);
}

CV_IMPL int cvNulDevReport( int, const char*, const char*,
                            const char*, int, void* )
{
    return 0;
}

CV_IMPL int cvStdErrReport( int, const char*, const char*,
                            const char*, int, void* )
{
    return 0;
}

CV_IMPL int cvGuiBoxReport( int, const char*, const char*,
                            const char*, int, void* )
{
    return 0;
}

CV_IMPL int cvGetErrInfo( const char**, const char**, const char**, int* )
{
    return 0;
}


CV_IMPL const char* cvErrorStr( int status )
{
    static char buf[256];

    switch (status)
    {
    case CV_StsOk :                  return "No Error";
    case CV_StsBackTrace :           return "Backtrace";
    case CV_StsError :               return "Unspecified error";
    case CV_StsInternal :            return "Internal error";
    case CV_StsNoMem :               return "Insufficient memory";
    case CV_StsBadArg :              return "Bad argument";
    case CV_StsNoConv :              return "Iterations do not converge";
    case CV_StsAutoTrace :           return "Autotrace call";
    case CV_StsBadSize :             return "Incorrect size of input array";
    case CV_StsNullPtr :             return "Null pointer";
    case CV_StsDivByZero :           return "Division by zero occured";
    case CV_BadStep :                return "Image step is wrong";
    case CV_StsInplaceNotSupported : return "Inplace operation is not supported";
    case CV_StsObjectNotFound :      return "Requested object was not found";
    case CV_BadDepth :               return "Input image depth is not supported by function";
    case CV_StsUnmatchedFormats :    return "Formats of input arguments do not match";
    case CV_StsUnmatchedSizes :      return "Sizes of input arguments do not match";
    case CV_StsOutOfRange :          return "One of arguments\' values is out of range";
    case CV_StsUnsupportedFormat :   return "Unsupported format or combination of formats";
    case CV_BadCOI :                 return "Input COI is not supported";
    case CV_BadNumChannels :         return "Bad number of channels";
    case CV_StsBadFlag :             return "Bad flag (parameter or structure field)";
    case CV_StsBadPoint :            return "Bad parameter of type CvPoint";
    case CV_StsBadMask :             return "Bad type of mask argument";
    case CV_StsParseError :          return "Parsing error";
    case CV_StsNotImplemented :      return "The function/feature is not implemented";
    case CV_StsBadMemBlock :         return "Memory block has been corrupted";
    case CV_StsAssert :              return "Assertion failed";
    case CV_GpuNotSupported :        return "No GPU support";
    case CV_GpuApiCallError :        return "Gpu API call";
    case CV_OpenGlNotSupported :     return "No OpenGL support";
    case CV_OpenGlApiCallError :     return "OpenGL API call";
    };

    sprintf(buf, "Unknown %s code %d", status >= 0 ? "status":"error", status);
    return buf;
}

CV_IMPL int cvGetErrMode(void)
{
    return 0;
}

CV_IMPL int cvSetErrMode(int)
{
    return 0;
}

CV_IMPL int cvGetErrStatus(void)
{
    return 0;
}

CV_IMPL void cvSetErrStatus(int)
{
}


CV_IMPL void cvError( int code, const char* func_name,
                      const char* err_msg,
                      const char* file_name, int line )
{
    cv::error(cv::Exception(code, err_msg, func_name, file_name, line));
}

/* function, which converts int to int */
CV_IMPL int
cvErrorFromIppStatus( int status )
{
    switch (status)
    {
    case CV_BADSIZE_ERR:               return CV_StsBadSize;
    case CV_BADMEMBLOCK_ERR:           return CV_StsBadMemBlock;
    case CV_NULLPTR_ERR:               return CV_StsNullPtr;
    case CV_DIV_BY_ZERO_ERR:           return CV_StsDivByZero;
    case CV_BADSTEP_ERR:               return CV_BadStep;
    case CV_OUTOFMEM_ERR:              return CV_StsNoMem;
    case CV_BADARG_ERR:                return CV_StsBadArg;
    case CV_NOTDEFINED_ERR:            return CV_StsError;
    case CV_INPLACE_NOT_SUPPORTED_ERR: return CV_StsInplaceNotSupported;
    case CV_NOTFOUND_ERR:              return CV_StsObjectNotFound;
    case CV_BADCONVERGENCE_ERR:        return CV_StsNoConv;
    case CV_BADDEPTH_ERR:              return CV_BadDepth;
    case CV_UNMATCHED_FORMATS_ERR:     return CV_StsUnmatchedFormats;
    case CV_UNSUPPORTED_COI_ERR:       return CV_BadCOI;
    case CV_UNSUPPORTED_CHANNELS_ERR:  return CV_BadNumChannels;
    case CV_BADFLAG_ERR:               return CV_StsBadFlag;
    case CV_BADRANGE_ERR:              return CV_StsBadArg;
    case CV_BADCOEF_ERR:               return CV_StsBadArg;
    case CV_BADFACTOR_ERR:             return CV_StsBadArg;
    case CV_BADPOINT_ERR:              return CV_StsBadPoint;

    default:
      return CV_StsError;
    }
}


/* End of file. */
