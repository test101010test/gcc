// { dg-do run { target c++11 } }

// Copyright (C) 2009-2025 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

#include <forward_list>
#include <testsuite_containers.h>

namespace __gnu_test
{
  template<>
    struct populate<std::forward_list<int>, true>
    {
      populate(std::forward_list<int>& container)
      {
	container.push_front(1);
	container.push_front(2);
      }      
  };
}

int main()
{
  typedef std::forward_list<int>  test_type;
  __gnu_test::citerator<test_type> test;
  return 0;
}
