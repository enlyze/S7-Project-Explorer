//
// EnlyzeS7PLib - Library for parsing symbols in Siemens STEP 7 project files
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#include <numeric>
#include <EnlyzeWinStringLib.h>

#include "CMc5ArrayEnumerator.h"
#include "CMc5codeParser.h"

struct ByteSizedVariableInfo
{
    const char* szName;
    unsigned char ByteAlignment;
    unsigned char ByteSize;

    bool operator==(const std::string& other) const
    {
        return szName == other;
    }
};


std::variant<std::monostate, CS7PError>
CMc5codeParser::_AddArrayVariable(const std::string& strStructureType, const std::string& strVariableName)
{
    // See "Programmieren mit STEP 7", A5E02789665-01, page 597.
    const size_t MaxArrayDimensions = 6;

    // Array variables need to be aligned to a 2-byte boundary.
    _AlignUp(2 * 8);

    // The next non-comment token must be the opening bracket.
    auto TokenResult = _GetNextToken("[");
    if (std::holds_alternative<std::monostate>(TokenResult))
    {
        return CS7PError(
            L"Expected opening bracket but found EOF while parsing array variable definition for " +
            StrToWstr(strVariableName)
        );
    }

    std::string strToken = std::get<std::string>(std::move(TokenResult));
    if (strToken != "[")
    {
        return CS7PError(
            L"Expected opening bracket but found \"" + StrToWstr(strToken) +
            L"\" while parsing array variable definition for " + StrToWstr(strVariableName)
        );
    }

    std::vector<CMc5ArrayDimension> ArrayDimensions;
    for (;;)
    {
        if (ArrayDimensions.size() == MaxArrayDimensions)
        {
            return CS7PError(L"Array variable " + StrToWstr(strVariableName) + L" exceeds maximum array dimensions");
        }

        auto ArrayDimensionInfoResult = _GetNextArrayDimensionInfo(strVariableName);
        if (const auto pError = std::get_if<CS7PError>(&ArrayDimensionInfoResult))
        {
            return *pError;
        }

        CMc5ArrayDimension Dimension = std::get<CMc5ArrayDimension>(std::move(ArrayDimensionInfoResult));
        ArrayDimensions.push_back(Dimension);

        // The next non-comment token must be a comma to indicate the next dimension or a closing bracket.
        TokenResult = _GetNextToken(",]");
        if (std::holds_alternative<std::monostate>(TokenResult))
        {
            return CS7PError(
                L"Expected comma or closing bracket but found EOF while parsing array variable definition for " +
                StrToWstr(strVariableName)
            );
        }

        strToken = std::get<std::string>(std::move(TokenResult));
        if (strToken == "]")
        {
            break;
        }
        else if (strToken != ",")
        {
            return CS7PError(
                L"Expected comma or closing bracket but found \"" + StrToWstr(strToken) +
                L"\" while parsing array variable definition for " + StrToWstr(strVariableName)
            );
        }
    }

    // The next non-comment token must be the word "OF".
    TokenResult = _GetNextToken();
    if (std::holds_alternative<std::monostate>(TokenResult))
    {
        return CS7PError(
            L"Expected \"OF\" but found EOF while parsing array variable definition for " + StrToWstr(strVariableName)
        );
    }

    strToken = std::get<std::string>(std::move(TokenResult));
    if (strToken != "OF")
    {
        return CS7PError(
            L"Expected \"OF\" but found \"" + StrToWstr(strToken) + L"\" while parsing array variable definition for " +
            StrToWstr(strVariableName)
        );
    }

    // The next non-comment token must be the element type.
    TokenResult = _GetNextToken(";[:");
    if (std::holds_alternative<std::monostate>(TokenResult))
    {
        return CS7PError(
            L"Expected element type but found EOF while parsing array variable definition for " + StrToWstr(strVariableName)
        );
    }

    std::string strElementType = std::get<std::string>(std::move(TokenResult));

    // Is this a complex array type?
    // Then unpack the array into its elements.
    if (strElementType == "STRUCT" || m_Mc5codeMap.find(strElementType) != m_Mc5codeMap.end())
    {
        const char* pszSavedPosition = m_pszMc5codePosition;

        // Iterate over all elements.
        for (const auto& Indexes : CMc5ArrayEnumerator(ArrayDimensions))
        {
            // Rewind back to the start position before reading the complex type again.
            m_pszMc5codePosition = pszSavedPosition;

            // Build the element name from the indexes.
            std::string strElementName = strVariableName + "[";

            strElementName += std::accumulate(
                std::next(Indexes.begin()),
                Indexes.end(),
                std::to_string(Indexes.front()),
                [](std::string a, const short& b)
                {
                    return std::move(a) + "," + std::to_string(b);
                }
            );

            strElementName += "]";

            // Add the variable.
            std::variant<std::monostate, CS7PError> Result;
            if (strElementType == "STRUCT")
            {
                Result = _AddStructVariable(strElementName);
            }
            else
            {
                Result = _AddBlockVariable(strElementName, strElementType);
            }

            if (const auto pError = std::get_if<CS7PError>(&Result))
            {
                return *pError;
            }
        }
    }
    else
    {
        // No complex type, then add the primitive variable with the array type.
        auto Result = _AddPrimitiveVariable(strStructureType, strVariableName, strElementType, &ArrayDimensions);
        if (const auto pError = std::get_if<CS7PError>(&Result))
        {
            return *pError;
        }
    }

    // Variables after an array always start on a 2-byte boundary.
    _AlignUp(2 * 8);

    return std::monostate();
}

std::variant<std::monostate, CS7PError>
CMc5codeParser::_AddBlockVariable(const std::string& strVariableName, const std::string& strVariableType)
{
    // Block variables need to be aligned to a 2-byte boundary.
    _AlignUp(2 * 8);

    // The next non-comment token must be the block number.
    auto TokenResult = _GetNextToken(";");
    if (std::holds_alternative<std::monostate>(TokenResult))
    {
        return CS7PError(
            L"Expected block number but found EOF while parsing " + StrToWstr(strVariableType) +
            L" variable definition for " + StrToWstr(strVariableName)
        );
    }

    std::string strToken = std::get<std::string>(std::move(TokenResult));
    auto Option = StrToSizeT(strToken);
    if (!Option.has_value())
    {
        return CS7PError(
            L"Expected block number but found \"" + StrToWstr(strToken) + L"\" while parsing " +
            StrToWstr(strVariableType) + L" variable definition for " + StrToWstr(strVariableName)
        );
    }

    size_t BlockNumber = Option.value();

    // Continue parsing up to EOF or the final semicolon.
    for (;;)
    {
        TokenResult = _GetNextToken(";");
        if (std::holds_alternative<std::monostate>(TokenResult))
        {
            break;
        }

        strToken = std::get<std::string>(std::move(TokenResult));
        if (strToken == ";")
        {
            break;
        }
    }

    // Find the block in its MC5 Code Map.
    const std::map<size_t, std::string>& BlockMc5codeMap = m_Mc5codeMap.at(strVariableType);
    const auto it = BlockMc5codeMap.find(BlockNumber);
    if (it == BlockMc5codeMap.end())
    {
        return CS7PError(
            L"Variable " + StrToWstr(strVariableName) + L" of DB" + std::to_wstring(m_DbNumber) + L" references " +
            StrToWstr(strVariableType) + std::to_wstring(BlockNumber) + L", which could not be found"
        );
    }

    // Parse the MC5 Code for this block.
    const std::string& strMc5code = it->second;
    CMc5codeParser Parser(m_Symbols, m_BitAddressCounter, m_DbNumber, strMc5code, m_Mc5codeMap);

    std::string strPrefix = strVariableName + ".";
    return Parser.Parse(strPrefix);
}

std::variant<std::monostate, CS7PError>
CMc5codeParser::_AddPrimitiveVariable(const std::string& strStructureType, const std::string& strVariableName, const std::string& strVariableType, const std::vector<CMc5ArrayDimension>* pArrayDimensions)
{
    static constexpr std::array<ByteSizedVariableInfo, 20> ByteSizedVariables = {{
        {"BYTE", 1, 1},
        {"CHAR", 1, 1},
        {"INT", 2, 2},
        {"WORD", 2, 2},
        {"COUNTER", 2, 2},
        {"DATE", 2, 2},
        {"TIMER", 2, 2},
        {"S5TIME", 2, 2},
        {"BLOCK_DB", 2, 2},
        {"BLOCK_FB", 2, 2},
        {"BLOCK_FC", 2, 2},
        {"BLOCK_SDB", 2, 2},
        {"DINT", 2, 4},
        {"DWORD", 2, 4},
        {"REAL", 2, 4},
        {"TIME", 2, 4},
        {"TIME_OF_DAY", 2, 4},
        {"POINTER", 2, 6},
        {"DATE_AND_TIME", 2, 8},
        {"ANY", 2, 10},
    }};

    size_t BitAddress;
    std::string strFullVariableType;

    // Assume a single variable, but check if the caller passed array dimensions.
    size_t ArrayDimensionCount = 1;
    long ElementCount = 1;
    if (pArrayDimensions)
    {
        ArrayDimensionCount = pArrayDimensions->size();

        // Count the total number of elements over all dimensions.
        // Array elements over multiple dimensions are ordered linearly without any padding. (except for BOOL!)
        for (const auto& Info : *pArrayDimensions)
        {
            ElementCount *= Info.EndIndex - Info.StartIndex + 1;
        }
    }

    if (strVariableType == "BOOL")
    {
        // A BOOL variable always works on the current address with no extra alignment.
        BitAddress = m_BitAddressCounter;

        if (ArrayDimensionCount > 1)
        {
            // Yay, special cases!
            // A multi-dimensional BOOL array variable needs 1 bit per element of the last dimension,
            // but aligns up to a byte boundary for each other dimension element.
            //
            // Two examples:
            //  * "ARRAY[1..2, 1..8] OF BOOL" needs 2 bytes in total:
            //    1,1 - 1,2 - 1,3 - 1,4 - 1,5 - 1,6 - 1,7 - 1,8
            //    2,1 - 2,2 - 2,3 - 2,4 - 2,5 - 2,6 - 2,7 - 2,8
            //
            //  * "ARRAY[1..8, 1..2] OF BOOL" needs 8 bytes in total:
            //    1,1 - 1,2
            //    2,1 - 2,2
            //    3,1 - 3,2
            //    4,1 - 4,2
            //    5,1 - 5,2
            //    6,1 - 6,2
            //    7,1 - 7,2
            //    8,1 - 8,2

            const auto& LastDimensionInfo = pArrayDimensions->back();
            long LastDimensionElementCount = LastDimensionInfo.EndIndex - LastDimensionInfo.StartIndex + 1;
            long OtherDimensionsElementCount = ElementCount / LastDimensionElementCount;

            for (long i = 0; i < OtherDimensionsElementCount; i++)
            {
                _AlignUp(8);
                m_BitAddressCounter += LastDimensionElementCount;
            }
        }
        else
        {
            // A single BOOL variable or a one-dimensional BOOL array variable needs just 1 bit per element.
            m_BitAddressCounter += ElementCount;
        }
    }
    else if (strVariableType == "STRING")
    {
        // The next non-comment token must be the opening bracket.
        auto TokenResult = _GetNextToken("[");
        if (std::holds_alternative<std::monostate>(TokenResult))
        {
            return CS7PError(
                L"Expected opening bracket but found EOF while parsing string variable definition for " + StrToWstr(strVariableName)
            );
        }

        std::string strToken = std::get<std::string>(std::move(TokenResult));
        if (strToken != "[")
        {
            return CS7PError(
                L"Expected opening bracket but found \"" + StrToWstr(strToken) +
                L"\" while parsing string variable definition for " + StrToWstr(strVariableName)
            );
        }

        // The next non-comment token must be the character count.
        TokenResult = _GetNextToken("]");
        if (std::holds_alternative<std::monostate>(TokenResult))
        {
            return CS7PError(
                L"Expected character count but found EOF while parsing string variable definition for " + StrToWstr(strVariableName)
            );
        }

        strToken = std::get<std::string>(std::move(TokenResult));
        auto Option = StrToSizeT(strToken);
        if (!Option.has_value())
        {
            return CS7PError(
                L"Expected character count but found \"" + StrToWstr(strToken) +
                L"\" while parsing string variable definition for " + StrToWstr(strVariableName)
            );
        }

        size_t CharacterCount = Option.value();

        // We can build the full variable type now.
        strFullVariableType = "STRING [" + std::to_string(CharacterCount) + "]";

        // A STRING variable has 2 bytes in addition to the character count.
        size_t StringByteCount = 2 + CharacterCount;

        // A STRING variable needs to be aligned to a 2-byte boundary.
        // We can't move the next two lines into the loop, because BitAddress needs to be set outside the loop. Otherwise, MSVC complains that it may be used uninitialized.
        _AlignUp(2 * 8);
        BitAddress = m_BitAddressCounter;

        for (long i = 0; i < ElementCount; i++)
        {
            // Each STRING variable within an ARRAY of STRINGs also needs to be aligned to a 2-byte boundary (note that StringByteCount can be odd, e.g. for a "STRING[3]").
            _AlignUp(2 * 8);

            // Advance the address counter by the calculated number of bytes for each element.
            m_BitAddressCounter += StringByteCount * 8;
        }
    }
    else
    {
        // Is it one of the trivial types without any special handling?
        const auto& it = std::find(ByteSizedVariables.begin(), ByteSizedVariables.end(), strVariableType);
        if (it != ByteSizedVariables.end())
        {
            _AlignUp(it->ByteAlignment * 8);
            BitAddress = m_BitAddressCounter;

            m_BitAddressCounter += it->ByteSize * 8 * ElementCount;
        }
        else
        {
            return CS7PError(
                L"Variable " + StrToWstr(strVariableName) + L" of DB" + std::to_wstring(m_DbNumber) +
                L" has unknown primitive variable type " + StrToWstr(strVariableType)
            );
        }
    }

    // Continue parsing up to EOF or the final semicolon.
    for (;;)
    {
        auto TokenResult = _GetNextToken(";");
        if (std::holds_alternative<std::monostate>(TokenResult))
        {
            break;
        }

        std::string strToken = std::get<std::string>(std::move(TokenResult));
        if (strToken == ";")
        {
            break;
        }
    }

    // Find the last comment for this symbol.
    std::string strVariableComment;
    for (;;)
    {
        auto TokenResult = _GetNextToken(":{", true);
        if (std::holds_alternative<std::monostate>(TokenResult))
        {
            break;
        }

        std::string strToken = std::get<std::string>(std::move(TokenResult));
        if (strToken.starts_with("//"))
        {
            strVariableComment = Str1252ToStr(strToken.substr(2));
        }
        else
        {
            // This is not a comment, but the next token, and we are not interested in that token here.
            // Rewind the reader position, so that it is found again by the next _GetNextToken call.
            m_pszMc5codePosition -= strToken.size();
            break;
        }
    }

    // Add this symbol.
    S7Symbol Symbol;
    Symbol.strName = strVariableName;

    size_t AddressMajor = BitAddress / 8;
    size_t AddressMinor = BitAddress % 8;
    Symbol.strCode = "DB" + std::to_string(m_DbNumber) + ":" + std::to_string(AddressMajor) + "." + std::to_string(AddressMinor);

    if (ElementCount > 1)
    {
        Symbol.strDatatype = "ARRAY [";

        Symbol.strDatatype += std::accumulate(
            std::next(pArrayDimensions->begin()),
            pArrayDimensions->end(),
            pArrayDimensions->front().AsString(),
            [](std::string a, const CMc5ArrayDimension& b)
            {
                return std::move(a) + ", " + b.AsString();
            }
        );

        Symbol.strDatatype += "] OF ";
    }

    if (!strFullVariableType.empty())
    {
        Symbol.strDatatype += strFullVariableType;
    }
    else
    {
        Symbol.strDatatype += strVariableType;
    }

    Symbol.strComment = strStructureType;
    if (!strVariableComment.empty())
    {
        Symbol.strComment += "; " + strVariableComment;
    }

    m_Symbols.push_back(Symbol);

    // Return success!
    return std::monostate();
}

std::variant<std::monostate, CS7PError>
CMc5codeParser::_AddSingleVariable(const std::string& strStructureType, const std::string& strVariableName, const std::string& strVariableType)
{
    if (strVariableType == "STRUCT")
    {
        return _AddStructVariable(strVariableName);
    }
    else if (m_Mc5codeMap.find(strVariableType) != m_Mc5codeMap.end())
    {
        return _AddBlockVariable(strVariableName, strVariableType);
    }
    else
    {
        return _AddPrimitiveVariable(strStructureType, strVariableName, strVariableType);
    }
}

std::variant<std::monostate, CS7PError>
CMc5codeParser::_AddStructVariable(const std::string& strVariableName)
{
    std::string strPrefix = strVariableName + ".";
    auto Result = _ParseInnerStructure("Struct", strPrefix);
    if (const auto pError = std::get_if<CS7PError>(&Result))
    {
        return *pError;
    }

    return std::monostate();
}

std::variant<std::monostate, CS7PError>
CMc5codeParser::_AddVariable(const std::string& strStructureType, const std::string& strVariableName)
{
    // The next non-comment token can be the delimiter between variable name and type, or the beginning of an attribute list.
    auto TokenResult = _GetNextToken(":{");
    if (std::holds_alternative<std::monostate>(TokenResult))
    {
        return CS7PError(
            L"Expected colon or opening brace but found EOF while parsing variable definition for " +
            StrToWstr(strVariableName)
        );
    }

    std::string strToken = std::get<std::string>(std::move(TokenResult));
    if (strToken == "{")
    {
        // This is the beginning of a variable attribute list.
        // We don't care about variable attributes, so just look for the closing brace.
        for (;;)
        {
            TokenResult = _GetNextToken("}");
            if (std::holds_alternative<std::monostate>(TokenResult))
            {
                return CS7PError(
                    L"Expected closing brace but found EOF while parsing variable definition for " +
                    StrToWstr(strVariableName)
                );
            }

            strToken = std::get<std::string>(std::move(TokenResult));
            if (strToken == "}")
            {
                break;
            }
        }

        // Now we should finally be at the delimiter.
        TokenResult = _GetNextToken(":");
        if (std::holds_alternative<std::monostate>(TokenResult))
        {
            return CS7PError(
                L"Expected colon but found EOF while parsing variable definition for " + StrToWstr(strVariableName)
            );
        }

        strToken = std::get<std::string>(std::move(TokenResult));
    }

    if (strToken != ":")
    {
        return CS7PError(
            L"Expected colon but found \"" + StrToWstr(strToken) +
            L"\" while parsing variable definition for " + StrToWstr(strVariableName)
        );
    }

    // The next non-comment token must be the variable type.
    TokenResult = _GetNextToken(";[:");
    if (std::holds_alternative<std::monostate>(TokenResult))
    {
        return CS7PError(
            L"Expected colon or equal but found EOF while parsing variable definition for " + StrToWstr(strVariableName)
        );
    }

    std::string strVariableType = std::get<std::string>(std::move(TokenResult));

    // Is this an array or a single variable?
    if (strVariableType == "ARRAY")
    {
        return _AddArrayVariable(strStructureType, strVariableName);
    }
    else
    {
        return _AddSingleVariable(strStructureType, strVariableName, strVariableType);
    }
}

void
CMc5codeParser::_AlignUp(const size_t BitAlignment)
{
    size_t BitMask = BitAlignment - 1;
    m_BitAddressCounter = (m_BitAddressCounter + BitMask) & ~BitMask;
}

std::variant<CMc5ArrayDimension, CS7PError>
CMc5codeParser::_GetNextArrayDimensionInfo(const std::string& strVariableName)
{
    // The next non-comment token must be the start index.
    auto TokenResult = _GetNextToken(".");
    if (std::holds_alternative<std::monostate>(TokenResult))
    {
        return CS7PError(
            L"Expected start index but found EOF while parsing array variable definition for " + StrToWstr(strVariableName)
        );
    }

    std::string strToken = std::get<std::string>(std::move(TokenResult));
    auto Option = StrToLong(strToken);
    if (!Option.has_value())
    {
        return CS7PError(
            L"Expected start index but found \"" + StrToWstr(strToken) +
            L"\" while parsing array variable definition for " + StrToWstr(strVariableName)
        );
    }

    long lStartIndex = Option.value();
    if (lStartIndex < SHRT_MIN || lStartIndex > SHRT_MAX)
    {
        return CS7PError(L"Start index " + std::to_wstring(lStartIndex) + L" is out of range");
    }

    CMc5ArrayDimension Info;
    Info.StartIndex = static_cast<short>(lStartIndex);

    // The next two non-comment tokens must be dots.
    for (int i = 1; i <= 2; i++)
    {
        TokenResult = _GetNextToken(".");
        if (std::holds_alternative<std::monostate>(TokenResult))
        {
            return CS7PError(
                L"Expected dot " + std::to_wstring(i) + L" but found EOF while parsing array variable definition for " +
                StrToWstr(strVariableName)
            );
        }

        strToken = std::get<std::string>(std::move(TokenResult));
        if (strToken != ".")
        {
            return CS7PError(
                L"Expected dot " + std::to_wstring(i) + L" but found \"" + StrToWstr(strToken) +
                L"\" while parsing array variable definition for " + StrToWstr(strVariableName)
            );
        }
    }

    // The next non-comment token must be the end index.
    TokenResult = _GetNextToken(",]");
    if (std::holds_alternative<std::monostate>(TokenResult))
    {
        return CS7PError(L"Expected end index but found EOF while parsing array variable definition for " + StrToWstr(strVariableName));
    }

    strToken = std::get<std::string>(std::move(TokenResult));
    Option = StrToLong(strToken);
    if (!Option.has_value())
    {
        return CS7PError(
            L"Expected end index but found \"" + StrToWstr(strToken) +
            L"\" while parsing array variable definition for " + StrToWstr(strVariableName)
        );
    }

    long lEndIndex = Option.value();
    if (lEndIndex < SHRT_MIN || lEndIndex > SHRT_MAX || lEndIndex < lStartIndex)
    {
        return CS7PError(
            L"End index " + std::to_wstring(lEndIndex) + L" is out of range (considering start index " +
            std::to_wstring(lStartIndex) + L")"
        );
    }

    Info.EndIndex = static_cast<short>(lEndIndex);

    return Info;
}

std::variant<std::string, std::monostate>
CMc5codeParser::_GetNextToken(const char* szTokens, bool bGetComments)
{
    for (;;)
    {
        // Skip whitespace.
        while (IsSpaceCharacter(*m_pszMc5codePosition))
        {
            m_pszMc5codePosition++;
        }

        // Have we reached the end of the MC5 Code?
        if (*m_pszMc5codePosition == '\0')
        {
            return std::monostate();
        }

        const char* pszStart = m_pszMc5codePosition;

        // Check if this is a line comment.
        if (*pszStart == '/' && pszStart[1] == '/')
        {
            // Find the end of the comment (at the end of the line).
            while (*m_pszMc5codePosition != '\0' && *m_pszMc5codePosition != '\r' && *m_pszMc5codePosition != '\n')
            {
                m_pszMc5codePosition++;
            }

            if (bGetComments)
            {
                // Return this comment.
                size_t Length = m_pszMc5codePosition - pszStart;
                return std::string(pszStart, Length);
            }
            else
            {
                // The caller is not interested in comments, so find another token.
                continue;
            }
        }

        // Check if we match one of the single-character tokens.
        if (strchr(szTokens, *m_pszMc5codePosition))
        {
            // Return that single character token.
            std::string strToken = std::string(1, *m_pszMc5codePosition);
            m_pszMc5codePosition++;
            return strToken;
        }

        // If this is none of the above, we return everything up to the next space or single-character token (= a word).
        while (*m_pszMc5codePosition != '\0' && !IsSpaceCharacter(*m_pszMc5codePosition) && !strchr(szTokens, *m_pszMc5codePosition))
        {
            m_pszMc5codePosition++;
        }

        size_t Length = m_pszMc5codePosition - pszStart;
        return std::string(pszStart, Length);
    }
}

std::variant<bool, CS7PError>
CMc5codeParser::_ParseStructureType(std::string& strStructureType)
{
    // Read the next non-comment token.
    auto TokenResult = _GetNextToken();
    if (std::holds_alternative<std::monostate>(TokenResult))
    {
        // End of file, we are done!
        return false;
    }

    std::string strToken = std::get<std::string>(std::move(TokenResult));

    // This token must be the struct type.
    if (strToken == "VAR_INPUT")
    {
        strStructureType = "In";
    }
    else if (strToken == "VAR_OUTPUT")
    {
        strStructureType = "Out";
    }
    else if (strToken == "VAR_IN_OUT")
    {
        strStructureType = "InOut";
    }
    else if (strToken == "VAR")
    {
        strStructureType = "Var";
    }
    else if (strToken == "STRUCT")
    {
        strStructureType = "Struct";
    }
    else if (strToken == "VAR_TEMP")
    {
        // There are no more interesting variables as soon as we hit VAR_TEMP.
        return false;
    }
    else
    {
        return CS7PError(L"Unknown structure type \"" + StrToWstr(strToken) + L"\" while parsing DB" + std::to_wstring(m_DbNumber));
    }

    return true;
}

std::variant<bool, CS7PError>
CMc5codeParser::_ParseInnerStructure(const std::string& strStructureType, const std::string& strPrefix)
{
    for (;;)
    {
        // Read the next non-comment token.
        auto TokenResult = _GetNextToken(":;");
        if (std::holds_alternative<std::monostate>(TokenResult))
        {
            // End of file, we are done!
            return false;
        }

        std::string strToken = std::get<std::string>(std::move(TokenResult));

        // Is this the end of the inner structure?
        if (strToken == "END_VAR")
        {
            // We have finished this structure, but there may be additional structures to parse.
            return true;
        }
        else if (strToken == "END_STRUCT")
        {
            // END_STRUCT concludes with a final semicolon.
            TokenResult = _GetNextToken(";");
            if (std::holds_alternative<std::monostate>(TokenResult))
            {
                // End of file, we are done!
                return false;
            }

            strToken = std::get<std::string>(std::move(TokenResult));
            if (strToken == ";")
            {
                // We have finished this structure, but there may be additional structures to parse.
                return true;
            }
            else
            {
                return CS7PError(L"Expected semicolon after END_STRUCT but got: " + StrToWstr(strToken));
            }
        }

        // No, then we are at the beginning of a variable definition and this must be the variable name.
        std::string strVariableName = strPrefix + Str1252ToStr(strToken);
        auto Result = _AddVariable(strStructureType, strVariableName);
        if (const auto pError = std::get_if<CS7PError>(&Result))
        {
            return *pError;
        }
    }
}


CMc5codeParser::CMc5codeParser(std::vector<S7Symbol>& Symbols, size_t& BitAddressCounter, const size_t DbNumber, const std::string& strMc5code, const std::map<std::string, std::map<size_t, std::string>>& Mc5codeMap)
    : m_pszMc5codePosition(strMc5code.c_str()), m_Mc5codeMap(Mc5codeMap), m_BitAddressCounter(BitAddressCounter), m_DbNumber(DbNumber), m_Symbols(Symbols)
{
}

std::variant<std::monostate, CS7PError>
CMc5codeParser::Parse(const std::string& strPrefix)
{
    // Parse the MC5 Code for this DB.
    for (;;)
    {
        // Parse one of the VAR_INPUT/VAR_OUTPUT/VAR_IN_OUT/VAR/STRUCT lines.
        std::string strCurrentStructureType;
        auto Result = _ParseStructureType(strCurrentStructureType);
        if (const auto pError = std::get_if<CS7PError>(&Result))
        {
            return *pError;
        }

        bool bContinueParsing = std::get<bool>(Result);
        if (!bContinueParsing)
        {
            break;
        }

        // Parse the inner struct.
        Result = _ParseInnerStructure(strCurrentStructureType, strPrefix);
        if (const auto pError = std::get_if<CS7PError>(&Result))
        {
            return *pError;
        }

        bContinueParsing = std::get<bool>(Result);
        if (!bContinueParsing)
        {
            break;
        }

        // A new structure starts on a 2-byte boundary.
        _AlignUp(2 * 8);
    }

    return std::monostate();
}
