#ifndef STATETBL_H
#define STATETBL_H

#define SM_DECL(proc,name) \
int proc(void)\
{\
    int sm_success=0;\
    char *sm_name=name;

#define SM_STATES \
    enum {

#define SM_NAMES \
    } sm_state; \
    char * sm_sname[] = {

#define SM_EDECL \
    };

#define SM_START(x) \
    sm_state=x;\
    Syslog('s', "SM (%s): Start => %s", sm_name, sm_sname[sm_state]); \
    while (!sm_success) switch (sm_state)\
    {\
    default: WriteError("Statemachine %s error: state=%d",sm_name,sm_state);\
    sm_success=-1;

#define SM_STATE(x) \
    break;\
    case x: 

#define SM_END \
    }\

#define SM_RETURN \
    return (sm_success != 1);\
}

#define SM_PROCEED(x) \
    if (x != sm_state) {\
	Syslog('s', "SM (%s): %s => %s", sm_name, sm_sname[sm_state], sm_sname[x]);\
    }\
    sm_state=x; break;

#define SM_SUCCESS \
    Syslog('s', "SM (%s): %s => Success", sm_name, sm_sname[sm_state]);\
    sm_success=1; break;

#define SM_ERROR \
    Syslog('s', "SM (%s): %s => Error", sm_name, sm_sname[sm_state]);\
    sm_success=-1; break;

#endif
