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

#include "ctrlm_xraudio_hal_priv.h"

static xraudio_hal_input_obj_t  rf4ce_ff_xraudio_hal_input_open(xraudio_input_format_t format, xraudio_device_input_configuration_t *configuration, json_t *obj_config);
static void                     rf4ce_ff_xraudio_hal_input_close(void);
static uint32_t                 rf4ce_ff_xraudio_hal_input_buffer_size_get(void);
static int32_t                  rf4ce_ff_xraudio_hal_input_read(uint8_t *data, uint32_t size, xraudio_eos_event_t *eos_event);
static bool                     rf4ce_ff_xraudio_hal_input_mute(bool enable);
static bool                     rf4ce_ff_xraudio_hal_input_focus(xraudio_sdf_mode_t mode);
static bool                     rf4ce_ff_xraudio_hal_input_stats(xraudio_hal_input_stats_t *input_stats, bool reset);

static ctrlm_hal_input_object_t      rf4ce_ff_ctrlm_hal_open(const xraudio_input_format_t *format, int fd);
static void                          rf4ce_ff_ctrlm_hal_close(bool close_fd);

static ctrlm_hal_input_obj_t g_rf4ce_ff = {
      .identifier                    = CTRLM_XRAUDIO_HAL_SESSION_IDENTIFIER,
      .input_capabilities            = XRAUDIO_CAPS_INPUT_FF,
      .xraudio_state                 = XRAUDIO_STATE_CLOSED,
      .ctrlm_is_open                 = false,
      .device                        = CTRLM_HAL_INPUT_FF,
      .xraudio_device_type           = XRAUDIO_DEVICE_INPUT_FF,
      .fd                            = -1,
      .format                        = {
         .container   = XRAUDIO_CONTAINER_INVALID,
         .encoding    = XRAUDIO_ENCODING_INVALID,
         .sample_rate = 0,
         .sample_size = 0,
         .channel_qty = 0
      },
      .xraudio_input_init                   = NULL,
      .xraudio_input_open                   = rf4ce_ff_xraudio_hal_input_open,
      .xraudio_input_close                  = rf4ce_ff_xraudio_hal_input_close,
      .xraudio_input_buffer_size_get        = rf4ce_ff_xraudio_hal_input_buffer_size_get,
      .xraudio_input_read                   = rf4ce_ff_xraudio_hal_input_read,
      .xraudio_input_mute                   = rf4ce_ff_xraudio_hal_input_mute,
      .xraudio_input_focus                  = rf4ce_ff_xraudio_hal_input_focus,
      .xraudio_input_stats                  = rf4ce_ff_xraudio_hal_input_stats,
      .xraudio_input_detection              = NULL,
      .xraudio_input_eos_cmd                = NULL,
      .xraudio_input_thread_poll            = NULL,
      .xraudio_input_power_mode             = NULL,
      .xraudio_input_privacy_mode           = NULL,
      .xraudio_input_privacy_mode_get       = NULL,
      .xraudio_input_stream_start_set       = NULL,
      .xraudio_input_keyword_detector_reset = NULL,
      .xraudio_input_test_mode              = NULL,
      .xraudio_input_stream_params_get      = NULL,
      .xraudio_input_stream_latency_set     = NULL,
      .ctrlm_open                           = rf4ce_ff_ctrlm_hal_open,
      .ctrlm_update_fd                      = NULL,
      .ctrlm_close                          = rf4ce_ff_ctrlm_hal_close,
      .ctrlm_call_data_read_cb              = NULL,
      .ctrlm_set_data_read_cb               = NULL,
      .ctrlm_data_read_cb                   = NULL,
      .ctrlm_data_read_cb_user_data         = NULL
};

ctrlm_hal_input_obj_t* rf4ce_ff_obj_get() {
   return &g_rf4ce_ff;
}

xraudio_hal_input_obj_t rf4ce_ff_xraudio_hal_input_open(xraudio_input_format_t format, xraudio_device_input_configuration_t *configuration, json_t *obj_config) {
   if(g_rf4ce_ff.xraudio_state != XRAUDIO_STATE_INIT) {
      XLOGD_ERROR("invalid xraudio state <%s>", xraudio_state_str(g_rf4ce_ff.xraudio_state));
      return(NULL);
   }
   sem_wait(&g_rf4ce_ff.semaphore);

   g_rf4ce_ff.xraudio_state = XRAUDIO_STATE_OPEN;
   int fd = g_rf4ce_ff.fd;

   if(fd < 0) {
      XLOGD_ERROR("invalid fd <%d>", fd);
   }

   if(format.encoding != g_rf4ce_ff.format.encoding) {
      XLOGD_WARN("encoding mismatch <%s> <%s>", xraudio_encoding_str(format.encoding), xraudio_encoding_str(g_rf4ce_ff.format.encoding));
   }

   configuration->fd = fd;

   sem_post(&g_rf4ce_ff.semaphore);
   return((xraudio_hal_input_obj_t)&g_rf4ce_ff);
}

void rf4ce_ff_xraudio_hal_input_close(void) {
   if(g_rf4ce_ff.xraudio_state != XRAUDIO_STATE_OPEN) {
      XLOGD_ERROR("invalid xraudio state <%s>", xraudio_state_str(g_rf4ce_ff.xraudio_state));
      return;
   }

   sem_wait(&g_rf4ce_ff.semaphore);
   if(g_rf4ce_ff.fd >= 0 && !g_rf4ce_ff.ctrlm_is_open) {
      XLOGD_INFO("Close read pipe - fd <%d>", g_rf4ce_ff.fd);
      close(g_rf4ce_ff.fd);
      g_rf4ce_ff.fd = -1;
   }
   g_rf4ce_ff.xraudio_state = XRAUDIO_STATE_CLOSED;
   sem_post(&g_rf4ce_ff.semaphore);
}

uint32_t rf4ce_ff_xraudio_hal_input_buffer_size_get(void) {
   return 0;
}

int32_t rf4ce_ff_xraudio_hal_input_read(uint8_t *data, uint32_t size, xraudio_eos_event_t *eos_event) {
   if(g_rf4ce_ff.format.encoding == XRAUDIO_ENCODING_OPUS_XVP) { // Some audio formats have variable length packets (OPUS).  To handle this, the packet size is prepended to each XVP packet
      uint8_t packet_size = 0xFF;

      errno = 0;
      int rc = read(g_rf4ce_ff.fd, &packet_size, 1);

      if(rc < 0) {
         int errsv = errno;
         XLOGD_ERROR("pipe read error packet size <%s>", strerror(errsv));
         return(-1);
      } else if(packet_size > size) {
         XLOGD_ERROR("invalid packet size <%u> max <%u>", packet_size, size);
         return(-1);
      }

      rc = read(g_rf4ce_ff.fd, data, packet_size);
      if(rc < 0) {
         int errsv = errno;
         XLOGD_ERROR("pipe read error packet body <%s>", strerror(errsv));
         return(-1);
      } else if(rc != packet_size) {
         XLOGD_ERROR("pipe read mismatch packet size <%u> read <%d>", packet_size, rc);
         return(-1);
      }
      return(rc);
   }

   return read(g_rf4ce_ff.fd, data, size);
}

bool rf4ce_ff_xraudio_hal_input_mute(bool enable) {
   return(false);
}

bool rf4ce_ff_xraudio_hal_input_focus(xraudio_sdf_mode_t mode) {
   return(false);
}

bool rf4ce_ff_xraudio_hal_input_stats(xraudio_hal_input_stats_t *input_stats, bool reset) {
   return(false);
}

ctrlm_hal_input_object_t rf4ce_ff_ctrlm_hal_open(const xraudio_input_format_t *format, int fd) {
   if(g_rf4ce_ff.ctrlm_is_open) {
      XLOGD_ERROR("already open");
      return(NULL);
   }
   sem_wait(&g_rf4ce_ff.semaphore);

   if(g_rf4ce_ff.xraudio_state != XRAUDIO_STATE_CLOSED) {
      XLOGD_ERROR("invalid xraudio state <%s>", xraudio_state_str(g_rf4ce_ff.xraudio_state));
   }

   g_rf4ce_ff.ctrlm_is_open = true;
   g_rf4ce_ff.format        = *format;
   g_rf4ce_ff.fd            = fd;
   g_rf4ce_ff.xraudio_state = XRAUDIO_STATE_INIT;

   sem_post(&g_rf4ce_ff.semaphore);
   return &g_rf4ce_ff;
}

void rf4ce_ff_ctrlm_hal_close(bool close_fd) {
   if(!g_rf4ce_ff.ctrlm_is_open) {
      XLOGD_ERROR("not open");
      return;
   }

   sem_wait(&g_rf4ce_ff.semaphore);
   if(g_rf4ce_ff.xraudio_state == XRAUDIO_STATE_INIT) {
      XLOGD_WARN("Session was requested by not started");
      xraudio_hal_msg_callback_t async_callback = xraudio_hal_async_callback_get();
      if(async_callback) {
         xraudio_hal_msg_input_error_t input_error;
         input_error.header.type       = XRAUDIO_MSG_TYPE_INPUT_ERROR;
         input_error.header.source     = g_rf4ce_ff.xraudio_device_type;
         if(!async_callback((void *)&input_error)) {
            XLOGD_ERROR("Unable to report input error");
         }
      } else {
         XLOGD_ERROR("Async callback is NULL!");
      }
      g_rf4ce_ff.xraudio_state = XRAUDIO_STATE_CLOSED;
   }
   if(close_fd && g_rf4ce_ff.fd >= 0  && (g_rf4ce_ff.xraudio_state == XRAUDIO_STATE_CLOSED)) {
      XLOGD_INFO("Close read pipe - fd <%d>", g_rf4ce_ff.fd);
      close(g_rf4ce_ff.fd);
      g_rf4ce_ff.fd = -1;
   }
   g_rf4ce_ff.ctrlm_is_open = false;
   sem_post(&g_rf4ce_ff.semaphore);
}
