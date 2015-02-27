#! /bin/sh

# UCLA CS 111 Lab 1 - Test that valid syntax is processed correctly.

# Copyright 2012-2014 Paul Eggert.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

tmp=$0-$$.tmp
mkdir "$tmp" || exit

(
cd "$tmp" || exit

cat >test.sh <<'EOF'
echo a

if echo test1;
then echo test2;
else echo test3;
fi

sleep 0.123456s | sleep 0.72s

(echo test2
echo test4
echo test3
exec echo test1) > input

cat < input | sort
EOF

cat >test.exp <<'EOF'
a
test1
test2
test1
test2
test3
test4
EOF

../profsh test.sh -p output >test.out 2>test.err || exit

diff -u test.exp test.out || exit
test ! -s test.err || {
  cat test.err
  exit 1
}
test $(cat output | grep "\[[0-9]*\]" | wc -l) == 4 || {
  echo >&2 "There is an unexpected number of processes run!"
  exit 1
}
test $(cat output | grep "[A-Za-z]" | wc -l) == 10 || {
  echo >&2 "There is an incorrect number of commands run!"
  exit 1
}
cat output | sort > output_sorted
diff -u output output_sorted || {
  echo >&2 "There is time interleaving or 2 commands finish at exactly the same
time!"
  exit 1
}
) || exit

rm -fr "$tmp"
