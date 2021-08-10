/*
 ##########################################################################
 # If not stated otherwise in this file or this component's LICENSE
 # file the following copyright and licenses apply:
 #
 # Copyright 2019 RDK Management
 #
 # Licensed under the Apache License, Version 2.0 (the "License");
 # you may not use this file except in compliance with the License.
 # You may obtain a copy of the License at
 #
 # http://www.apache.org/licenses/LICENSE-2.0
 #
 # Unless required by applicable law or agreed to in writing, software
 # distributed under the License is distributed on an "AS IS" BASIS,
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 # See the License for the specific language governing permissions and
 # limitations under the License.
 ##########################################################################
 */

#ifndef SRC_CTRLM_XRAUDIO_HAL_H_
#define SRC_CTRLM_XRAUDIO_HAL_H_

#include "xraudio_hal.h"

typedef void * ctrlm_hal_input_object_t;


typedef enum {
   CTRLM_HAL_INPUT_PTT,
   CTRLM_HAL_INPUT_FF,
   CTRLM_HAL_INPUT_MICS,
   CTRLM_HAL_INPUT_INVALID
} ctrlm_hal_input_device_t;

typedef struct {
   ctrlm_hal_input_device_t device;
   xraudio_input_format_t   input_format;
   int                      fd;
} ctrlm_hal_input_params_t;

typedef struct {
   int bytes_sent;
   void *user_data;
} ctrlm_hal_data_cb_params_t;

typedef void (*ctrlm_data_read_cb_t)(ctrlm_hal_data_cb_params_t *params);

#ifdef __cplusplus
extern "C" {
#endif

ctrlm_hal_input_object_t ctrlm_xraudio_hal_input_open(const ctrlm_hal_input_params_t *params);
ctrlm_hal_input_object_t ctrlm_xraudio_hal_input_session_req(const ctrlm_hal_input_params_t *params);
bool                     ctrlm_xraudio_hal_input_session_begin(ctrlm_hal_input_object_t object, const ctrlm_hal_input_params_t *input_params);
bool                     ctrlm_xraudio_hal_input_set_ctrlm_data_read_cb(ctrlm_hal_input_object_t object, ctrlm_data_read_cb_t callback, void *user_data);
bool                     ctrlm_xraudio_hal_input_use_external_pipe(ctrlm_hal_input_object_t object, int fd);
void                     ctrlm_xraudio_hal_input_close(ctrlm_hal_input_object_t object);
#ifdef __cplusplus
}
#endif

#endif
