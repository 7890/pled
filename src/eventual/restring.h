// ----------------------------------------------------------------------------
//
//  Copyright (C) 2015 Thomas Brand <tom@trellis.ch>
//    
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//  This file is part of pled
//  https://github.com/7890/pled
//
//tb/150716+
// ----------------------------------------------------------------------------

#ifndef restring_H_INCLUDED
#define restring_H_INCLUDED

#include <iostream>
#include <string.h>
#include <regex.h>
#include <inttypes.h>

using namespace std;

//=============================================================
void trim_leading(string& s)
{
	int p = s.find_first_not_of(" \t");
	if(p==std::string::npos)
	{
		s.erase(0,s.length());
	}
	else
	{
		s.erase(0, p);
	}
}

//=============================================================
void trim_trailing(string& s)
{
	int p = s.find_last_not_of(" \t");
	if(p==std::string::npos)
	{
		s.erase(0,s.length());
	}
	else
	{
		s.erase(p+1, s.length());
	}
}

/*
this will strip the ^M
pastebin "raw" adds these at end of lines

cat -A revelas:
/-this is a txl file^M$

should be:
/-this is a txl file$

*/

//=============================================================
void remove_ctrl_m(string& s)
{
	int p = s.find("\r");
	if(p!=-1)
	{
		s.erase(p);
	}
}

//=============================================================
//escape text for xml
//http://stackoverflow.com/questions/5665231/most-efficient-way-to-escape-xml-html-in-c-string
//currently unused
void encode(string& data)
{
	string buffer;
	buffer.reserve(data.size());
	for(size_t pos = 0; pos != data.size(); ++pos)
	{
		switch(data[pos])
		{
			case '&':	buffer.append("&amp;");		break;
			case '\"':	buffer.append("&quot;");	break;
			case '\'':	buffer.append("&apos;");	break;
			case '<':	buffer.append("&lt;");		break;
			case '>':	buffer.append("&gt;");		break;
			default:	buffer.append(&data[pos], 1);	break;
		}
	}
	data.swap(buffer);
}//end encode

//=============================================================
//match regular expression
int re(string s,string pattern)
{
	regex_t regex;
	int reti;

	//man regex
	reti = regcomp(&regex, pattern.c_str(), REG_EXTENDED);
	if (reti)
	{
		fprintf(stderr, "Could not compile regex\n");
		exit(1);
	}

	//execute regular expression
	reti = regexec(&regex, s.c_str(), 0, NULL, 0);

	//match
	if (!reti)
	{
		;;
	}
	//no match
	else if (reti == REG_NOMATCH)
	{
		;;
	}
	else
	{
		char msgbuf[100];
		regerror(reti, &regex, msgbuf, sizeof(msgbuf));
		fprintf(stderr, "Regex match failed: %s\n", msgbuf);
		exit(1);
	}

	//free compiled regular expression if you want to use the regex_t again
	regfree(&regex);

	if(!reti)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}//end re

//=============================================================
int extract_one_digit(string input, uint64_t &arg1)
{
	return sscanf (input.c_str()," %"PRId64"",&arg1);
}

//=============================================================
int extract_two_digits(string input, uint64_t &arg1, uint64_t &arg2)
{
	return sscanf (input.c_str(),"%*s %"PRId64" %"PRId64"",&arg1,&arg2);
}

//=============================================================
int extract_two_digits_(string input, uint64_t &arg1, uint64_t &arg2)
{
	return sscanf (input.c_str()," %"PRId64" %"PRId64"",&arg1,&arg2);
}

#endif
//EOF
