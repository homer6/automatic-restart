
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>

#include <sys/wait.h>


using std::string;
using std::vector;
using std::cout;
using std::endl;


pid_t start_api_server( vector<pid_t> &apiserver_pids );
pid_t start_api_worker( vector<pid_t> &apiworker_pids );


int main( int argc, char** argv ){

	//vector<int> child_process_ids;

	setlogmask( LOG_UPTO(LOG_NOTICE) );

	std::vector<pid_t> apiserver_pids;
	std::vector<pid_t> apiworker_pids;


	openlog( argv[0], LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1 );

	syslog( LOG_NOTICE, "Program started by User %d", getuid() );

	if( argc > 1 ){

		int status;

		string command( argv[1] );

		const char *pid_filename = "/run/gpd.pid";

		std::ifstream input_pid_file( pid_filename, std::ifstream::binary );


		if( command == "status" ){

			if( input_pid_file ){
				pid_t pid;
				input_pid_file >> pid;
				cout << "Service is running - PID: " << pid << endl;
			}else{
				cout << "Service is not running." << endl;
			}

			exit(0);

		}else if( command == "start" ){

			if( input_pid_file ){
				pid_t pid;
				input_pid_file >> pid;
				cout << "Service is already running - PID: " << pid << endl;
				exit(0);
			}

			daemon(1,1);

		}else if( command == "stop" ){

			if( input_pid_file ){
				pid_t pid;
				input_pid_file >> pid;

				cout << "Stopping - PID: " << pid << "... ";
				killpg( pid, SIGTERM );
				cout << "stopped." << endl;

				remove( pid_filename );
				exit(0);

			}else{

				cout << "Service was not running." << endl;
				exit(0);

			}

		}else if( command == "restart" ){

			if( input_pid_file ){
				pid_t pid;
				input_pid_file >> pid;

				cout << "Stopping - PID: " << pid << "... ";
				killpg( pid, SIGTERM );
				cout << "stopped." << endl;				

			}else{

				cout << "Service was not running." << endl;

			}

			cout << "Restarting... started." << endl;

			daemon(1,1);


		}else{
			cout << "Unknown command." << endl;
			exit(1);
		}
		

		std::ofstream output_pid_file( pid_filename, std::ofstream::out );
		output_pid_file << getpid();
		output_pid_file.close();

		for( int z = 0; z < 5; ++z ){
			start_api_server(apiserver_pids);
		}

		for( int z = 0; z < 10; ++z ){
			start_api_worker(apiworker_pids);
		}


		//cout << "Child pid: " << pid << endl;
		//child_process_ids.push_back( pid );

		int x = 0;

        do {

        	++x;

            pid_t child_pid = waitpid(-1, &status, WUNTRACED | WCONTINUED );

            if( child_pid == -1 ){
                perror("waitpid");
                
                exit(EXIT_FAILURE);
            }

            auto api_server_it = std::find(apiserver_pids.begin(), apiserver_pids.end(), child_pid);
            auto api_worker_it = std::find(apiworker_pids.begin(), apiworker_pids.end(), child_pid);

			if( api_server_it != apiserver_pids.end() ){

				apiserver_pids.erase(api_server_it);

				syslog( LOG_NOTICE, "API Server PID Stopped: %d", child_pid );

	            if( WIFEXITED(status) ){
	            	syslog( LOG_WARNING, "API Server PID (%d) exited with status %d.", child_pid, WEXITSTATUS(status) );
	                start_api_server(apiserver_pids);
	            } else if (WIFSIGNALED(status)) {
	            	syslog( LOG_ERR, "API Server PID (%d) killed by signal: %d", child_pid, WTERMSIG(status) );
	                start_api_server(apiserver_pids);
	            } else if (WIFSTOPPED(status)) {
	            	syslog( LOG_ERR, "API Server PID (%d) stopped by signal: %d", child_pid, WSTOPSIG(status) );
	                start_api_server(apiserver_pids);
	            } else if (WIFCONTINUED(status)) {
	            	syslog( LOG_NOTICE, "API Server PID (%d) continued.", child_pid );
	                start_api_server(apiserver_pids);
	            }

			}else if( api_worker_it != apiworker_pids.end() ){

				apiworker_pids.erase(api_worker_it);

				syslog( LOG_NOTICE, "API Worker PID Stopped: %d", child_pid );

	            if( WIFEXITED(status) ){
	            	syslog( LOG_WARNING, "API Worker PID (%d) exited with status %d.", child_pid, WEXITSTATUS(status) );
	                start_api_worker(apiworker_pids);
	            } else if (WIFSIGNALED(status)) {
	            	syslog( LOG_ERR, "API Worker PID (%d) killed by signal: %d", child_pid, WTERMSIG(status) );
	                start_api_worker(apiworker_pids);
	            } else if (WIFSTOPPED(status)) {
	            	syslog( LOG_ERR, "API Worker PID (%d) stopped by signal: %d", child_pid, WSTOPSIG(status) );
	                start_api_worker(apiworker_pids);
	            } else if (WIFCONTINUED(status)) {
	            	syslog( LOG_NOTICE, "API Worker PID (%d) continued.", child_pid );
	                start_api_worker(apiworker_pids);
	            }

			}else{

				syslog( LOG_ERR, "Unrecognized PID (%d) stopped.", child_pid );

			}

			/*
            if( x > 30 ){
            	remove( pid_filename );
            	exit(0);
            }
            */

        } while( 1 );
        //} while( !WIFEXITED(status) && !WIFSIGNALED(status) );

	}

	//sleep( 20 );

	return 0;

}










pid_t start_api_server( vector<pid_t> &apiserver_pids ){

	pid_t pid = fork();

	if( pid == 0 ){
		//child

		const char* child_args[] = { "gpapiserver", "Hello", "world!", NULL };
		const char* child_envp[] = { "HELP=doggy", NULL };

		int result = execve( "/home/user/dev/GP-Stack/apps/gp/bin/gpapiserver/src/gpapiserver", const_cast<char**>(child_args), const_cast<char**>(child_envp) );

		//result will always be -1 here because execve doesn't return if successful
		if( result == -1 ){

			int e = errno;
			const char *msg = "Failed to execve";

			cout << msg << endl;
			if (e) 
				syslog( LOG_ERR, "%s [%s]", msg, strerror(e) );
			else
				syslog( LOG_ERR, "%s", msg );

		}

	}else if( pid == -1 ){
		//error

		int e = errno;

		const char *msg = "Failed to fork";

		cout << msg << endl;
		if (e) 
			syslog( LOG_ERR, "%s [%s]", msg, strerror(e) );
		else
			syslog( LOG_ERR, "%s", msg );

	}else{
		//parent
		
		apiserver_pids.push_back( pid );

		return pid;
		//cout << "Child pid: " << pid << endl;
		//child_process_ids.push_back( pid );

	}

	return -1;

}




pid_t start_api_worker( vector<pid_t> &apiworker_pids ){

	pid_t pid = fork();

	if( pid == 0 ){
		//child

		const char* child_args[] = { "gpapiworker", "WORKER", "YAYA", NULL };
		const char* child_envp[] = { "HELP=cat", NULL };

		int result = execve( "/home/user/dev/GP-Stack/apps/gp/bin/gpapiworker/src/gpapiworker", const_cast<char**>(child_args), const_cast<char**>(child_envp) );

		//result will always be -1 here because execve doesn't return if successful
		if( result == -1 ){

			int e = errno;
			const char *msg = "Failed to execve";

			if (e) 
				syslog( LOG_ERR, "%s [%s]", msg, strerror(e) );
			else
				syslog( LOG_ERR, "%s", msg );

		}

	}else if( pid == -1 ){
		//error

		int e = errno;

		const char *msg = "Failed to fork";

		cout << msg << endl;
		if (e) 
			syslog( LOG_ERR, "%s [%s]", msg, strerror(e) );
		else
			syslog( LOG_ERR, "%s", msg );

	}else{
		//parent
		
		apiworker_pids.push_back( pid );

		return pid;
		//cout << "Child pid: " << pid << endl;
		//child_process_ids.push_back( pid );

	}

	return -1;

}
