# Helper script for building the binary distribution. It's unlikely you'll need this unless you're
# forking the project.
#
#!/bin/bash -x
make deps
make MACHINE=i686 deps
