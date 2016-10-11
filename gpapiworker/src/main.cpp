
#include <string>
#include <syslog.h>
#include <unistd.h>
#include <stdlib.h>

#include <iostream>

#include <sys/types.h>
#include <pwd.h>

       


using std::string;



int main( int argc, char** argv ){

	struct passwd *current_user = getpwnam( "gpproduction" );

	setuid( current_user->pw_uid );

	setlogmask( LOG_UPTO(LOG_NOTICE) );

	openlog( argv[0], LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1 );

	syslog( LOG_NOTICE, "GP API worker started by User %d", getuid() );

	string argument;

	if( argc > 1 ){
		argument = argv[1];
	}

	string help;
	char *env_help = secure_getenv( "HELP" );
	if( env_help != 0 ){
		help = env_help;
	}


	int x = 0;

	while( x < 20 ){

		++x;

		string message( "HELP=" + help + " argument=" + argument + " x=" + std::to_string(x) );
		syslog( LOG_NOTICE, "%s", message.c_str() );

		sleep(1);

	}

	closelog ();

	return 0;

}

