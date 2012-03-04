#
# Copyright (C) 2009-2012 Chris McClelland
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Helper script for building the binary distribution. It's unlikely you'll need
# this unless you're forking the project.
#
#!/bin/bash -x
cp Makefile Makefile.old
patch Makefile <<EOF
--- Makefile.old	2012-03-04 16:06:03.521488029 +0000
+++ Makefile.new	2012-03-04 16:06:30.325488544 +0000
@@ -20,2 +20,2 @@
-SUBDIRS   := tests #firmware
-#PRE_BUILD := \$(ROOT)/3rd/fx2lib/lib/fx2.lib
+SUBDIRS   := tests firmware
+PRE_BUILD := \$(ROOT)/3rd/fx2lib/lib/fx2.lib
EOF
make deps
rm -f Makefile
mv Makefile.old Makefile
make MACHINE=i686 deps
