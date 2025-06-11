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
#include "hev-socks5-client.h"
#include "misc/hev-utils.h"

#include "hev-socks5-session.h"

void
hev_socks5_session_run (HevSocks5Session *self)
{
    HevSocks5SessionIface *iface;
    HevConfigServer *srv;
    int connect_timeout;
    int dns_timeout_ms;
    int read_write_timeout_ms; // Renamed for clarity from 'read_write_timeout'
    int res;

    LOG_D ("%p socks5 session run", self);

    srv = hev_config_get_socks5_server ();
    connect_timeout = hev_config_get_misc_connect_timeout ();
    dns_timeout_ms = hev_config_get_misc_dns_timeout (); // New
    read_write_timeout_ms = hev_config_get_misc_read_write_timeout ();

    // Determine actual timeout for the connect call
    if (hev_utils_is_hostname(srv->addr)) { // New check
        read_write_timeout_ms = dns_timeout_ms;
        LOG_D ("%p socks5 session using DNS timeout %dms for server %s", self, dns_timeout_ms, srv->addr);
    }

    hev_socks5_set_timeout(HEV_SOCKS5(self), connect_timeout); // Use determined timeout

    res = hev_socks5_client_connect(HEV_SOCKS5_CLIENT(self), srv->addr, srv->port);

    if (res < 0) {
        LOG_E ("%p socks5 session connect to %s failed", self, srv->addr); // Enhanced log
        return;
    }

    // IMPORTANT: Restore timeout for subsequent operations (handshake, etc.)
    // The original code sets read_write_timeout here. This is correct.
    hev_socks5_set_timeout(HEV_SOCKS5(self), read_write_timeout_ms);

    if (srv->user && srv->pass) {
        hev_socks5_client_set_auth (HEV_SOCKS5_CLIENT (self), srv->user,
                                    srv->pass);
        LOG_D ("%p socks5 client auth %s:%s", self, srv->user, srv->pass);
    }

    res = hev_socks5_client_handshake (HEV_SOCKS5_CLIENT (self), srv->pipeline);
    if (res < 0) {
        LOG_E ("%p socks5 session handshake with %s failed", self, srv->addr); // Enhanced log
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
