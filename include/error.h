#ifndef _CXCORE_ERROR_H_
#define _CXCORE_ERROR_H_

#define VOS_StsOk                    0  /* everithing is ok                */
#define VOS_StsBackTrace            -1  /* pseudo error for back trace     */
#define VOS_StsError                -2  /* unknown /unspecified error      */
#define VOS_StsInternal             -3  /* internal error (bad state)      */
#define VOS_StsNoMem                -4  /* insufficient memory             */
#define VOS_StsBadArg               -5  /* function arg/param is bad       */
#define VOS_StsBadFunc              -6  /* unsupported function            */
#define VOS_StsAutoTrace            -8  /* tracing                         */

#define VOS_HeaderIsNull            -9  /* image header is NULL            */
#define VOS_BadImageSize            -10 /* image size is invalid           */
#define VOS_BadOffset               -11 /* offset is invalid               */
#define VOS_BadStep                 -13 /**/
#define VOS_BadNumChannels          -15 /**/
#define VOS_BadDepth                -17 /**/
#define VOS_BadOrder                -19 /**/
#define VOS_BadOrigin               -20 /**/
#define VOS_BadAlign                -21 /**/
#define VOS_BadCOI                  -24 /**/
#define VOS_BadROISize              -25 /**/
#define VOS_StsNullPtr                -27 /* null pointer */

#define VOS_StsBadSize                -201 /* the input/output structure size is incorrect  */
#define VOS_StsDivByZero              -202 /* division by zero */
#define VOS_StsUnmatchedFormats       -205 /* formats of input/output arrays differ */
#define VOS_StsBadFlag                -206 /* flag is wrong or not supported */  
#define VOS_StsUnmatchedSizes         -209 /* sizes of input/output structures do not match */
#define VOS_StsUnsupportedFormat      -210 /* the data format/type is not supported by the function*/
#define VOS_StsOutOfRange             -211 /* some of parameters are out of range */
#define VOS_StsNotImplemented         -213 /* the requested function/feature is not implemented */


#ifdef VOS_NO_FUNC_NAMES
    #define VOS_FUNCNAME( Name )
    #define sysFuncName ""
#else    
    #define VOS_FUNCNAME( Name )  \
    static char sysFuncName[] = Name
#endif


#define VOS_ERROR( Code, Msg )                                       \
{                                                                   \
     SysError( (Code), sysFuncName, Msg, __FILE__, __LINE__ );        \
     EXIT;                                                          \
}

/* Simplified form of VOS_ERROR */
#define VOS_ERROR_FROM_CODE( code )   \
    VOS_ERROR( code, "" )

#define VOS_CHECK()                                                  \
{                                                                   \
    if( GetErrStatus() < 0 )                                      \
        VOS_ERROR( VOS_StsBackTrace, "Inner function failed." );      \
}

#define VOS_CALL( Func )                                             \
{                                                                   \
    Func;                                                           \
    VOS_CHECK();                                                     \
}


/* Runtime assertion macro */
#define VOS_ASSERT( Condition )                                          \
{                                                                       \
    if( !(Condition) )                                                  \
        VOS_ERROR( VOS_StsInternal, "Assertion: " #Condition " failed" ); \
}

#define __BEGIN__       {
#define __END__         goto exit; exit: ; }
#define __CLEANUP__
#define EXIT            goto exit

#endif /* _CXCORE_ERROR_H_ */

/* End of file. */
