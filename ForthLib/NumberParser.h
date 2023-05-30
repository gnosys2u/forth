#pragma once
//////////////////////////////////////////////////////////////////////
//
// NumberParser.h: interface for the NumberParser class.
//
//////////////////////////////////////////////////////////////////////


#include "Forth.h"

enum class NumberType :ucell {
    kInvalid,
    kInt,
    kLong,
    kDoubleFloat,
    kSingleFloat,
    kDoubleCell
};

class NumberParser
{
public:
    NumberParser();
    ~NumberParser();

    NumberType ScanNumber(const char* pStr, int defaultBase);
    bool ScanFloat(const char* pStr, double& result);

    NumberType GetNumberType() const;

    int32_t GetIntValue() const;
    int64_t GetLongValue() const;
    double GetDoubleFloatValue() const;
    float GetSingleFloatValue() const;
    const doubleCell& GetDoubleCellValue() const;

    char* GetValidChars();
    int GetNumValidChars() const;
    int GetBase() const;
    bool IsOffset() const;

private:
    void ResetValues();

    char mValidChars[132];     // big enough for 128 binary digits, minus sign, terminating nul and some slop
    int mNumValidChars;
    bool mForcedLong;
    bool mForcedFloat;
    bool mIsDoubleCell;
    bool mIsOffset;
    bool mIsNegative;

    int64_t mLongValue;
    double mDoubleFloatValue;
    float mSingleFloatValue;
    doubleCell mDoubleCellValue;
    NumberType mNumberType;
    int mExponentPosition;
    int mPeriodPosition;

    int mBase;
};

