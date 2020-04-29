/*
 * Copyright (c) 2019 Sprint
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef __CSTATS_H
#define __CSTATS_H

#ifdef __cplusplus
extern "C" {
#endif

void csInit(void *logger, long interval);
void csStart(void);
void csStop(void);
int csGetInterval(char **response);
int csGetStatLogging(char **response);
int csUpdateStatLogging(const char *json, char **response);
int csUpdateInterval(const char *json, char **response);
int csGetLive(char **response);
int csGetLiveAll(char **response);
void get_current_file_size(size_t len);
void cli_init(cli_node_t *cli_node, int *cnt_peer);

#ifdef __cplusplus
}
#endif

#endif /* __CSTATS_H */
