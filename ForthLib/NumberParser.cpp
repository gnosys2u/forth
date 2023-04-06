//////////////////////////////////////////////////////////////////////
//
// NumberParser.cpp: implementation of the NumberParser class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "NumberParser.h"

//////////////////////////////////////////////////////////////////////
////
///
//                     NumberParser
// 


NumberParser::NumberParser()
{
    ResetValues();
}

void NumberParser::ResetValues()
{
    mValidChars[0] = 0;
    mNumValidChars = 0;
    mForcedLong = false;
    mForcedFloat = false;
    mIsDoubleCell = false;
    mLongValue = 0L;
    mDoubleFloatValue = 0.0;
    mSingleFloatValue = 0.0f;
    mDoubleCellValue.cells[0] = 0;
    mDoubleCellValue.cells[1] = 0;
    mBase = 10;

    mExponentPosition = -1;
    mPeriodPosition = -1;
    mIsOffset = false;
    mIsNegative = false;
    mNumberType = NumberType::kInvalid;
}

NumberParser::~NumberParser()
{
}

NumberType NumberParser::ScanNumber(const char* pSrcString, int defaultBase)
{
    int32_t digit;
    bool inExponent = false;
    char c;
    int digitsFound = 0;
    ucell len = (ucell)strlen(pSrcString);
    bool baseForced = false;
    bool forcedLong = false;
    bool forcedSingleFloat = false;
    int srcIndex = 0;
    int dstIndex = 0;

    ResetValues();

    mBase = defaultBase;

    // if CFloatLiterals is off, '.' indicates a double-precision number, not a float
    //bool periodMeansDoubleInt = !CheckFeature(kFFCFloatLiterals);

    mIsNegative = false;
    bool isSingle = true;

    while (srcIndex < len)
    {
        c = pSrcString[srcIndex];
        bool isLastChar = (srcIndex == len - 1);

        if (c == '$')
        {
            if (srcIndex < 2 && !baseForced)
            {
                mBase = 16;
                baseForced = true;
            }
            else
            {
                return NumberType::kInvalid;
            }
        }
        else if (c == '#')
        {
            if (srcIndex < 2 && !baseForced)
            {
                mBase = 10;
                baseForced = true;
            }
            else
            {
                return NumberType::kInvalid;
            }
        }
        else if (c == '%')
        {
            if (srcIndex < 2 && !baseForced)
            {
                mBase = 2;
                baseForced = true;
            }
            else
            {
                return NumberType::kInvalid;
            }
        }
        else if (c == '+')
        {
            if (isLastChar)
            {
                // last char is plus, so this NNN+, which is a positive offset op
                mIsOffset = true;
            }
            else
            {
                if (srcIndex > 1 && srcIndex != mExponentPosition + 1)
                {
                    // plus can only happen in first 2 chars, or first char of exponent,
                    //   or as very last char (for offsets)
                    return NumberType::kInvalid;
                }
            }
        }
        else if (c == '-')
        {
            if (isLastChar)
            {
                // last char is minus, so this NNN-, which is a negative offset op
                mIsNegative = !mIsNegative;
                mIsOffset = true;
            }
            else
            {
                if (srcIndex > 1 && srcIndex != mExponentPosition + 1)
                {
                    // minus can only happen in first 2 chars, or first char of exponent,
                    //   or as very last char (for offsets)
                    return NumberType::kInvalid;
                }

                if (srcIndex < 2)    // don't set negative flag if this is in exponent part
                {
                    mIsNegative = true;
                }
                mValidChars[mNumValidChars++] = '-';
            }
        }
        else if (mBase == 10 && (c == 'e' || c == 'E'))
        {
            if (mExponentPosition < 0)
            {
                if (srcIndex == 0)
                {
                    // exponent char E can't be first char
                    return NumberType::kInvalid;
                }
                mExponentPosition = srcIndex;
                mValidChars[mNumValidChars++] = c;
            }
            else
            {
                // we have already seen an E
                return NumberType::kInvalid;
            }
        }
        else if (isLastChar && mBase < 20 && (c == 'L' || c == 'l'))
        {
            forcedLong = true;
        }
        else if (isLastChar && mExponentPosition >= 0 && (c == 'f' || c == 'F'))
        {
            forcedSingleFloat = true;
        }
        else if (c == '.')
        {
            if (mPeriodPosition >= 0)
            {
                // we have already seen a period
                return NumberType::kInvalid;
            }
            mPeriodPosition = srcIndex;
            mValidChars[mNumValidChars++] = c;
        }
        else
        {
            // this must be a valid digit in current base, if so accumulate it into value
            // otherwise it's not a valid number
            digit = -1;
            if ((c >= '0') && (c <= '9'))
            {
                digit = c - '0';
            }
            else if ((c >= 'A') && (c <= 'Z'))
            {
                digit = 10 + (c - 'A');
            }
            else if ((c >= 'a') && (c <= 'z'))
            {
                digit = 10 + (c - 'a');
            }

            if (digit >= 0 && digit < mBase)
            {
                if (mExponentPosition < 0)
                {
#if defined(FORTH64)
#if defined(WINDOWS_BUILD)
                    // ucells[1] holds high part of product
                    uint64_t lowPart = UnsignedMultiply128(mDoubleCellValue.ucells[0], mBase, &mDoubleCellValue.ucells[1]);
                    mDoubleCellValue.ucells[0] = lowPart + digit;
                    if (mDoubleCellValue.ucells[0] < lowPart)
                    {
                        // carry from low part to high part
                        mDoubleCellValue.ucells[1]++;
                    }
#else
                    mDoubleCellValue.udcell = ((__uint128)value) * mBase;
                    mDoubleCellValue.udcell += digit;
#endif
#else
                    mDoubleCellValue.udcell = (mDoubleCellValue.udcell * mBase) + digit;
#endif
                }
                mValidChars[mNumValidChars++] = c;
            }
            else
            {
                // char isn't a valid digit for this base
                return NumberType::kInvalid;
            }
        }

        srcIndex++;
    }
    mValidChars[mNumValidChars++] = '\0';

    // number is valid, set final value based on detected type
    if (mExponentPosition >= 0)
    {
        if (mExponentPosition == mNumValidChars - 2)
        {
            // there was no exponent, the float literal string just ended at 'E', we need to
            //  stuff a dummy exponent 0 to make sscanf happy
            mValidChars[mNumValidChars - 1] = '0';
            mValidChars[mNumValidChars] = '\0';
        }
        // single or double precision float
        if (sscanf(&(mValidChars[0]), "%lf", &mDoubleFloatValue) == 1)
        {
            mValidChars[mNumValidChars - 1] = '\0';
            if (forcedSingleFloat)
            {
                mNumberType = NumberType::kSingleFloat;
                mSingleFloatValue = mDoubleFloatValue;
            }
            else
            {
                mNumberType = NumberType::kDoubleFloat;
            }
        }
        else
        {
            return NumberType::kInvalid;
        }
    }
    else if (mPeriodPosition >= 0)
    {
        // double-cell integer
        mNumberType = NumberType::kDoubleCell;
        int64_t temp;
#if defined(FORTH64)
#if defined(WINDOWS_BUILD)
        if (mIsNegative)
        {
            mDoubleCellValue.cells[0] = ~mDoubleCellValue.cells[0];
            mDoubleCellValue.cells[1] = ~mDoubleCellValue.cells[1];
            mDoubleCellValue.cells[0]++;
            if (mDoubleCellValue.cells[0] == 0)
            {
                mDoubleCellValue.cells[1]++;
            }
        }
#else
        if (mIsNegative)
        {
            mDoubleCellValue.sdcell = -mDoubleCellValue.sdcell;
        }
#endif
        // swap low and high parts as per Forth ANSI standard
        temp = mDoubleCellValue.cells[0];
        mDoubleCellValue.cells[0] = mDoubleCellValue.cells[1];
        mDoubleCellValue.cells[1] = temp;
#else
        stackInt64 vv;
        vv.s64 = mIsNegative ? -mDoubleCellValue.sdcell : mDoubleCellValue.sdcell;

        mDoubleCellValue.cells[0] = vv.s32[1];
        mDoubleCellValue.cells[1] = vv.s32[0];
#endif
    }
    else
    {
        // 32-bit int or 64-bit long
#if defined(FORTH64)
        mNumberType = NumberType::kLong;
        mLongValue = mIsNegative ? -mDoubleCellValue.cells[0] : mDoubleCellValue.cells[0];
#else
        mNumberType = forcedLong ? NumberType::kLong : NumberType::kInt;
        mLongValue = mIsNegative ? -mDoubleCellValue.sdcell : mDoubleCellValue.sdcell;
#endif
    }
    return mNumberType;
}

NumberType NumberParser::GetNumberType() const
{
    return mNumberType;
}

int32_t NumberParser::GetIntValue() const
{
    return (int32_t) mLongValue;
}

int64_t NumberParser::GetLongValue() const
{
    return mLongValue;
}

double NumberParser::GetDoubleFloatValue() const
{
    return mDoubleFloatValue;
}

float NumberParser::GetSingleFloatValue() const
{
    return mSingleFloatValue;
}

const doubleCell& NumberParser::GetDoubleCellValue() const
{
    return mDoubleCellValue;
}


char* NumberParser::GetValidChars()
{
    return &(mValidChars[0]);
}

int NumberParser::GetNumValidChars() const
{
    return mNumValidChars;
}

int NumberParser::GetBase() const
{
    return mBase;
}

bool NumberParser::IsOffset() const
{
    return mIsOffset;
}

