/*
 ============================================================================
 Name        : hev-socks5-session.c
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2017 - 2023 hev
 Description : Socks5 Session
 ============================================================================
 */

#include <string.h>

#include "hev-logger.h"
#include "hev-config.h"
#include <lwip/udp.h>

#include "hev-socks5.h"
#include "hev-socks5-client.h"
#include "hev-socks5-session.h"
#include "hev-socks5-session-udp.h"

void
hev_socks5_session_run (HevSocks5Session *self)
{
    HevSocks5SessionIface *iface;
    HevConfigServer *srv;
    int connect_timeout;
    int res;

    LOG_D ("%p socks5 session run", self);

    srv = hev_config_get_socks5_server ();
    connect_timeout = hev_config_get_misc_connect_timeout ();

    hev_socks5_set_timeout (HEV_SOCKS5 (self), connect_timeout);

    res = hev_socks5_client_connect (HEV_SOCKS5_CLIENT (self), srv->addr,
                                     srv->port);
    if (res < 0) {
        LOG_E ("%p socks5 session connect", self);
        return;
    }

    if (HEV_SOCKS5 (self)->type == HEV_SOCKS5_TYPE_TCP) {
        int timeout = hev_config_get_misc_read_write_timeout ();
        hev_socks5_set_timeout (HEV_SOCKS5 (self), timeout);
    } else {
        HevSocks5SessionUDP *udp = HEV_SOCKS5_SESSION_UDP (self);
        if (udp->pcb->local_port == 53) {
            hev_socks5_set_timeout (HEV_SOCKS5 (self), 10000);
        } else {
            int timeout = hev_config_get_misc_udp_timeout ();
            hev_socks5_set_timeout (HEV_SOCKS5 (self), timeout);
        }
    }

    if (srv->user && srv->pass) {
        hev_socks5_client_set_auth (HEV_SOCKS5_CLIENT (self), srv->user,
                                    srv->pass);
        LOG_D ("%p socks5 client auth %s:%s", self, srv->user, srv->pass);
    }

    res = hev_socks5_client_handshake (HEV_SOCKS5_CLIENT (self), srv->pipeline);
    if (res < 0) {
        LOG_E ("%p socks5 session handshake", self);
        return;
    }

    iface = HEV_OBJECT_GET_IFACE (self, HEV_SOCKS5_SESSION_TYPE);
    iface->splicer (self);
}

void
hev_socks5_session_terminate (HevSocks5Session *self)
{
    HevSocks5SessionIface *iface;

    LOG_D ("%p socks5 session terminate", self);

    iface = HEV_OBJECT_GET_IFACE (self, HEV_SOCKS5_SESSION_TYPE);
    hev_socks5_set_timeout (HEV_SOCKS5 (self), 0);
    hev_task_wakeup (iface->get_task (self));
}

void
hev_socks5_session_set_task (HevSocks5Session *self, HevTask *task)
{
    HevSocks5SessionIface *iface;

    iface = HEV_OBJECT_GET_IFACE (self, HEV_SOCKS5_SESSION_TYPE);
    iface->set_task (self, task);
}

HevListNode *
hev_socks5_session_get_node (HevSocks5Session *self)
{
    HevSocks5SessionIface *iface;

    iface = HEV_OBJECT_GET_IFACE (self, HEV_SOCKS5_SESSION_TYPE);
    return iface->get_node (self);
}

void *
hev_socks5_session_iface (void)
{
    static char type;

    return &type;
}
