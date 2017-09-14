/*
Copyright 2017, Intel Corporation
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#
#     * Neither the name of the copyright holder nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <vector>
#include <bitset>
#include <iostream>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/timer.hpp>

using namespace std;
const int MAX_NO_KEYS = 10;
const int MAX_SIZE_KEY = 10;

template<typename C>
void test_custom(string const& s, char const* d, C& ret)
{
	C output;
	bitset<255> delims;
	while( *d )
	{
		unsigned char code = *d++;
		delims[code] = true;
	}
	typedef string::const_iterator iter;
	iter beg;
	bool in_token = false;
	for( string::const_iterator it = s.begin(), end = s.end();
			it != end; ++it )
	{
		if( delims[*it] )
		{
			if( in_token )
			{
				output.push_back(typename C::value_type(beg, it));
				in_token = false;
			}
		}
		else if( !in_token )
		{
			beg = it;
			in_token = true;
		}
	}
	if( in_token )
		output.push_back(typename C::value_type(beg, s.end()));
	output.swap(ret);
}

template<typename C>
void test_strpbrk(string const& s, char const* delims, C& ret)
{
	C output;
    	char const* p = s.c_str();
	char const* q = strpbrk(p+1, delims);
	for( ; q != NULL; q = strpbrk(p, delims) )
	{
		output.push_back(typename C::value_type(p, q));
		p = q + 1;
	}
	output.swap(ret);
}

template<typename C>
void test_boost(string const& s, char const* delims)
{
	C output;
	boost::split(output, s, boost::is_any_of(delims));
}

int main()
{
	string text("dupa1/test2/ala3/i4/dupadupa5/test6/asdsa7/sdf8/g9/ojojoj10");
	  
	char const* delims = "/";
	
	// Output strings
	boost::timer timer;
	test_boost<vector<string> >(text, delims);
	cout << "Boost string time: " << timer.elapsed() << endl;
	
	// Output string views
	typedef string::const_iterator iter;
	typedef boost::iterator_range<iter> string_view;
	timer.restart();
	test_boost<vector<string_view> >(text, delims);
	cout << "Boost string_view time: " << timer.elapsed() << endl;

	// Custom split
	timer.restart();
	vector<string> vs;
	test_custom(text, delims, vs);
	cout << "Custome string time: " << timer.elapsed() << endl;

	// Custom split
	timer.restart();
	vector<string_view> vsv;
	test_custom(text, delims, vsv);
	cout << "Custom string_view time: " << timer.elapsed() << endl;

	// Custom split
	timer.restart();
	vector<string> vsp;
	test_strpbrk(text, delims, vsp);
	cout << "strpbrk string time: " << timer.elapsed() << endl;

	// Custom split
	timer.restart();
	vector<string_view> vsvp;
	test_strpbrk(text, delims, vsvp);
	cout << "strpbrk string_view time: " << timer.elapsed() << endl;

	return 0;
}
