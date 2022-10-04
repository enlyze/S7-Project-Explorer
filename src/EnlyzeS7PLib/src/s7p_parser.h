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

#include "CS7PError.h"

struct S7Symbol
{
    std::string strName;
    std::string strCode;
    std::string strDatatype;
    std::string strComment;
};

struct S7Block
{
    std::string strName;
    std::vector<S7Symbol> Symbols;
};

struct S7DeviceSymbolInfo
{
    std::string strName;
    std::vector<S7Block> Blocks;
    std::map<size_t, std::string> DbNamesMap;
    std::vector<CS7PError> Warnings;
};

std::variant<std::vector<S7DeviceSymbolInfo>, CS7PError> ParseS7P(
    const std::wstring& wstrS7PFilePath
    );
