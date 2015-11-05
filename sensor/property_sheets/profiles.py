import operator
import os.path
import shutil


class Component( object ):

    def __init__( self, name, builder_name, *templates, **opts ):
        super( Component, self ).__init__()
        self.name = name
        self.builder_name = builder_name
        self.templates = templates
        self.opts = opts
        self.node = None
        self.deplist = []

    def extend_env( self, env ):
        env.AppendUnique( **self.opts )
        self.add_env_settings( env )

    def add_env_settings( self, env ):
        pass

    def extend_deps( self, env, new_node ):
        env.Depends( new_node, self.node )
        self.add_deps( env, new_node )
 
    def add_deps( self, env, new_node ):
        pass

    def Target( self, env, sources, compmap, *compnames ):
        comp_trans = self._dep_sort( compmap, compnames )
        for compname in comp_trans:
            compmap[ compname ].extend_env( env )
        self.extend_env( env )
        if type( sources ) is str:
            sources = [ sources ]
        self.node = getattr( env, self.builder_name )( 
                build_join( env, self.name ),
                ( 
                    sources +
                    reduce(
                        operator.add,
                        ( t.additional_sources() for t in self.templates ),
                        []
                        )
                    )
                )
        for compname in comp_trans:
            compmap[ compname ].extend_deps( env, self.node )
        for template in self.templates:
            template.execute( env, self.node )
        compmap[ self.name ] = self
        self.deplist = list( compnames )
        return self.node

    def _dep_sort( self, compmap, compnames ):
        complist = []
        self._dep_sort_iter( compmap, compnames, complist )
        return complist

    def _dep_sort_iter( self, compmap, compnames, complist ):
        for name in compnames:
            comp = compmap[ name ]
            if 0 < len( comp.deplist ):
                self._dep_sort_iter( compmap, comp.deplist, complist )
            if name not in complist:
                complist.insert( 0, name )
        return complist


class Program( Component ):

    def __init__( self, name, *templates, **opts ):
        super( Program, self ).__init__( name, "Program", *templates, **opts )


class StaticLibrary( Component ):

    def __init__( self, name, *templates, **opts ):
        super( StaticLibrary, self ).__init__( name, "StaticLibrary", *templates, **opts )

    def extend_env( self, env ):
        env.AppendUnique( CPPPATH = include_join( env ) )
        env.AppendUnique( LIBS = self.name )
        super( StaticLibrary, self ).extend_env( env )


class DynamicLibrary( Component ):

    def __init__( self, name, *templates, **opts ):
        super( DynamicLibrary, self ).__init__( name, "SharedLibrary", *templates, **opts )


class Template( object ):

    def additional_sources( self ):
        return []

    def execute( self, env, node ):
        pass


class ObfuscatedHeader( Template ):

    def execute( self, env, node ):
        process_path = build_join( env, 'lib/obfuscationLib/processObfuscatedHeader.py' )
        env.Depends( 'obfuscated.h', process_path )
        cmd_node = env.Command(
                'obfuscated.h',
                'obfuscated.txt',
                'python %s ./' % no_hash( process_path )
                )
        env.Depends( node, cmd_node )


class RpalModule( Template ):

    def additional_sources( self ):
        return [ 'rpal_module.c' ]

    def execute( self, env, node ):
        cmd_node = env.Command(
                'rpal_module.c',
                build_join( env, 'include/rpal/rpal_module.c' ),
                action = env.Action( self.run )
                )
        env.Depends( node, cmd_node )

    def run( self, target, source, env ):
        shutil.copyfile(
                env.GetBuildPath( source )[ 0 ],
                env.GetBuildPath( target )[ 0 ]
                )


class GitInfo( Template ):

    def execute( self, env, node ):
        process_path = build_join( env, 'lib/gitLib/generateGitInfo.py' )
        env.Depends( 'git_info.h', process_path )
        cmd_node = env.Command(
                'git_info.h',
                'SConscript',
                'python %s `dirname $SOURCES`' % no_hash( process_path )
                )
        env.Depends( node, cmd_node )

def build_join( env, *p ):
    return os.path.join( "#", env[ 'BUILD_DIR' ], *p )

def include_join( env, *suffix ):
    return build_join( env, 'include', *suffix )

def no_hash( path ):
    cp = os.path.commonprefix( [ path, "#/" ] )
    return path[ len( cp ): ]

def make_rpal_master( env ):
    env.Append( CPPDEFINES = 'RPAL_MODE_MASTER' )


# EOF
