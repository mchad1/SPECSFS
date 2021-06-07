/**
 *  @copyright
 *  Copyright (c) 2003-2020 by Iozone.org
 *      All rights reserved.
 *              Iozone.org
 *              7417 Crenshaw Dr.
 *              Plano, TX 75025
 *
 *      This product contains benchmarks acquired from several sources who
 *      understand and agree with Iozone's goal of creating fair and objective
 *      benchmarks to measure computer performance.
 *
 *      This copyright notice is placed here only to protect Iozone.org in the
 *      event the source is misused in any manner that is contrary to the
 *      spirit, the goals and the intent of Iozone.org
 *
 *  Author: Udayan Bapat, NetApp Inc.
 *
 */
#include "./copyright.txt"
#include "./license.txt"

#ifndef __NETMIST_FSM_H__
#define __NETMIST_FSM_H__

typedef enum _fsm_states
{
    /*
     * Prime States
     */
    PRIME_FSM_DUMMY = 0,
    PRIME_FSM_START,
    PRIME_FSM_SPAWN_NMS,
    PRIME_FSM_JOIN_WAIT_NMS,
    PRIME_FSM_VERSION_CHECK,
    PRIME_FSM_VERSION_CHECK_DONE,
    PRIME_FSM_TOKENFILE_TO_NMS,
    PRIME_FSM_NMS_WAIT_SPAWN,
    PRIME_FSM_KEEPALIVE_WAIT,
    PRIME_FSM_GO,
    PRIME_FSM_GO_DONE,
    PRIME_FSM_INIT_PHASE,
    PRIME_FSM_INIT_PHASE_DONE,
    PRIME_FSM_ASK_INIT_RESULTS,
    PRIME_FSM_GET_INIT_RESULTS,
    PRIME_FSM_TIME_SYNC,
    PRIME_FSM_TIME_SYNC_DONE,
    PRIME_FSM_EPOCH,
    PRIME_FSM_EPOCH_DONE,
    PRIME_FSM_WARMUP_PHASE,
    PRIME_FSM_WARMUP_PHASE_DONE,
    PRIME_FSM_RUN_PHASE,
    PRIME_FSM_RUN_PHASE_DONE,
    PRIME_FSM_ASK_RESULTS,
    PRIME_FSM_GET_RESULTS,
    PRIME_FSM_COMPLETE_PHASE,
    PRIME_FSM_SHUTDOWN_PHASE,
    PRIME_FSM_FINISHED,
    PRIME_FSM_ERROR_STATE,
    PRIME_FSM_DEBUG_STATE,
    PRIME_FSM_MAX_STATE,

    /*
     * NodeManager states
     */
    NODEMANAGER_FSM_DUMMY,
    NODEMANAGER_FSM_FORK,
    NODEMANAGER_FSM_START,
    NODEMANAGER_FSM_PRIME_CONNECT,
    NODEMANAGER_FSM_PRIME_JOIN,
    NODEMANAGER_FSM_VERSION_CHECK,
    NODEMANAGER_FSM_VERSION_CHECK_DONE,
    NODEMANAGER_FSM_GET_TOKEN_CONFIG,
    NODEMANAGER_FSM_SPAWN_CLIENTS,
    NODEMANAGER_WAIT_FOR_CLIENTS,
    NODEMANAGER_JOIN_CLIENTS,
    NODEMANAGER_KEEPALIVE_WAIT,
    NODEMANAGER_FSM_PRIME_READY,
    NODEMANAGER_FSM_GO,
    NODEMANAGER_FSM_GO_DONE,
    NODEMANAGER_FSM_INIT_PHASE,
    NODEMANAGER_FSM_INIT_PHASE_DONE,
    NODEMANAGER_FSM_ASK_INIT_RESULTS,
    NODEMANAGER_FSM_GET_INIT_RESULTS,
    NODEMANAGER_FSM_TIME_SYNC,
    NODEMANAGER_FSM_TIME_SYNC_DONE,
    NODEMANAGER_FSM_EPOCH,
    NODEMANAGER_FSM_EPOCH_DONE,
    NODEMANAGER_FSM_WARMUP_PHASE,
    NODEMANAGER_FSM_WARMUP_PHASE_DONE,
    NODEMANAGER_FSM_RUN_PHASE,
    NODEMANAGER_FSM_RUN_PHASE_DONE,
    NODEMANAGER_FSM_ASK_RESULTS,
    NODEMANAGER_FSM_GET_RESULTS,
    NODEMANAGER_FSM_COMPLETE_PHASE,
    NODEMANAGER_FSM_SHUTDOWN_PHASE,
    NODEMANAGER_FSM_FINISHED,
    NODEMANAGER_FSM_ERROR_STATE,
    NODEMANAGER_FSM_SYNC_OK_STATE,
    NODEMANAGER_FSM_DEBUG_STATE,
    NODEMANAGER_FSM_MAX_STATE,

    /* 
     * Client states
     */
    CLIENT_FSM_DUMMY,
    CLIENT_FSM_START,
    CLIENT_FSM_NODEMANAGER_CONNECT,
    CLIENT_FSM_NODEMANAGER_JOIN,
    CLIENT_OPEN_KEEPALIVE,
    CLIENT_FSM_GO,
    CLIENT_FSM_GO_DONE,
    CLIENT_FSM_INIT_PHASE,
    CLIENT_FSM_INIT_PHASE_DONE,
    CLIENT_FSM_INIT_RESULT_REQUEST,
    CLIENT_FSM_INIT_SEND_RESULT,
    CLIENT_FSM_TIME_SYNC,
    CLIENT_FSM_TIME_SYNC_DONE,
    CLIENT_FSM_EPOCH,
    CLIENT_FSM_EPOCH_DONE,
    CLIENT_FSM_WARMUP_PHASE,
    CLIENT_FSM_WARMUP_PHASE_DONE,
    CLIENT_FSM_RUN_PHASE,
    CLIENT_FSM_RUN_PHASE_DONE,
    CLIENT_FSM_RESULT_REQUEST,
    CLIENT_FSM_SEND_RESULT,
    CLIENT_FSM_COMPLETE_PHASE,
    CLIENT_FSM_SHUTDOWN_PHASE,
    CLIENT_FSM_FINISHED,
    CLIENT_FSM_ERROR_STATE,
    CLIENT_FSM_SYNC_OK_STATE,
    CLIENT_FSM_DEBUG_STATE,
    CLIENT_FSM_MAX_STATE
} _FSM_STATES;

extern _FSM_STATES fsm_state;
extern _FSM_STATES fsm_prev_state;
extern _FSM_STATES fsm_next_state;

extern const char *_fsm_state_names[];

#endif