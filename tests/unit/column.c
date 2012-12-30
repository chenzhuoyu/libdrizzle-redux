/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab: 
 *
 *  Drizzle Client & Protocol Library
 *
 * Copyright (C) 2012 Andrew Hutchings (andrew@linuxjedi.co.uk)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 *     * The names of its contributors may not be used to endorse or
 * promote products derived from this software without specific prior
 * written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <yatl/lite.h>

#include <libdrizzle-5.1/libdrizzle.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
  (void) argc;
  (void) argv;
  drizzle_row_t row;
  int num_fields;

  drizzle_st *con= drizzle_create_tcp("localhost", DRIZZLE_DEFAULT_TCP_PORT, "root", NULL, NULL, 0);
  ASSERT_NOT_NULL_(con, "Drizzle connection object creation error");

  drizzle_return_t ret= drizzle_connect(con);
  if (ret == DRIZZLE_RETURN_COULD_NOT_CONNECT)
  {
    const char *error= drizzle_error(con);
    drizzle_quit(con);
    SKIP_IF_(ret == DRIZZLE_RETURN_COULD_NOT_CONNECT, "%s(%s)", error, drizzle_strerror(ret));
  }
  ASSERT_EQ_(DRIZZLE_RETURN_OK, ret, "%s(%s)", drizzle_error(con), strerror(ret));

  drizzle_query_str(con, "DROP SCHEMA IF EXISTS libdrizzle", &ret);
  ASSERT_EQ_(DRIZZLE_RETURN_OK, ret, "CREATE SCHEMA libdrizzle (%s)", drizzle_error(con));

  drizzle_query_str(con, "CREATE SCHEMA libdrizzle", &ret);
  ASSERT_EQ_(DRIZZLE_RETURN_OK, ret, "CREATE SCHEMA libdrizzle (%s)", drizzle_error(con));

  drizzle_result_st *result= drizzle_select_db(con, "libdrizzle", &ret);
  ASSERT_EQ_(DRIZZLE_RETURN_OK, ret, "USE libdrizzle");

  drizzle_query_str(con, "create table libdrizzle.t1 (a int primary key auto_increment, b varchar(255), c timestamp default current_timestamp)", &ret);
  ASSERT_TRUE_(ret == DRIZZLE_RETURN_OK, "create table libdrizzle.t1 (a int primary key auto_increment, b varchar(255), c timestamp default current_timestamp)");

  drizzle_query_str(con, "insert into libdrizzle.t1 (b) values ('this'),('is'),('war')", &ret);
  ASSERT_TRUE_(ret == DRIZZLE_RETURN_OK, "insert into libdrizzle.t1 (b) values ('this'),('is'),('war')");

  result= drizzle_query_str(con, "select * from libdrizzle.t1", &ret);
  ASSERT_TRUE_(ret == DRIZZLE_RETURN_OK, "select * from libdrizzle.t1");

  drizzle_result_buffer(result);
  num_fields= drizzle_result_column_count(result);

  ASSERT_TRUE_(num_fields == 3, "Retrieved bad number of fields");

  int i= 0;
  drizzle_column_st *column;
  while ((row = drizzle_row_next(result)))
  {
    drizzle_column_seek(result, 0);
    int j= 0;
    i++;
    char buf[10];
    snprintf(buf, sizeof(buf), "%d", i);
    ASSERT_EQ_(strcmp(row[0], buf), 0, "Retrieved bad row value");
    while ((column= drizzle_column_next(result)))
    {
      j++;
      ASSERT_EQ_(strcmp(drizzle_column_db(column), "libdrizzle"), 0, "Column has bad DB name");
      ASSERT_EQ_(strcmp(drizzle_column_table(column), "t1"), 0, "Column had bad table name");
      ASSERT_FALSE_((j == 2) && (drizzle_column_max_size(column) != 255), "Column max size wrong %lu != 255", drizzle_column_max_size(column));

      ASSERT_FALSE_((j == 2) && (drizzle_column_charset(column) != DRIZZLE_CHARSET_LATIN1_SWEDISH_CI), "Column type wrong, %d != %d", drizzle_column_charset(column), DRIZZLE_CHARSET_UTF8_BIN);
      ASSERT_FALSE_((j == 3) && (drizzle_column_type(column) != DRIZZLE_COLUMN_TYPE_TIMESTAMP), "Column type wrong");
    }
    ASSERT_EQ_(j, 3, "Wrong column count");
  }
  /* Should have had 3 rows */
  ASSERT_EQ_(i, 3, "Retrieved bad number of rows");

  drizzle_result_free(result);

  drizzle_query_str(con, "DROP TABLE libdrizzle.t1", &ret);
  ASSERT_EQ_(DRIZZLE_RETURN_OK, ret, "DROP TABLE libdrizzle.t1");

  drizzle_query_str(con, "DROP SCHEMA IF EXISTS libdrizzle", &ret);
  ASSERT_EQ_(DRIZZLE_RETURN_OK, ret, "DROP SCHEMA libdrizzle (%s)", drizzle_error(con));

  ret= drizzle_quit(con);
  ASSERT_EQ_(DRIZZLE_RETURN_OK, ret, "%s", drizzle_strerror(ret));

  return EXIT_SUCCESS;
}