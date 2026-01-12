//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2025 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : corestrcasecmp.cpp
// Description : C Runtime fallback strcasecmp/stricmp implementation
//
//************************************************************************************************

#include <string.h>
#include <ctype.h>
#include <sys/types.h>

//************************************************************************************************
// strcasecmp
//************************************************************************************************

int strcasecmp (char const* s1, char const* s2)
{
    unsigned char c1 = '\0';
    unsigned char c2 = '\0';

    while(1) 
	{
        c1 = *s1;
        c2 = *s2;
        s1++;
        s2++;
        if(!c1)
            break;
        if(!c2)
            break;
        if(c1 == c2)
            continue;
        c1 = tolower(c1);
        c2 = tolower(c2);
        if(c1 != c2)
            break;
    }
    return (int)c1 - (int)c2;
}
