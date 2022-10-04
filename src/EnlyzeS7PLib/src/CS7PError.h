//
// EnlyzeS7PLib - Library for parsing symbols in Siemens STEP 7 project files
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#pragma once

#include <string>

class CS7PError
{
public:
    explicit CS7PError() {}
    explicit CS7PError(const std::wstring& wstrMessage) : m_wstrMessage(wstrMessage) {}

    const std::wstring& Message() const { return m_wstrMessage; }

private:
    std::wstring m_wstrMessage;
};
