#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>


#include "lock.h"



int    lock_file_old(char* file)
{
    // check correctness of lock file
    FILE* f;
    pid_t pid;
    int rc;
    int ret;

    ret = 0;
    f = fopen( file, "rt");
    if (f)
    {
        if (  fscanf(f, "%d", &pid) == 1 )
            if ( ( kill( pid, 0 ) == -1 ) && (errno == ESRCH ) ) ret = 1;
        fclose(f);
    }
    return ret;
}


void    write_lock_file(int fd)
{
    char  str[ 10 ];
    pid_t pid;
    int   rc;

    pid = getpid();
    rc = snprintf( str, sizeof(str), "%d\n", pid );
    if (rc>0)  write( fd, str, rc);
}

void    control_lock(modbus_params_t* params,int lock_type, bool enable)
{
    /* const char filename[]="lock.pid"; */
    int*  pfd;
    int   fd;
    char* lock_file;

    switch(lock_type)
    {
        case LOCK_INPUT:
            pfd = &params->lock_file_in_fd;
            lock_file = params->lock_file_in;
            break;

        case LOCK_OUTPUT:
            pfd = &params->lock_file_out_fd;
            lock_file = params->lock_file_out;
            break;

        default:
            fprintf( stderr, "Unknown lock type\n");
            return;
    }

    if (!lock_file) return;

    if (enable)
    {
        const max_cnt=5000;
        int cnt = 0;
        do
        {
            /* create lock */
            fd = open( lock_file , O_CREAT | O_EXCL | O_WRONLY, S_IRUSR );
            if (fd==-1)
            {
                if (lock_file_old( lock_file ) )
                {
                    /* delete old lock file */
                    unlink( lock_file );
                }
                else
                {
                    cnt++;
                    if (cnt>max_cnt)
                    {
                        fprintf( stderr, "Can't create lock file %s\n", lock_file);
                        exit(RESULT_ERROR);
                    }
                    usleep( 100000 );
               }
            }
        }
        while (fd==-1);
        write_lock_file(fd);
    }
    else
    {
        close( fd );
        unlink( lock_file );
        fd = 0;
    }

    *pfd = fd;
}


void  set_lock(modbus_params_t* params, int lock_type)
{
    control_lock( params, lock_type, true);
}

void  release_lock(modbus_params_t* params, int lock_type)
{
    control_lock( params, lock_type, false);
}