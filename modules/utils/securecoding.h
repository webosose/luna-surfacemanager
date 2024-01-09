// Copyright (c) 2024 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef SECURECODING_H
#define SECURECODING_H

#include <climits>
#include <math.h>
#include <QDebug>

static int double2int(double val)
{
    if (isgreater(val, double(INT_MAX)))
    {
        qWarning() << "This conversion from double to int may result in data lost, because the value exceeds INT_MAX. Before: " << val << ", After: " << INT_MAX;
        return INT_MAX;
    }

    if (isless(val, double(INT_MIN)))
    {
        qWarning() << "This conversion from double to int may result in data lost, because the value is less than INT_MIN. Before: " << val << ", After: " << INT_MIN;
        return INT_MIN;
    }

    return static_cast<int>(val);
}

static uint32_t double2uint(double val)
{
    if (isgreater(val, double(UINT_MAX)))
    {
        qWarning() << "This conversion from double to u_int may result in data lost, because the value exceeds UINT_MAX. Before: " << val << ", After: " << UINT_MAX;
        return UINT_MAX;
    }

    if (isless(val, double(0)))
    {
        qWarning() << "This conversion from double to u_int may result in data lost, because the value is less than 0. Before: " << val << ", After: " << 0;
        return UINT_MAX;
    }

    return static_cast<uint32_t>(val);
}

#endif
