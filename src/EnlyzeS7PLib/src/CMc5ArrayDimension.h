//
// EnlyzeS7PLib - Library for parsing symbols in Siemens STEP 7 project files
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#pragma once

#include <string>

struct CMc5ArrayDimension
{
public:
    std::string AsString() const { return std::to_string(StartIndex) + ".." + std::to_string(EndIndex); };

    short StartIndex;
    short EndIndex;
};
