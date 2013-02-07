/*
    pmacct (Promiscuous mode IP Accounting package)
    pmacct is Copyright (C) 2003-2013 by Paolo Lucente
*/

/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#define __TEE_RECVS_C

#include "../pmacct.h"
#include "tee_plugin.h"
#include "tee_recvs.h"

int tee_recvs_map_id_handler(char *filename, struct id_entry *e, char *value, struct plugin_requests *req, int acct_type)
{
  struct tee_receivers *table = (struct tee_receivers *) req->key_value_table; 
  int pool_idx, recv_idx, pool_id;
  char *endptr = NULL;

  if (table) {
    if (table->num < config.tee_max_receiver_pools) {
      pool_id = strtoull(value, &endptr, 10);

      if (!pool_id || pool_id > UINT32_MAX) {
        Log(LOG_ERR, "ERROR ( %s/%s ): Invalid Pool ID specified. ", config.name, config.type);
        return TRUE;
      }

      /* Ensure no pool ID duplicates */
      for (pool_idx = 0; pool_idx < table->num; pool_idx++) {
	if (pool_id == table->pools[table->num].id) {
	  Log(LOG_ERR, "ERROR ( %s/%s ): Duplicate Pool ID specified: %u. ", config.name, config.type, pool_id);
	  return TRUE;
	}
      }

      table->pools[table->num].id = pool_id;
    }
    else {
      Log(LOG_ERR, "ERROR ( %s/%s ): Maximum amount of receivers pool reached: %u. ", config.name, config.type, config.tee_max_receiver_pools);
      return TRUE;
    }
  }
  else {
    Log(LOG_ERR, "ERROR ( %s/%s ): Receivers table not allocated. ", config.name, config.type);
    return TRUE;
  }

  return FALSE;
}

int tee_recvs_map_ip_handler(char *filename, struct id_entry *e, char *value, struct plugin_requests *req, int acct_type)
{
  struct tee_receivers *table = (struct tee_receivers *) req->key_value_table;
  struct tee_receiver *target = NULL;
  int pool_idx, recv_idx;
  char *str_ptr, *token;

  if (table) {
    str_ptr = value;
    recv_idx = 0;

    while (token = extract_token(&str_ptr, ',')) {
      if (recv_idx < config.tee_max_receivers) {
	target = &table->pools[table->num].receivers[recv_idx];
	target->dest_len = sizeof(target->dest);
	if (!Tee_parse_hostport(token, (struct sockaddr *) &target->dest, &target->dest_len)) recv_idx++;
	else Log(LOG_WARNING, "WARN ( %s/%s ): Invalid receiver %s in map '%s'.\n",
		config.name, config.type, token, filename);
      }
      else {
	Log(LOG_WARNING, "WARN ( %s/%s ): Maximum amount of receivers pool reached %u in map '%s'.\n",
		config.name, config.type, config.tee_max_receiver_pools, filename);
	break;
      }
    }

    if (!recv_idx) {
      Log(LOG_ERR, "ERROR ( %s/%s ): No valid receivers. ", config.name, config.type);
      return TRUE;
    }
    else table->pools[table->num].num = recv_idx;
  }
  else {
    Log(LOG_ERR, "ERROR ( %s/%s ): Receivers table not allocated. ", config.name, config.type);
    return TRUE;
  }

  return FALSE;
}

int tee_recvs_map_tag_handler(char *filename, struct id_entry *e, char *value, struct plugin_requests *req, int acct_type)
{
  struct tee_receivers *table = (struct tee_receivers *) req->key_value_table;
  int pool_idx, recv_idx, ret;

  if (table) ret = load_tags(filename, &table->pools[table->num].tag_filter, value);
  else {
    Log(LOG_ERR, "ERROR ( %s/%s ): Receivers table not allocated. ", config.name, config.type);
    return TRUE;
  }

  if (!ret) return TRUE;
  else return FALSE;
}

void tee_recvs_map_validate(char *filename, struct plugin_requests *req)
{
  struct tee_receivers *table = (struct tee_receivers *) req->key_value_table;

  /* If we have got: a) a valid pool ID and b) at least a receiver
     THEN validate entry ELSE clean up */ 
  if (table->pools[table->num].id && table->pools[table->num].num > 0) table->num++;
  else {
    table->pools[table->num].id = 0;
    table->pools[table->num].num = 0;
    memset(table->pools[table->num].receivers, 0, config.tee_max_receivers*sizeof(struct tee_receivers));
  }
}
