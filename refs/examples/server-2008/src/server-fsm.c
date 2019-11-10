/*  
 *  EDIT THIS FILE WITH CAUTION  (server-fsm.c)
 *  
 *  It has been AutoGen-ed  November 30, 2010 at 05:19:22 PM by AutoGen 5.10
 *  From the definitions    server.def
 *  and the template file   fsm
 *
 *  Automated Finite State Machine
 *
 *  copyright (c) 2001-2009 by Bruce Korb - all rights reserved
 *
 *  AutoFSM is free software copyrighted by Bruce Korb.
 *  
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name ``Bruce Korb'' nor the name of any other
 *     contributor may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *  
 *  AutoFSM IS PROVIDED BY Bruce Korb ``AS IS'' AND ANY EXPRESS
 *  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED.  IN NO EVENT SHALL Bruce Korb OR ANY OTHER CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#define DEFINE_FSM
#include "server-fsm.h"
#include <stdio.h>
#include <ctype.h>

/*
 *  Do not make changes to this file, except between the START/END
 *  comments, or it will be removed the next time it is generated.
 */
/* START === USER HEADERS === DO NOT CHANGE THIS COMMENT */
/* А была бы эта строчка выше -- обошлись бы без явного приведения типов ниже... */
#include "server-state.h"
#include "server-cmd.h"
#define DEBUG
/* END   === USER HEADERS === DO NOT CHANGE THIS COMMENT */

#ifndef NULL
#  define NULL 0
#endif

/*
 *  Enumeration of the valid transition types
 *  Some transition types may be common to several transitions.
 */
typedef enum {
    SERVER_FSM_TR_AUTHORIZATION_CMD_QUIT,
    SERVER_FSM_TR_AUTHORIZATION_PASSWORD_ACCEPTED,
    SERVER_FSM_TR_AUTHORIZATION_PASSWORD_INVALID,
    SERVER_FSM_TR_AUTHORIZATION_TIMEOUT,
    SERVER_FSM_TR_AUTHORIZATION_TOO_MANY_ATTEMPTS,
    SERVER_FSM_TR_INIT_CMD_QUIT,
    SERVER_FSM_TR_INIT_CMD_USER,
    SERVER_FSM_TR_INIT_TIMEOUT,
    SERVER_FSM_TR_INVALID,
    SERVER_FSM_TR_SESSION_CMD_PROCEED,
    SERVER_FSM_TR_SESSION_CMD_QUIT,
    SERVER_FSM_TR_SESSION_TIMEOUT,
    SERVER_FSM_TR_USER_KNOWN_CMD_PASSW,
    SERVER_FSM_TR_USER_KNOWN_CMD_QUIT,
    SERVER_FSM_TR_USER_KNOWN_TIMEOUT
} te_server_fsm_trans;
#define SERVER_FSM_TRANSITION_CT  15

/*
 *  the state transition handling map
 *  This table maps the state enumeration + the event enumeration to
 *  the new state and the transition enumeration code (in that order).
 *  It is indexed by first the current state and then the event code.
 */
typedef struct server_fsm_transition t_server_fsm_transition;
struct server_fsm_transition {
    te_server_fsm_state  next_state;
    te_server_fsm_trans  transition;
};
static const t_server_fsm_transition
server_fsm_trans_table[ SERVER_FSM_STATE_CT ][ SERVER_FSM_EVENT_CT ] = {

  /* STATE 0:  SERVER_FSM_ST_INIT */
  { { SERVER_FSM_ST_DONE, SERVER_FSM_TR_INIT_TIMEOUT }, /* EVT:  timeout */
    { SERVER_FSM_ST_INVALID, SERVER_FSM_TR_INVALID }, /* EVT:  cmd_proceed */
    { SERVER_FSM_ST_DONE, SERVER_FSM_TR_INIT_CMD_QUIT }, /* EVT:  cmd_quit */
    { SERVER_FSM_ST_USER_KNOWN, SERVER_FSM_TR_INIT_CMD_USER }, /* EVT:  cmd_user */
    { SERVER_FSM_ST_INVALID, SERVER_FSM_TR_INVALID }, /* EVT:  cmd_passw */
    { SERVER_FSM_ST_INVALID, SERVER_FSM_TR_INVALID }, /* EVT:  password_accepted */
    { SERVER_FSM_ST_INVALID, SERVER_FSM_TR_INVALID }, /* EVT:  password_invalid */
    { SERVER_FSM_ST_INVALID, SERVER_FSM_TR_INVALID } /* EVT:  too_many_attempts */
  },


  /* STATE 1:  SERVER_FSM_ST_USER_KNOWN */
  { { SERVER_FSM_ST_DONE, SERVER_FSM_TR_USER_KNOWN_TIMEOUT }, /* EVT:  timeout */
    { SERVER_FSM_ST_INVALID, SERVER_FSM_TR_INVALID }, /* EVT:  cmd_proceed */
    { SERVER_FSM_ST_DONE, SERVER_FSM_TR_USER_KNOWN_CMD_QUIT }, /* EVT:  cmd_quit */
    { SERVER_FSM_ST_INVALID, SERVER_FSM_TR_INVALID }, /* EVT:  cmd_user */
    { SERVER_FSM_ST_AUTHORIZATION, SERVER_FSM_TR_USER_KNOWN_CMD_PASSW }, /* EVT:  cmd_passw */
    { SERVER_FSM_ST_INVALID, SERVER_FSM_TR_INVALID }, /* EVT:  password_accepted */
    { SERVER_FSM_ST_INVALID, SERVER_FSM_TR_INVALID }, /* EVT:  password_invalid */
    { SERVER_FSM_ST_INVALID, SERVER_FSM_TR_INVALID } /* EVT:  too_many_attempts */
  },


  /* STATE 2:  SERVER_FSM_ST_AUTHORIZATION */
  { { SERVER_FSM_ST_DONE, SERVER_FSM_TR_AUTHORIZATION_TIMEOUT }, /* EVT:  timeout */
    { SERVER_FSM_ST_INVALID, SERVER_FSM_TR_INVALID }, /* EVT:  cmd_proceed */
    { SERVER_FSM_ST_DONE, SERVER_FSM_TR_AUTHORIZATION_CMD_QUIT }, /* EVT:  cmd_quit */
    { SERVER_FSM_ST_INVALID, SERVER_FSM_TR_INVALID }, /* EVT:  cmd_user */
    { SERVER_FSM_ST_INVALID, SERVER_FSM_TR_INVALID }, /* EVT:  cmd_passw */
    { SERVER_FSM_ST_SESSION, SERVER_FSM_TR_AUTHORIZATION_PASSWORD_ACCEPTED }, /* EVT:  password_accepted */
    { SERVER_FSM_ST_INIT, SERVER_FSM_TR_AUTHORIZATION_PASSWORD_INVALID }, /* EVT:  password_invalid */
    { SERVER_FSM_ST_DONE, SERVER_FSM_TR_AUTHORIZATION_TOO_MANY_ATTEMPTS } /* EVT:  too_many_attempts */
  },


  /* STATE 3:  SERVER_FSM_ST_SESSION */
  { { SERVER_FSM_ST_DONE, SERVER_FSM_TR_SESSION_TIMEOUT }, /* EVT:  timeout */
    { SERVER_FSM_ST_SESSION, SERVER_FSM_TR_SESSION_CMD_PROCEED }, /* EVT:  cmd_proceed */
    { SERVER_FSM_ST_DONE, SERVER_FSM_TR_SESSION_CMD_QUIT }, /* EVT:  cmd_quit */
    { SERVER_FSM_ST_INVALID, SERVER_FSM_TR_INVALID }, /* EVT:  cmd_user */
    { SERVER_FSM_ST_INVALID, SERVER_FSM_TR_INVALID }, /* EVT:  cmd_passw */
    { SERVER_FSM_ST_INVALID, SERVER_FSM_TR_INVALID }, /* EVT:  password_accepted */
    { SERVER_FSM_ST_INVALID, SERVER_FSM_TR_INVALID }, /* EVT:  password_invalid */
    { SERVER_FSM_ST_INVALID, SERVER_FSM_TR_INVALID } /* EVT:  too_many_attempts */
  }
};


#define Server_FsmFsmErr_off     19
#define Server_FsmEvInvalid_off  75
#define Server_FsmStInit_off     83


static char const zServer_FsmStrings[222] =
    "** OUT-OF-RANGE **\0"
    "FSM Error:  in state %d (%s), event %d (%s) is invalid\n\0"
    "invalid\0"
    "init\0"
    "user_known\0"
    "authorization\0"
    "session\0"
    "timeout\0"
    "cmd_proceed\0"
    "cmd_quit\0"
    "cmd_user\0"
    "cmd_passw\0"
    "password_accepted\0"
    "password_invalid\0"
    "too_many_attempts\0";

static const size_t aszServer_FsmStates[4] = {
    83,  88,  99,  113 };

static const size_t aszServer_FsmEvents[9] = {
    121, 129, 141, 150, 159, 169, 187, 204, 75 };


#define SERVER_FSM_EVT_NAME(t)   ( (((unsigned)(t)) >= 9) \
    ? zServer_FsmStrings : zServer_FsmStrings + aszServer_FsmEvents[t])

#define SERVER_FSM_STATE_NAME(s) ( (((unsigned)(s)) >= 4) \
    ? zServer_FsmStrings : zServer_FsmStrings + aszServer_FsmStates[s])

#ifndef EXIT_FAILURE
# define EXIT_FAILURE 1
#endif

static int server_fsm_invalid_transition( te_server_fsm_state st, te_server_fsm_event evt );

/* * * * * * * * * THE CODE STARTS HERE * * * * * * * *
 *
 *  Print out an invalid transition message and return EXIT_FAILURE
 */
static int
server_fsm_invalid_transition( te_server_fsm_state st, te_server_fsm_event evt )
{
    /* START == INVALID TRANS MSG == DO NOT CHANGE THIS COMMENT */
    char const * fmt = zServer_FsmStrings + Server_FsmFsmErr_off;
    fprintf( stderr, fmt, st, SERVER_FSM_STATE_NAME(st), evt, SERVER_FSM_EVT_NAME(evt));
    /* END   == INVALID TRANS MSG == DO NOT CHANGE THIS COMMENT */

    return EXIT_FAILURE;
}

/*
 *  Step the FSM.  Returns the resulting state.  If the current state is
 *  SERVER_FSM_ST_DONE or SERVER_FSM_ST_INVALID, it resets to
 *  SERVER_FSM_ST_INIT and returns SERVER_FSM_ST_INIT.
 */
te_server_fsm_state
server_fsm_step(
    te_server_fsm_state server_fsm_state,
    te_server_fsm_event trans_evt,
    const char *cmd,
    void *state )
{
    te_server_fsm_state nxtSt;
    te_server_fsm_trans trans;

    if ((unsigned)server_fsm_state >= SERVER_FSM_ST_INVALID) {
        return SERVER_FSM_ST_INIT;
    }

#ifndef __COVERITY__
    if (trans_evt >= SERVER_FSM_EV_INVALID) {
        nxtSt = SERVER_FSM_ST_INVALID;
        trans = SERVER_FSM_TR_INVALID;
    } else
#endif /* __COVERITY__ */
    {
        const t_server_fsm_transition* pTT =
            server_fsm_trans_table[ server_fsm_state ] + trans_evt;
        nxtSt = pTT->next_state;
        trans = pTT->transition;
    }


    switch (trans) {
    case SERVER_FSM_TR_AUTHORIZATION_CMD_QUIT:
        /* START == AUTHORIZATION_CMD_QUIT == DO NOT CHANGE THIS COMMENT */
//         nxtSt = HANDLE_AUTHORIZATION_CMD_QUIT();
        /* END   == AUTHORIZATION_CMD_QUIT == DO NOT CHANGE THIS COMMENT */
        break;


    case SERVER_FSM_TR_AUTHORIZATION_PASSWORD_ACCEPTED:
        /* START == AUTHORIZATION_PASSWORD_ACCEPTED == DO NOT CHANGE THIS COMMENT */
//         nxtSt = HANDLE_AUTHORIZATION_PASSWORD_ACCEPTED();
        /* END   == AUTHORIZATION_PASSWORD_ACCEPTED == DO NOT CHANGE THIS COMMENT */
        break;


    case SERVER_FSM_TR_AUTHORIZATION_PASSWORD_INVALID:
        /* START == AUTHORIZATION_PASSWORD_INVALID == DO NOT CHANGE THIS COMMENT */
//         nxtSt = HANDLE_AUTHORIZATION_PASSWORD_INVALID();
        /* END   == AUTHORIZATION_PASSWORD_INVALID == DO NOT CHANGE THIS COMMENT */
        break;


    case SERVER_FSM_TR_AUTHORIZATION_TIMEOUT:
        /* START == AUTHORIZATION_TIMEOUT == DO NOT CHANGE THIS COMMENT */
//         nxtSt = HANDLE_AUTHORIZATION_TIMEOUT();
        /* END   == AUTHORIZATION_TIMEOUT == DO NOT CHANGE THIS COMMENT */
        break;


    case SERVER_FSM_TR_AUTHORIZATION_TOO_MANY_ATTEMPTS:
        /* START == AUTHORIZATION_TOO_MANY_ATTEMPTS == DO NOT CHANGE THIS COMMENT */
//         nxtSt = HANDLE_AUTHORIZATION_TOO_MANY_ATTEMPTS();
        /* END   == AUTHORIZATION_TOO_MANY_ATTEMPTS == DO NOT CHANGE THIS COMMENT */
        break;


    case SERVER_FSM_TR_INIT_CMD_QUIT:
        /* START == INIT_CMD_QUIT == DO NOT CHANGE THIS COMMENT */
//         nxtSt = HANDLE_INIT_CMD_QUIT();
        /* END   == INIT_CMD_QUIT == DO NOT CHANGE THIS COMMENT */
        break;


    case SERVER_FSM_TR_INIT_CMD_USER:
        /* START == INIT_CMD_USER == DO NOT CHANGE THIS COMMENT */
	/* 
	 * Если не удалось обработать команду -- состояние 
	 * менять не будем.
	 */
	if (server_cmd_user(cmd, state))
		nxtSt = SERVER_FSM_ST_INVALID;
//         nxtSt = HANDLE_INIT_CMD_USER();
        /* END   == INIT_CMD_USER == DO NOT CHANGE THIS COMMENT */
        break;


    case SERVER_FSM_TR_INIT_TIMEOUT:
        /* START == INIT_TIMEOUT == DO NOT CHANGE THIS COMMENT */
//         nxtSt = HANDLE_INIT_TIMEOUT();
        /* END   == INIT_TIMEOUT == DO NOT CHANGE THIS COMMENT */
        break;


    case SERVER_FSM_TR_INVALID:
        /* START == INVALID == DO NOT CHANGE THIS COMMENT */
//         server_fsm_invalid_transition( server_fsm_state, trans_evt );
        /* END   == INVALID == DO NOT CHANGE THIS COMMENT */
        break;


    case SERVER_FSM_TR_SESSION_CMD_PROCEED:
        /* START == SESSION_CMD_PROCEED == DO NOT CHANGE THIS COMMENT */
//         nxtSt = HANDLE_SESSION_CMD_PROCEED();
        /* END   == SESSION_CMD_PROCEED == DO NOT CHANGE THIS COMMENT */
        break;


    case SERVER_FSM_TR_SESSION_CMD_QUIT:
        /* START == SESSION_CMD_QUIT == DO NOT CHANGE THIS COMMENT */
//         nxtSt = HANDLE_SESSION_CMD_QUIT();
        /* END   == SESSION_CMD_QUIT == DO NOT CHANGE THIS COMMENT */
        break;


    case SERVER_FSM_TR_SESSION_TIMEOUT:
        /* START == SESSION_TIMEOUT == DO NOT CHANGE THIS COMMENT */
//         nxtSt = HANDLE_SESSION_TIMEOUT();
        /* END   == SESSION_TIMEOUT == DO NOT CHANGE THIS COMMENT */
        break;


    case SERVER_FSM_TR_USER_KNOWN_CMD_PASSW:
        /* START == USER_KNOWN_CMD_PASSW == DO NOT CHANGE THIS COMMENT */
        if (server_cmd_password(cmd, state))
		nxtSt = SERVER_FSM_ST_INVALID;
        /* END   == USER_KNOWN_CMD_PASSW == DO NOT CHANGE THIS COMMENT */
        break;


    case SERVER_FSM_TR_USER_KNOWN_CMD_QUIT:
        /* START == USER_KNOWN_CMD_QUIT == DO NOT CHANGE THIS COMMENT */
//         nxtSt = HANDLE_USER_KNOWN_CMD_QUIT();
        /* END   == USER_KNOWN_CMD_QUIT == DO NOT CHANGE THIS COMMENT */
        break;


    case SERVER_FSM_TR_USER_KNOWN_TIMEOUT:
        /* START == USER_KNOWN_TIMEOUT == DO NOT CHANGE THIS COMMENT */
//         nxtSt = HANDLE_USER_KNOWN_TIMEOUT();
        /* END   == USER_KNOWN_TIMEOUT == DO NOT CHANGE THIS COMMENT */
        break;


    default:
        /* START == BROKEN MACHINE == DO NOT CHANGE THIS COMMENT */
	nxtSt = SERVER_FSM_ST_INVALID;
        server_fsm_invalid_transition(server_fsm_state, trans_evt);
        /* END   == BROKEN MACHINE == DO NOT CHANGE THIS COMMENT */
    }



    /* START == FINISH STEP == DO NOT CHANGE THIS COMMENT */
    /* END   == FINISH STEP == DO NOT CHANGE THIS COMMENT */

    return nxtSt;
}
/*
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of server-fsm.c */
