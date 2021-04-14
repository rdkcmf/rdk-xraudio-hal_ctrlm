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

#ifndef SRC_CTRLM_XRAUDIO_HAL_PRIV_H_
#define SRC_CTRLM_XRAUDIO_HAL_PRIV_H_

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <unistd.h>
#include <semaphore.h>
#include <rdkx_logger.h>
#include <ctrlm_xraudio_hal.h>

#define CTRLM_XRAUDIO_HAL_SESSION_IDENTIFIER (0x89344187)

typedef enum {
   XRAUDIO_STATE_INIT   = 0,
   XRAUDIO_STATE_OPEN   = 1,
   XRAUDIO_STATE_CLOSED = 2
} xraudio_state_t;

typedef void                          (*xraudio_hal_input_init_t)(json_t *obj_config);
typedef xraudio_hal_input_obj_t       (*xraudio_hal_input_open_t)(xraudio_input_format_t format, xraudio_device_input_configuration_t *configuration, json_t *obj_config);
typedef void                          (*xraudio_hal_input_close_t)(void);
typedef uint32_t                      (*xraudio_hal_input_buffer_size_get_t)(void);
typedef int32_t                       (*xraudio_hal_input_read_t)(uint8_t *data, uint32_t size, xraudio_eos_event_t *eos_event);
typedef bool                          (*xraudio_hal_input_mute_t)(bool enable);
typedef bool                          (*xraudio_hal_input_focus_t)(xraudio_sdf_mode_t mode);
typedef bool                          (*xraudio_hal_input_stats_fp_t)(xraudio_hal_input_stats_t *input_stats, bool reset);
typedef bool                          (*xraudio_hal_input_detection_t)(uint32_t chan, bool *ignore);
typedef bool                          (*xraudio_hal_input_eos_cmd_t)(xraudio_eos_cmd_t cmd, uint32_t chan);
typedef bool                          (*xraudio_hal_input_thread_poll_t)(void);
typedef bool                          (*xraudio_hal_input_power_mode_t)(xraudio_power_mode_t power_mode);
typedef bool                          (*xraudio_hal_input_privacy_mode_t)(bool enable);
typedef bool                          (*xraudio_hal_input_stream_start_set_t)(uint32_t start_sample);
typedef bool                          (*xraudio_hal_input_keyword_detector_reset_t)(void);

typedef ctrlm_hal_input_object_t      (*ctrlm_input_hal_open_t)(const xraudio_input_format_t *format, int fd);
typedef void                          (*ctrlm_input_hal_update_fd_t)(int fd);
typedef void                          (*ctrlm_input_hal_close_t)(bool close_fd);
typedef void                          (*ctrlm_input_hal_call_data_read_cb_t)(int32_t bytes_read);
typedef void                          (*ctrlm_input_hal_set_data_read_cb_t)(ctrlm_data_read_cb_t callback, void *user_data);

typedef struct {
   xraudio_hal_dsp_config_t                   dsp_config;
   xraudio_hal_input_init_t                   xraudio_input_init;
   uint32_t                                   identifier;
   sem_t                                      semaphore;
   uint16_t                                   input_capabilities;
   xraudio_state_t                            xraudio_state;
   bool                                       ctrlm_is_open;
   ctrlm_hal_input_device_t                   device;
   xraudio_devices_input_t                    xraudio_device_type;
   int                                        fd;
   xraudio_input_format_t                     format;
   xraudio_device_input_configuration_t       configuration;
   xraudio_hal_input_open_t                   xraudio_input_open;
   xraudio_hal_input_close_t                  xraudio_input_close;
   xraudio_hal_input_buffer_size_get_t        xraudio_input_buffer_size_get;
   xraudio_hal_input_read_t                   xraudio_input_read;
   xraudio_hal_input_mute_t                   xraudio_input_mute;
   xraudio_hal_input_focus_t                  xraudio_input_focus;
   xraudio_hal_input_stats_fp_t               xraudio_input_stats;
   xraudio_hal_input_detection_t              xraudio_input_detection;
   xraudio_hal_input_eos_cmd_t                xraudio_input_eos_cmd;
   xraudio_hal_input_thread_poll_t            xraudio_input_thread_poll;
   xraudio_hal_input_power_mode_t             xraudio_input_power_mode;
   xraudio_hal_input_privacy_mode_t           xraudio_input_privacy_mode;
   xraudio_hal_input_stream_start_set_t       xraudio_input_stream_start_set;
   xraudio_hal_input_keyword_detector_reset_t xraudio_input_keyword_detector_reset;
   ctrlm_input_hal_open_t                     ctrlm_open;
   ctrlm_input_hal_update_fd_t                ctrlm_update_fd;
   ctrlm_input_hal_close_t                    ctrlm_close;
   ctrlm_input_hal_call_data_read_cb_t        ctrlm_call_data_read_cb;
   ctrlm_input_hal_set_data_read_cb_t         ctrlm_set_data_read_cb;
   ctrlm_data_read_cb_t                       ctrlm_data_read_cb;
   void                                      *ctrlm_data_read_cb_user_data;
} ctrlm_hal_input_obj_t;


#ifdef __cplusplus
extern "C" {
#endif

extern ctrlm_hal_input_obj_t *rf4ce_ptt_obj_get();
extern ctrlm_hal_input_obj_t *rf4ce_ff_obj_get();
#ifdef LOCAL_MICS_XRAUDIO_HAL
extern ctrlm_hal_input_obj_t *mic_local_obj_get();
extern void                   mic_local_version(xraudio_version_info_t *version_info, uint32_t *qty);
#endif

xraudio_hal_msg_callback_t xraudio_hal_async_callback_get();

const char *xraudio_state_str(xraudio_state_t state);

#ifdef __cplusplus
}
#endif

#endif
