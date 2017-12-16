
// */

#include "core.precomp.hpp"

namespace cv
{

class MatOp_Identity : public MatOp
{
public:
    MatOp_Identity() {}
    virtual ~MatOp_Identity() {}

    bool elementWise(const MatExpr& /*expr*/) const { return true; }
    void assign(const MatExpr& expr, Mat& m, int type=-1) const;

    static void makeExpr(MatExpr& res, const Mat& m);
};

static MatOp_Identity g_MatOp_Identity;

class MatOp_AddEx : public MatOp
{
public:
    MatOp_AddEx() {}
    virtual ~MatOp_AddEx() {}

    bool elementWise(const MatExpr& /*expr*/) const { return true; }
    void assign(const MatExpr& expr, Mat& m, int type=-1) const;

    void add(const MatExpr& e1, const Scalar& s, MatExpr& res) const;
    void subtract(const Scalar& s, const MatExpr& expr, MatExpr& res) const;
    void multiply(const MatExpr& e1, double s, MatExpr& res) const;
    void divide(double s, const MatExpr& e, MatExpr& res) const;

    void transpose(const MatExpr& e1, MatExpr& res) const;
    void abs(const MatExpr& expr, MatExpr& res) const;

    static void makeExpr(MatExpr& res, const Mat& a, const Mat& b, double alpha, double beta, const Scalar& s=Scalar());
};

static MatOp_AddEx g_MatOp_AddEx;

class MatOp_Bin : public MatOp
{
public:
    MatOp_Bin() {}
    virtual ~MatOp_Bin() {}

    bool elementWise(const MatExpr& /*expr*/) const { return true; }
    void assign(const MatExpr& expr, Mat& m, int type=-1) const;

    void multiply(const MatExpr& e1, double s, MatExpr& res) const;
    void divide(double s, const MatExpr& e, MatExpr& res) const;

    static void makeExpr(MatExpr& res, char op, const Mat& a, const Mat& b, double scale=1);
    static void makeExpr(MatExpr& res, char op, const Mat& a, const Scalar& s);
};

static MatOp_Bin g_MatOp_Bin;

class MatOp_Cmp : public MatOp
{
public:
    MatOp_Cmp() {}
    virtual ~MatOp_Cmp() {}

    bool elementWise(const MatExpr& /*expr*/) const { return true; }
    void assign(const MatExpr& expr, Mat& m, int type=-1) const;

    static void makeExpr(MatExpr& res, int cmpop, const Mat& a, const Mat& b);
    static void makeExpr(MatExpr& res, int cmpop, const Mat& a, double alpha);
};

static MatOp_Cmp g_MatOp_Cmp;

class MatOp_T : public MatOp
{
public:
    MatOp_T() {}
    virtual ~MatOp_T() {}

    bool elementWise(const MatExpr& /*expr*/) const { return false; }
    void assign(const MatExpr& expr, Mat& m, int type=-1) const;

    void multiply(const MatExpr& e1, double s, MatExpr& res) const;
    void transpose(const MatExpr& expr, MatExpr& res) const;

    static void makeExpr(MatExpr& res, const Mat& a, double alpha=1);
};

static MatOp_T g_MatOp_T;



 

static inline bool isIdentity(const MatExpr& e) { return e.op == &g_MatOp_Identity; }
static inline bool isAddEx(const MatExpr& e) { return e.op == &g_MatOp_AddEx; }
static inline bool isScaled(const MatExpr& e) { return isAddEx(e) && (!e.b.data || e.beta == 0) && e.s == Scalar(); }
static inline bool isBin(const MatExpr& e, char c) { return e.op == &g_MatOp_Bin && e.flags == c; }
static inline bool isCmp(const MatExpr& e) { return e.op == &g_MatOp_Cmp; }
static inline bool isReciprocal(const MatExpr& e) { return isBin(e,'/') && (!e.b.data || e.beta == 0); }
static inline bool isT(const MatExpr& e) { return e.op == &g_MatOp_T; }

/////////////////////////////////////////////////////////////////////////////////////////////////////

bool MatOp::elementWise(const MatExpr& /*expr*/) const
{
    return false;
}

void MatOp::roi(const MatExpr& expr, const Range& rowRange, const Range& colRange, MatExpr& e) const
{
    if( elementWise(expr) )
    {
        e = MatExpr(expr.op, expr.flags, Mat(), Mat(), Mat(),
                    expr.alpha, expr.beta, expr.s);
        if(expr.a.data)
            e.a = expr.a(rowRange, colRange);
        if(expr.b.data)
            e.b = expr.b(rowRange, colRange);
        if(expr.c.data)
            e.c = expr.c(rowRange, colRange);
    }
    else
    {
        Mat m;
        expr.op->assign(expr, m);
        e = MatExpr(&g_MatOp_Identity, 0, m(rowRange, colRange), Mat(), Mat());
    }
}

void MatOp::diag(const MatExpr& expr, int d, MatExpr& e) const
{
    if( elementWise(expr) )
    {
        e = MatExpr(expr.op, expr.flags, Mat(), Mat(), Mat(),
                    expr.alpha, expr.beta, expr.s);
        if(expr.a.data)
            e.a = expr.a.diag(d);
        if(expr.b.data)
            e.b = expr.b.diag(d);
        if(expr.c.data)
            e.c = expr.c.diag(d);
    }
    else
    {
        Mat m;
        expr.op->assign(expr, m);
        e = MatExpr(&g_MatOp_Identity, 0, m.diag(d), Mat(), Mat());
    }
}


void MatOp::augAssignAdd(const MatExpr& expr, Mat& m) const
{
    Mat temp;
    expr.op->assign(expr, temp);
    m += temp;
}


void MatOp::augAssignSubtract(const MatExpr& expr, Mat& m) const
{
    Mat temp;
    expr.op->assign(expr, temp);
    m -= temp;
}


void MatOp::augAssignMultiply(const MatExpr& expr, Mat& m) const
{
    Mat temp;
    expr.op->assign(expr, temp);
    m *= temp;
}


void MatOp::augAssignDivide(const MatExpr& expr, Mat& m) const
{
    Mat temp;
    expr.op->assign(expr, temp);
    m /= temp;
}


void MatOp::augAssignAnd(const MatExpr& expr, Mat& m) const
{
    Mat temp;
    expr.op->assign(expr, temp);
    m &= temp;
}


void MatOp::augAssignOr(const MatExpr& expr, Mat& m) const
{
    Mat temp;
    expr.op->assign(expr, temp);
    m |= temp;
}


void MatOp::augAssignXor(const MatExpr& expr, Mat& m) const
{
    Mat temp;
    expr.op->assign(expr, temp);
    m ^= temp;
}


void MatOp::add(const MatExpr& e1, const MatExpr& e2, MatExpr& res) const
{
    if( this == e2.op )
    {
        double alpha = 1, beta = 1;
        Scalar s;
        Mat m1, m2;
        if( isAddEx(e1) && (!e1.b.data || e1.beta == 0) )
        {
            m1 = e1.a;
            alpha = e1.alpha;
            s = e1.s;
        }
        else
            e1.op->assign(e1, m1);

        if( isAddEx(e2) && (!e2.b.data || e2.beta == 0) )
        {
            m2 = e2.a;
            beta = e2.alpha;
            s += e2.s;
        }
        else
            e2.op->assign(e2, m2);
        MatOp_AddEx::makeExpr(res, m1, m2, alpha, beta, s);
    }
    else
        e2.op->add(e1, e2, res);
}


void MatOp::add(const MatExpr& expr1, const Scalar& s, MatExpr& res) const
{
    Mat m1;
    expr1.op->assign(expr1, m1);
    MatOp_AddEx::makeExpr(res, m1, Mat(), 1, 0, s);
}


void MatOp::subtract(const MatExpr& e1, const MatExpr& e2, MatExpr& res) const
{
    if( this == e2.op )
    {
        double alpha = 1, beta = -1;
        Scalar s;
        Mat m1, m2;
        if( isAddEx(e1) && (!e1.b.data || e1.beta == 0) )
        {
            m1 = e1.a;
            alpha = e1.alpha;
            s = e1.s;
        }
        else
            e1.op->assign(e1, m1);

        if( isAddEx(e2) && (!e2.b.data || e2.beta == 0) )
        {
            m2 = e2.a;
            beta = -e2.alpha;
            s -= e2.s;
        }
        else
            e2.op->assign(e2, m2);
        MatOp_AddEx::makeExpr(res, m1, m2, alpha, beta, s);
    }
    else
        e2.op->subtract(e1, e2, res);
}


void MatOp::subtract(const Scalar& s, const MatExpr& expr, MatExpr& res) const
{
    Mat m;
    expr.op->assign(expr, m);
    MatOp_AddEx::makeExpr(res, m, Mat(), -1, 0, s);
}


void MatOp::multiply(const MatExpr& e1, const MatExpr& e2, MatExpr& res, double scale) const
{
    if( this == e2.op )
    {
        Mat m1, m2;

        if( isReciprocal(e1) )
        {
            if( isScaled(e2) )
            {
                scale *= e2.alpha;
                m2 = e2.a;
            }
            else
                e2.op->assign(e2, m2);

            MatOp_Bin::makeExpr(res, '/', m2, e1.a, scale/e1.alpha);
        }
        else
        {
            char op = '*';
            if( isScaled(e1) )
            {
                m1 = e1.a;
                scale *= e1.alpha;
            }
            else
                e1.op->assign(e1, m1);

            if( isScaled(e2) )
            {
                m2 = e2.a;
                scale *= e2.alpha;
            }
            else if( isReciprocal(e2) )
            {
                op = '/';
                m2 = e2.a;
                scale /= e2.alpha;
            }
            else
                e2.op->assign(e2, m2);

            MatOp_Bin::makeExpr(res, op, m1, m2, scale);
        }
    }
    else
        e2.op->multiply(e1, e2, res, scale);
}


void MatOp::multiply(const MatExpr& expr, double s, MatExpr& res) const
{
    Mat m;
    expr.op->assign(expr, m);
    MatOp_AddEx::makeExpr(res, m, Mat(), s, 0);
}


void MatOp::divide(const MatExpr& e1, const MatExpr& e2, MatExpr& res, double scale) const
{
    if( this == e2.op )
    {
        if( isReciprocal(e1) && isReciprocal(e2) )
            MatOp_Bin::makeExpr(res, '/', e2.a, e1.a, e1.alpha/e2.alpha);
        else
        {
            Mat m1, m2;
            char op = '/';

            if( isScaled(e1) )
            {
                m1 = e1.a;
                scale *= e1.alpha;
            }
            else
                e1.op->assign(e1, m1);

            if( isScaled(e2) )
            {
                m2 = e2.a;
                scale /= e2.alpha;
            }
            else if( isReciprocal(e2) )
            {
                m2 = e2.a;
                scale /= e2.alpha;
                op = '*';
            }
            else
                e2.op->assign(e2, m2);
            MatOp_Bin::makeExpr(res, op, m1, m2, scale);
        }
    }
    else
        e2.op->divide(e1, e2, res, scale);
}


void MatOp::divide(double s, const MatExpr& expr, MatExpr& res) const
{
    Mat m;
    expr.op->assign(expr, m);
    MatOp_Bin::makeExpr(res, '/', m, Mat(), s);
}


void MatOp::abs(const MatExpr& expr, MatExpr& res) const
{
    Mat m;
    expr.op->assign(expr, m);
    MatOp_Bin::makeExpr(res, 'a', m, Mat());
}


void MatOp::transpose(const MatExpr& expr, MatExpr& res) const
{
    Mat m;
    expr.op->assign(expr, m);
    MatOp_T::makeExpr(res, m, 1);
}


void MatOp::matmul(const MatExpr& e1, const MatExpr& e2, MatExpr& res) const
{
    if( this == e2.op )
    {
        double scale = 1;
        int flags = 0;
        Mat m1, m2;

        if( isT(e1) )
        {
            flags = CV_GEMM_A_T;
            scale = e1.alpha;
            m1 = e1.a;
        }
        else if( isScaled(e1) )
        {
            scale = e1.alpha;
            m1 = e1.a;
        }
        else
            e1.op->assign(e1, m1);

        if( isT(e2) )
        {
            flags |= CV_GEMM_B_T;
            scale *= e2.alpha;
            m2 = e2.a;
        }
        else if( isScaled(e2) )
        {
            scale *= e2.alpha;
            m2 = e2.a;
        }
        else
            e2.op->assign(e2, m2);

    }
    else
        e2.op->matmul(e1, e2, res);
}


void MatOp::invert(const MatExpr& expr, int method, MatExpr& res) const
{
    Mat m;
    expr.op->assign(expr, m);

}


Size MatOp::size(const MatExpr& expr) const
{
    return !expr.a.empty() ? expr.a.size() : expr.b.empty() ? expr.b.size() : expr.c.size();
}

int MatOp::type(const MatExpr& expr) const
{
    return !expr.a.empty() ? expr.a.type() : expr.b.empty() ? expr.b.type() : expr.c.type();
}

//////////////////////////////////////////////////////////////////////////////////////////////////


MatExpr operator + (const MatExpr& e, const Scalar& s)
{
    MatExpr en;
    e.op->add(e, s, en);
    return en;
}

MatExpr operator + (const Scalar& s, const MatExpr& e)
{
    MatExpr en;
    e.op->add(e, s, en);
    return en;
}

MatExpr operator + (const MatExpr& e1, const MatExpr& e2)
{
    MatExpr en;
    e1.op->add(e1, e2, en);
    return en;
}

MatExpr operator - (const Mat& a, const Mat& b)
{
    MatExpr e;
    MatOp_AddEx::makeExpr(e, a, b, 1, -1);
    return e;
}

MatExpr operator - (const Mat& a, const Scalar& s)
{
    MatExpr e;
    MatOp_AddEx::makeExpr(e, a, Mat(), 1, 0, -s);
    return e;
}

MatExpr operator - (const Scalar& s, const Mat& a)
{
    MatExpr e;
    MatOp_AddEx::makeExpr(e, a, Mat(), -1, 0, s);
    return e;
}

MatExpr operator - (const MatExpr& e, const Scalar& s)
{
    MatExpr en;
    e.op->add(e, -s, en);
    return en;
}

MatExpr operator - (const Scalar& s, const MatExpr& e)
{
    MatExpr en;
    e.op->subtract(s, e, en);
    return en;
}

MatExpr operator - (const MatExpr& e1, const MatExpr& e2)
{
    MatExpr en;
    e1.op->subtract(e1, e2, en);
    return en;
}

MatExpr operator - (const Mat& m)
{
    MatExpr e;
    MatOp_AddEx::makeExpr(e, m, Mat(), -1, 0);
    return e;
}

MatExpr operator - (const MatExpr& e)
{
    MatExpr en;
    e.op->subtract(Scalar(0), e, en);
    return en;
}

MatExpr operator * (const Mat& a, const Mat& b)
{
    MatExpr e;
    return e;
}

MatExpr operator * (const Mat& a, double s)
{
    MatExpr e;
    MatOp_AddEx::makeExpr(e, a, Mat(), s, 0);
    return e;
}

MatExpr operator * (double s, const Mat& a)
{
    MatExpr e;
    MatOp_AddEx::makeExpr(e, a, Mat(), s, 0);
    return e;
}


MatExpr operator * (const MatExpr& e, double s)
{
    MatExpr en;
    e.op->multiply(e, s, en);
    return en;
}

MatExpr operator * (double s, const MatExpr& e)
{
    MatExpr en;
    e.op->multiply(e, s, en);
    return en;
}

MatExpr operator * (const MatExpr& e1, const MatExpr& e2)
{
    MatExpr en;
    e1.op->matmul(e1, e2, en);
    return en;
}

MatExpr operator / (const Mat& a, const Mat& b)
{
    MatExpr e;
    MatOp_Bin::makeExpr(e, '/', a, b);
    return e;
}

MatExpr operator / (const Mat& a, double s)
{
    MatExpr e;
    MatOp_AddEx::makeExpr(e, a, Mat(), 1./s, 0);
    return e;
}

MatExpr operator / (double s, const Mat& a)
{
    MatExpr e;
    MatOp_Bin::makeExpr(e, '/', a, Mat(), s);
    return e;
}


MatExpr operator / (const MatExpr& e, double s)
{
    MatExpr en;
    e.op->multiply(e, 1./s, en);
    return en;
}

MatExpr operator / (double s, const MatExpr& e)
{
    MatExpr en;
    e.op->divide(s, e, en);
    return en;
}

MatExpr operator / (const MatExpr& e1, const MatExpr& e2)
{
    MatExpr en;
    e1.op->divide(e1, e2, en);
    return en;
}

MatExpr operator < (const Mat& a, const Mat& b)
{
    MatExpr e;
    MatOp_Cmp::makeExpr(e, CV_CMP_LT, a, b);
    return e;
}

MatExpr operator < (const Mat& a, double s)
{
    MatExpr e;
    MatOp_Cmp::makeExpr(e, CV_CMP_LT, a, s);
    return e;
}

MatExpr operator < (double s, const Mat& a)
{
    MatExpr e;
    MatOp_Cmp::makeExpr(e, CV_CMP_GT, a, s);
    return e;
}

MatExpr operator <= (const Mat& a, const Mat& b)
{
    MatExpr e;
    MatOp_Cmp::makeExpr(e, CV_CMP_LE, a, b);
    return e;
}

MatExpr operator <= (const Mat& a, double s)
{
    MatExpr e;
    MatOp_Cmp::makeExpr(e, CV_CMP_LE, a, s);
    return e;
}

MatExpr operator <= (double s, const Mat& a)
{
    MatExpr e;
    MatOp_Cmp::makeExpr(e, CV_CMP_GE, a, s);
    return e;
}

MatExpr operator == (const Mat& a, const Mat& b)
{
    MatExpr e;
    MatOp_Cmp::makeExpr(e, CV_CMP_EQ, a, b);
    return e;
}

MatExpr operator == (const Mat& a, double s)
{
    MatExpr e;
    MatOp_Cmp::makeExpr(e, CV_CMP_EQ, a, s);
    return e;
}

MatExpr operator == (double s, const Mat& a)
{
    MatExpr e;
    MatOp_Cmp::makeExpr(e, CV_CMP_EQ, a, s);
    return e;
}

MatExpr operator != (const Mat& a, const Mat& b)
{
    MatExpr e;
    MatOp_Cmp::makeExpr(e, CV_CMP_NE, a, b);
    return e;
}

MatExpr operator != (const Mat& a, double s)
{
    MatExpr e;
    MatOp_Cmp::makeExpr(e, CV_CMP_NE, a, s);
    return e;
}

MatExpr operator != (double s, const Mat& a)
{
    MatExpr e;
    MatOp_Cmp::makeExpr(e, CV_CMP_NE, a, s);
    return e;
}

MatExpr operator >= (const Mat& a, const Mat& b)
{
    MatExpr e;
    MatOp_Cmp::makeExpr(e, CV_CMP_GE, a, b);
    return e;
}

MatExpr operator >= (const Mat& a, double s)
{
    MatExpr e;
    MatOp_Cmp::makeExpr(e, CV_CMP_GE, a, s);
    return e;
}

MatExpr operator >= (double s, const Mat& a)
{
    MatExpr e;
    MatOp_Cmp::makeExpr(e, CV_CMP_LE, a, s);
    return e;
}

MatExpr operator > (const Mat& a, const Mat& b)
{
    MatExpr e;
    MatOp_Cmp::makeExpr(e, CV_CMP_GT, a, b);
    return e;
}

MatExpr operator > (const Mat& a, double s)
{
    MatExpr e;
    MatOp_Cmp::makeExpr(e, CV_CMP_GT, a, s);
    return e;
}

MatExpr operator > (double s, const Mat& a)
{
    MatExpr e;
    MatOp_Cmp::makeExpr(e, CV_CMP_LT, a, s);
    return e;
}

MatExpr min(const Mat& a, const Mat& b)
{
    MatExpr e;
    MatOp_Bin::makeExpr(e, 'm', a, b);
    return e;
}

MatExpr min(const Mat& a, double s)
{
    MatExpr e;
    MatOp_Bin::makeExpr(e, 'n', a, s);
    return e;
}

MatExpr min(double s, const Mat& a)
{
    MatExpr e;
    MatOp_Bin::makeExpr(e, 'n', a, s);
    return e;
}

MatExpr max(const Mat& a, const Mat& b)
{
    MatExpr e;
    MatOp_Bin::makeExpr(e, 'M', a, b);
    return e;
}

MatExpr max(const Mat& a, double s)
{
    MatExpr e;
    MatOp_Bin::makeExpr(e, 'N', a, s);
    return e;
}

MatExpr max(double s, const Mat& a)
{
    MatExpr e;
    MatOp_Bin::makeExpr(e, 'N', a, s);
    return e;
}

MatExpr operator & (const Mat& a, const Mat& b)
{
    MatExpr e;
    MatOp_Bin::makeExpr(e, '&', a, b);
    return e;
}

MatExpr operator & (const Mat& a, const Scalar& s)
{
    MatExpr e;
    MatOp_Bin::makeExpr(e, '&', a, s);
    return e;
}

MatExpr operator & (const Scalar& s, const Mat& a)
{
    MatExpr e;
    MatOp_Bin::makeExpr(e, '&', a, s);
    return e;
}

MatExpr operator | (const Mat& a, const Mat& b)
{
    MatExpr e;
    MatOp_Bin::makeExpr(e, '|', a, b);
    return e;
}

MatExpr operator | (const Mat& a, const Scalar& s)
{
    MatExpr e;
    MatOp_Bin::makeExpr(e, '|', a, s);
    return e;
}

MatExpr operator | (const Scalar& s, const Mat& a)
{
    MatExpr e;
    MatOp_Bin::makeExpr(e, '|', a, s);
    return e;
}

MatExpr operator ^ (const Mat& a, const Mat& b)
{
    MatExpr e;
    MatOp_Bin::makeExpr(e, '^', a, b);
    return e;
}

MatExpr operator ^ (const Mat& a, const Scalar& s)
{
    MatExpr e;
    MatOp_Bin::makeExpr(e, '^', a, s);
    return e;
}

MatExpr operator ^ (const Scalar& s, const Mat& a)
{
    MatExpr e;
    MatOp_Bin::makeExpr(e, '^', a, s);
    return e;
}

MatExpr operator ~(const Mat& a)
{
    MatExpr e;
    MatOp_Bin::makeExpr(e, '~', a, Scalar());
    return e;
}

MatExpr abs(const Mat& a)
{
    MatExpr e;
    MatOp_Bin::makeExpr(e, 'a', a, Scalar());
    return e;
}

MatExpr abs(const MatExpr& e)
{
    MatExpr en;
    e.op->abs(e, en);
    return en;
}


Size MatExpr::size() const
{

   return Size();
}


int MatExpr::type() const
{

    return  -1;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////

void MatOp_Identity::assign(const MatExpr& e, Mat& m, int _type) const
{
    if( _type == -1 || _type == e.a.type() )
        m = e.a;
    else
    {
        CV_Assert( CV_MAT_CN(_type) == e.a.channels() );
        e.a.convertTo(m, _type);
    }
}

inline void MatOp_Identity::makeExpr(MatExpr& res, const Mat& m)
{
    res = MatExpr(&g_MatOp_Identity, 0, m, Mat(), Mat(), 1, 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

void MatOp_AddEx::assign(const MatExpr& e, Mat& m, int _type) const
{
    Mat temp, &dst = _type == -1 || e.a.type() == _type ? m : temp;
    if( e.b.data )
    {
        if( e.s == Scalar() || !e.s.isReal() )
        {
            if( e.alpha == 1 )
            {
                if( e.beta == 1 )
                    cv::add(e.a, e.b, dst);
                else if( e.beta == -1 )
                    cv::subtract(e.a, e.b, dst);
                else
                    cv::scaleAdd(e.b, e.beta, e.a, dst);
            }
            else if( e.beta == 1 )
            {
                if( e.alpha == -1 )
                    cv::subtract(e.b, e.a, dst);
                else
                    cv::scaleAdd(e.a, e.alpha, e.b, dst);
            }
            else
                cv::addWeighted(e.a, e.alpha, e.b, e.beta, 0, dst);

            if( !e.s.isReal() )
                cv::add(dst, e.s, dst);
        }
        else
            cv::addWeighted(e.a, e.alpha, e.b, e.beta, e.s[0], dst);
    }
    else if( e.s.isReal() && (dst.data != m.data || fabs(e.alpha) != 1))
    {
        e.a.convertTo(m, _type, e.alpha, e.s[0]);
        return;
    }
    else if( e.alpha == 1 )
        cv::add(e.a, e.s, dst);
    else if( e.alpha == -1 )
        cv::subtract(e.s, e.a, dst);
    else
    {
        e.a.convertTo(dst, e.a.type(), e.alpha);
        cv::add(dst, e.s, dst);
    }

    if( dst.data != m.data )
        dst.convertTo(m, m.type());
}


void MatOp_AddEx::add(const MatExpr& e, const Scalar& s, MatExpr& res) const
{
    res = e;
    res.s += s;
}


void MatOp_AddEx::subtract(const Scalar& s, const MatExpr& e, MatExpr& res) const
{
    res = e;
    res.alpha = -res.alpha;
    res.beta = -res.beta;
    res.s = s - res.s;
}

void MatOp_AddEx::multiply(const MatExpr& e, double s, MatExpr& res) const
{
    res = e;
    res.alpha *= s;
    res.beta *= s;
    res.s *= s;
}

void MatOp_AddEx::divide(double s, const MatExpr& e, MatExpr& res) const
{
    if( isScaled(e) )
        MatOp_Bin::makeExpr(res, '/', e.a, Mat(), s/e.alpha);
    else
        MatOp::divide(s, e, res);
}


void MatOp_AddEx::transpose(const MatExpr& e, MatExpr& res) const
{
    if( isScaled(e) )
        MatOp_T::makeExpr(res, e.a, e.alpha);
    else
        MatOp::transpose(e, res);
}

void MatOp_AddEx::abs(const MatExpr& e, MatExpr& res) const
{
    if( (!e.b.data || e.beta == 0) && fabs(e.alpha) == 1 )
        MatOp_Bin::makeExpr(res, 'a', e.a, -e.s*e.alpha);
    else if( e.b.data && e.alpha + e.beta == 0 && e.alpha*e.beta == -1 )
        MatOp_Bin::makeExpr(res, 'a', e.a, e.b);
    else
        MatOp::abs(e, res);
}

inline void MatOp_AddEx::makeExpr(MatExpr& res, const Mat& a, const Mat& b, double alpha, double beta, const Scalar& s)
{
    res = MatExpr(&g_MatOp_AddEx, 0, a, b, Mat(), alpha, beta, s);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MatOp_Bin::assign(const MatExpr& e, Mat& m, int _type) const
{
    Mat temp, &dst = _type == -1 || e.a.type() == _type ? m : temp;

    if( e.flags == '*' )
        cv::multiply(e.a, e.b, dst, e.alpha);
    else if( e.flags == '/' && e.b.data )
        cv::divide(e.a, e.b, dst, e.alpha);
    else if( e.flags == '/' && !e.b.data )
        cv::divide(e.alpha, e.a, dst );
    else if( e.flags == '&' && e.b.data )
        bitwise_and(e.a, e.b, dst);
    else if( e.flags == '&' && !e.b.data )
        bitwise_and(e.a, e.s, dst);
    else if( e.flags == '|' && e.b.data )
        bitwise_or(e.a, e.b, dst);
    else if( e.flags == '|' && !e.b.data )
        bitwise_or(e.a, e.s, dst);
    else if( e.flags == '^' && e.b.data )
        bitwise_xor(e.a, e.b, dst);
    else if( e.flags == '^' && !e.b.data )
        bitwise_xor(e.a, e.s, dst);
    else if( e.flags == '~' && !e.b.data )
        bitwise_not(e.a, dst);
    else if( e.flags == 'm' )
        cv::min(e.a, e.b, dst);
    else if( e.flags == 'n' )
        cv::min(e.a, e.s[0], dst);
    else if( e.flags == 'M' )
        cv::max(e.a, e.b, dst);
    else if( e.flags == 'N' )
        cv::max(e.a, e.s[0], dst);
    else if( e.flags == 'a' && e.b.data )
        cv::absdiff(e.a, e.b, dst);
    else if( e.flags == 'a' && !e.b.data )
        cv::absdiff(e.a, e.s, dst);
    else
        CV_Error(CV_StsError, "Unknown operation");

    if( dst.data != m.data )
        dst.convertTo(m, _type);
}

void MatOp_Bin::multiply(const MatExpr& e, double s, MatExpr& res) const
{
    if( e.flags == '*' || e.flags == '/' )
    {
        res = e;
        res.alpha *= s;
    }
    else
        MatOp::multiply(e, s, res);
}

void MatOp_Bin::divide(double s, const MatExpr& e, MatExpr& res) const
{
    if( e.flags == '/' && (!e.b.data || e.beta == 0) )
        MatOp_AddEx::makeExpr(res, e.a, Mat(), s/e.alpha, 0);
    else
        MatOp::divide(s, e, res);
}

inline void MatOp_Bin::makeExpr(MatExpr& res, char op, const Mat& a, const Mat& b, double scale)
{
    res = MatExpr(&g_MatOp_Bin, op, a, b, Mat(), scale, b.data ? 1 : 0);
}

inline void MatOp_Bin::makeExpr(MatExpr& res, char op, const Mat& a, const Scalar& s)
{
    res = MatExpr(&g_MatOp_Bin, op, a, Mat(), Mat(), 1, 0, s);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

void MatOp_Cmp::assign(const MatExpr& e, Mat& m, int _type) const
{
    Mat temp, &dst = _type == -1 || _type == CV_8U ? m : temp;

    if( e.b.data )
        cv::compare(e.a, e.b, dst, e.flags);
    else
        cv::compare(e.a, e.alpha, dst, e.flags);

    if( dst.data != m.data )
        dst.convertTo(m, _type);
}

inline void MatOp_Cmp::makeExpr(MatExpr& res, int cmpop, const Mat& a, const Mat& b)
{
    res = MatExpr(&g_MatOp_Cmp, cmpop, a, b, Mat(), 1, 1);
}

inline void MatOp_Cmp::makeExpr(MatExpr& res, int cmpop, const Mat& a, double alpha)
{
    res = MatExpr(&g_MatOp_Cmp, cmpop, a, Mat(), Mat(), alpha, 1);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

void MatOp_T::assign(const MatExpr& e, Mat& m, int _type) const
{
    Mat temp, &dst = _type == -1 || _type == e.a.type() ? m : temp;

    cv::transpose(e.a, dst);

    if( dst.data != m.data || e.alpha != 1 )
        dst.convertTo(m, _type, e.alpha);
}

void MatOp_T::multiply(const MatExpr& e, double s, MatExpr& res) const
{
    res = e;
    res.alpha *= s;
}

void MatOp_T::transpose(const MatExpr& e, MatExpr& res) const
{
    if( e.alpha == 1 )
        MatOp_Identity::makeExpr(res, e.a);
    else
        MatOp_AddEx::makeExpr(res, e.a, Mat(), e.alpha, 0);
}

inline void MatOp_T::makeExpr(MatExpr& res, const Mat& a, double alpha)
{
    res = MatExpr(&g_MatOp_T, 0, a, Mat(), Mat(), alpha, 0);
}



}

/* End of file. */
