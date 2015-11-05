import platform
import re
import sys

name_map = dict(
        redhat = "centos",
        macosx = "macosx",
        )

def map_name(name):
    if name in name_map:
        return name_map[ name ]
    return name

def name():
    sys_name = platform.dist()[ 0 ].lower()
    if '' == sys_name:
        if '' != platform.mac_ver()[ 0 ]:
            sys_name = 'macosx'
    return map_name(sys_name)

def version():
    v = platform.dist()[ 1 ]
    if '' == v:
        v = platform.mac_ver()[ 0 ]
    return v

def arch():
    return platform.machine()

def bus_size():
    return int( re.match( "(\d+)", platform.architecture()[ 0 ] ).group( 1 ) )


if __name__ == "__main__":
    template = "{name}/{version}/{arch}"
    if len( sys.argv ) > 1:
      template = sys.argv[ 1 ]
    sys.stdout.write(
            template.format(
                name    = name(),
                version = version(),
                arch    = arch()
                )
            )


# EOF
