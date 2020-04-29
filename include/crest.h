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

#ifndef __CREST_H
#define __CREST_H

#ifdef __cplusplus
extern "C" {
#endif

enum CRCommand
{
	eCRCommandGet,
	eCRCommandPost,
	eCRCommandPut,
	eCRCommandDelete
};

typedef int (*CRestStaticHandler)(const char *requestBody, char **responseBody);
typedef int (*CRestDynamicHandler)(const char *param, const char *requestBody, char **responseBody);

void crInit(void *auditLogger, int port, size_t thread_count);
void crStart(void);
void crStop(void);
void crRegisterStaticHandler(enum CRCommand cmd, const char *route, CRestStaticHandler handler);
void crRegisterDynamicHandler(enum CRCommand cmd, const char *baseroute, const char *param, const char *route, CRestDynamicHandler handler);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __CREST_H */
