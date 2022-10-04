//
// S7-Project-Explorer - GUI for browsing variables in Siemens STEP 7 projects and exporting the list
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#pragma once

std::variant<std::monostate, CS7PError> ExportCSV(const std::wstring& wstrCSVFilePath, const std::vector<S7DeviceSymbolInfo>& DeviceSymbolInfos);
