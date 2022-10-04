//
// EnlyzeS7PLib - Library for parsing symbols in Siemens STEP 7 project files
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#pragma once

#include <vector>

#include "CMc5ArrayDimension.h"

class CMc5ArrayEnumerator
{
public:
    class InvalidIterator
    {
    public:
        InvalidIterator()
        {
        }
    };

    class Iterator
    {
    public:
        Iterator(std::vector<short>&& Indexes, const std::vector<CMc5ArrayDimension>& Dimensions)
            : m_bInvalid(false), m_Dimensions(Dimensions), m_Indexes(Indexes)
        {
        }

        const std::vector<short>& operator*() const
        {
            return m_Indexes;
        }

        void operator++()
        {
            // Increment the index, from the last to the first dimension until we have processed all elements.
            // For an ARRAY [-5..5, 1..2, 3..4], the order would be:
            //   -5,1,3 | -5,1,4 | -5,2,3 | -5,2,4 | -4,1,3 | ...
            for (int i = m_Indexes.size(); --i >= 0;)
            {
                if (m_Indexes[i] != m_Dimensions[i].EndIndex)
                {
                    m_Indexes[i]++;

                    // Reset the indexes of subsequent dimensions to their start indexes when
                    // incrementing this dimension's index.
                    for (size_t j = i + 1; j < m_Dimensions.size(); j++)
                    {
                        m_Indexes[j] = m_Dimensions[j].StartIndex;
                    }

                    return;
                }
            }

            // We couldn't increment any index, so this iterator is no longer valid.
            m_bInvalid = true;
        }

        bool operator!=([[maybe_unused]] const InvalidIterator& other) const
        {
            // Only implemented as required for the `it != enumerator.end()` condition.
            return !m_bInvalid;
        }

    private:
        bool m_bInvalid;
        const std::vector<CMc5ArrayDimension>& m_Dimensions;
        std::vector<short> m_Indexes;
    };

    CMc5ArrayEnumerator(const std::vector<CMc5ArrayDimension>& Dimensions)
        : m_Dimensions(Dimensions)
    {
    }

    Iterator begin()
    {
        std::vector<short> Indexes;
        for (const auto& Dimension : m_Dimensions)
        {
            Indexes.push_back(Dimension.StartIndex);
        }

        return Iterator(std::move(Indexes), m_Dimensions);
    }

    InvalidIterator end()
    {
        return InvalidIterator();
    }

private:
    const std::vector<CMc5ArrayDimension>& m_Dimensions;
};
