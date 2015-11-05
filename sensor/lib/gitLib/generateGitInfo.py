import os
import subprocess
import sys

git_info_template = """
#ifndef _GITLIB_DATA
#define _GITLIB_DATA

#define GIT_REVISION     %d

#endif
"""

if len(sys.argv) < 2:
    print "Usage: {} <dir>".format(__file__)
    sys.exit(-1)
os.chdir(sys.argv[1])

rev_data = None
p = subprocess.Popen(
    "git log -1 --pretty=format:%h", 
    shell = True,
    stdout = subprocess.PIPE,
    stderr = subprocess.PIPE
    )
rev_data, _ = p.communicate()

if len( rev_data ) != 7:
    print "GIT Revision does not seem valid: %s" % rev_data
    sys.exit(1)
git_revision = rev_data

git_info = open("git_info.h", "w")
git_info.write( git_info_template % int( git_revision, 16 ) )
git_info.close()
print "GIT information fully integrated: %s." % git_revision


# EOF
