//
// EnlyzeS7PLib - Library for parsing symbols in Siemens STEP 7 project files
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#pragma once

#include <map>
#include <string>
#include <variant>
#include <vector>

#include "CMc5ArrayDimension.h"
#include "CS7PError.h"
#include "s7p_parser.h"

class CMc5codeParser
{
public:
    CMc5codeParser(std::vector<S7Symbol>& Symbols, size_t& BitAddressCounter, const size_t DbNumber, const std::string& strMc5code, const std::map<std::string, std::map<size_t, std::string>>& Mc5codeMap);

    std::variant<std::monostate, CS7PError> Parse(const std::string& strPrefix = std::string());

private:
    const char* m_pszMc5codePosition;
    const std::map<std::string, std::map<size_t, std::string>>& m_Mc5codeMap;
    size_t& m_BitAddressCounter;
    size_t m_DbNumber;
    std::vector<S7Symbol>& m_Symbols;
    
    std::variant<std::monostate, CS7PError> _AddArrayVariable(const std::string& strStructureType, const std::string& strVariableName);
    std::variant<std::monostate, CS7PError> _AddBlockVariable(const std::string& strVariableName, const std::string& strVariableType);
    std::variant<std::monostate, CS7PError> _AddPrimitiveVariable(const std::string& strCurrentStructureType, const std::string& strVariableName, const std::string& strVariableType, const std::vector<CMc5ArrayDimension>* pArrayDimensions = nullptr);
    std::variant<std::monostate, CS7PError> _AddSingleVariable(const std::string& strStructureType, const std::string& strVariableName, const std::string& strVariableType);
    std::variant<std::monostate, CS7PError> _AddStructVariable(const std::string& strVariableName);
    std::variant<std::monostate, CS7PError> _AddVariable(const std::string& strStructureType, const std::string& strVariableName);
    void _AlignUp(const size_t BitAlignment);
    std::variant<CMc5ArrayDimension, CS7PError> _GetNextArrayDimensionInfo(const std::string& strVariableName);
    std::variant<std::string, std::monostate> _GetNextToken(const char* szTokens = "", bool bGetComments = false);
    std::variant<bool, CS7PError> _ParseStructureType(std::string& strStructureType);
    std::variant<bool, CS7PError> _ParseInnerStructure(const std::string& strStructureType, const std::string& strPrefix);
};
