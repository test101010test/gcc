// { dg-do run { target c++11 } }

// Copyright (C) 2012-2025 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

#include <vector>
#include <set>
#include <testsuite_hooks.h>

class PathPoint
{
public:
  PathPoint(char t, const std::vector<double>& c)
  : type(t), coords(c) { }
  PathPoint(char t, std::vector<double>&& c)
  : type(t), coords(std::move(c)) { }
  char getType() const { return type; }
  const std::vector<double>& getCoords() const { return coords; }
private:
  char type;
  std::vector<double> coords;
};

struct PathPointLess
{
  bool operator() (const PathPoint& __lhs, const PathPoint& __rhs) const
  { return __lhs.getType() < __rhs.getType(); }
};

void test01()
{
  typedef std::set<PathPoint, PathPointLess> Set;
  Set s;

  std::vector<double> coord1 = { 0.0, 1.0, 2.0 };

  auto ret = s.emplace('a', coord1);
  VERIFY( ret.second );
  VERIFY( s.size() == 1 );
  VERIFY( ret.first->getType() == 'a' );

  coord1[0] = 3.0;
  ret = s.emplace('a', coord1);
  VERIFY( !ret.second );
  VERIFY( s.size() == 1 );
  VERIFY( ret.first->getType() == 'a' );
  VERIFY( ret.first->getCoords()[0] == 0.0 );

  auto it = s.emplace_hint(s.begin(), 'b', coord1);
  VERIFY( it != s.end() );
  VERIFY( it->getType() == 'b' );
  VERIFY( it->getCoords()[0] == 3.0 );

  double *px = &coord1[0];
  ret = s.emplace('c', std::move(coord1));
  VERIFY( ret.second );
  VERIFY( ret.first->getType() == 'c' );
  VERIFY( &(ret.first->getCoords()[0]) == px );
}

int main()
{
  test01();
  return 0;
}
