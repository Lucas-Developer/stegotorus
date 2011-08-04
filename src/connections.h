/* Copyright 2011 Nick Mathewson, George Kadianakis, Zack Weinberg
   See LICENSE for other credits and copying information
*/

#ifndef CONNECTIONS_H
#define CONNECTIONS_H

/**
   This struct defines the state of one socket-level connection.  Each
   protocol may extend this structure with additional private data by
   embedding it as the first member of a larger structure.  The
   protocol's conn_create() method is responsible only for filling in
   the |cfg| and |mode| fields of this structure, plus any private
   data of course.

   An incoming connection is not associated with a circuit until the
   destination for the other side of the circuit is known.  An outgoing
   connection is associated with a circuit from its creation.
 */
struct conn_t {
  config_t           *cfg;
  char               *peername;
  circuit_t          *circuit;
  struct bufferevent *buffer;
  enum listen_mode    mode;
};

conn_t *conn_create(config_t *cfg);
void conn_free(conn_t *conn);
void conn_start_shutdown(int barbaric);
unsigned long conn_count(void);

/**
   This struct defines a pair of established connections.

   The "upstream" connection is to the higher-level client or server
   that we are proxying traffic for.  The "downstream" connection is
   to the remote peer.  Circuits always have an upstream connection,
   and normally also have a downstream connection; however, a circuit
   that's waiting for SOCKS directives from its upstream will have a
   non-null socks_state field instead.

   A circuit is "open" if both its upstream and downstream connections
   have been established (not just if both conn_t objects exist).
   It is "flushing" if one of the two connections has hit either EOF
   or an error, and we are clearing out the other side's pending
   transmissions before closing it.  Both of these flags are used
   near-exclusively for assertion checks; the actual behavior is
   controlled by changing bufferevent callbacks on the connections.
 */

struct circuit_t {
  conn_t             *upstream;
  conn_t             *downstream;
  socks_state_t      *socks_state;
  unsigned int        is_open : 1;
  unsigned int        is_flushing : 1;
};

int circuit_create(conn_t *up, conn_t *down);
void circuit_create_socks(conn_t *up);
int circuit_add_down(circuit_t *circuit, conn_t *down);
void circuit_free(circuit_t *circuit);

#endif
