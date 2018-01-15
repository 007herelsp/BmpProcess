#ifndef _CXCORE_ERROR_H_
#define _CXCORE_ERROR_H_

#define VOS_StsOk 0            /* everithing is ok                */
#define VOS_StsBackTrace -1    /* pseudo error for back trace     */
#define VOS_StsError -2        /* unknown /unspecified error      */
#define VOS_StsInternal -3     /* internal error (bad state)      */
#define VOS_StsNoMem -4        /* insufficient memory             */
#define VOS_StsBadArg -5       /* function arg/param is bad       */
#define VOS_StsBadFunc -6      /* unsupported function            */
#define VOS_StsAutoTrace -8    /* tracing                         */
#define VOS_HeaderIsNull -9    /* image header is NULL            */
#define VOS_BadImageSize -10   /* image size is invalid           */
#define VOS_BadOffset -11      /* offset is invalid               */
#define VOS_BadStep -11        /**/
#define VOS_BadNumChannels -12 /**/
#define VOS_BadDepth -13       /**/
#define VOS_BadOrder -14       /**/
#define VOS_BadOrigin -15      /**/
#define VOS_BadAlign -16       /**/
#define VOS_BadCOI -17         /**/
#define VOS_BadROISize -18     /**/
#define VOS_StsNullPtr -19     /* null pointer */

#define VOS_StsBadSize -20           /* the input/output structure size is incorrect  */
#define VOS_StsDivByZero -21         /* division by zero */
#define VOS_StsUnmatchedFormats -22  /* formats of input/output arrays differ */
#define VOS_StsBadFlag -23           /* flag is wrong or not supported */
#define VOS_StsUnmatchedSizes -24    /* sizes of input/output structures do not match */
#define VOS_StsUnsupportedFormat -25 /* the data format/type is not supported by the function*/
#define VOS_StsOutOfRange -26        /* some of parameters are out of range */
#define VOS_StsNotImplemented -27    /* the requested function/feature is not implemented */
#define VOS_StsBUTT -28

#define VOS_FUNCNAME(Name) static const char sysFuncName[] = Name

#define VOS_ERROR(Code, Msg)                                    \
    \
{                                                          \
    SysError((Code), sysFuncName, Msg, __FILE__, __LINE__); \
    EXIT;                                                   \
    \
    }

#define VOS_ERROR_FROM_CODE(code) \
    VOS_ERROR(code, "")

#define VOS_CHECK()                                                \
    \
{                                                             \
    if (GetErrStatus() < 0)                                    \
    VOS_ERROR(VOS_StsBackTrace, "Inner function failed."); \
    \
    }

#define VOS_CALL(Func) \
    \
{                 \
    Func;          \
    VOS_CHECK();   \
    \
    }

#define VOS_ASSERT(Condition)                                               \
    \
{                                                                      \
    if (!(Condition))                                                   \
    VOS_ERROR(VOS_StsInternal, "Assertion: " #Condition " failed"); \
    \
    }


#define VOS_FUN_CALL(Func)                    \
    \
{                                        \
    int fun_call_result;                  \
    fun_call_result = Func;               \
    \
    if (fun_call_result < 0)              \
    VOS_ERROR((fun_call_result), ""); \
    \
    }

#define __BEGIN__ {
#define __END__ \
    goto exit;  \
    exit:;      \
    }
#define __CLEANUP__
#define EXIT goto exit

#endif /* _CXCORE_ERROR_H_ */

/* End of file. */
