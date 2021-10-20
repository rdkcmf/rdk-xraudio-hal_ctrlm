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

#include <ctrlm_xraudio_hal_priv.h>
#include <ctrlm_xraudio_hal_version.h>
#include <xraudio_hal_config.h>

#define CTRLM_XRAUDIO_HAL_GLOBAL_IDENTIFIER  (0x53426789)

typedef struct {
   uint32_t                   identifier;
   bool                       debug;
   xraudio_power_mode_t       power_mode;
   bool                       privacy_mode;
   xraudio_hal_msg_callback_t async_callback;
   json_t *                   obj_config_mic;
} ctrlm_hal_global_obj_t;

static ctrlm_hal_global_obj_t g_ctrlm_hal_obj = { CTRLM_XRAUDIO_HAL_GLOBAL_IDENTIFIER, false, XRAUDIO_POWER_MODE_INVALID, false, NULL, NULL };

typedef ctrlm_hal_input_obj_t* (*ctrlm_hal_input_obj_get_t)();

static ctrlm_hal_input_obj_get_t ctrlm_hal_input_objs[] = {
   rf4ce_ptt_obj_get,
   rf4ce_ff_obj_get,
#ifdef LOCAL_MICS_XRAUDIO_HAL
   mic_local_obj_get
#endif
};


static bool ctrlm_xraudio_hal_global_obj_is_valid(ctrlm_hal_global_obj_t *obj);
static bool ctrlm_xraudio_hal_input_obj_is_valid(ctrlm_hal_input_obj_t *obj);

void xraudio_hal_version(xraudio_version_info_t *version_info, uint32_t *qty) {
   if(version_info == NULL || qty == NULL || *qty < 3) {
      XLOGD_ERROR("invalid version params <%p> qty <%p><%u>", version_info, qty, (qty == NULL) ? 0 : *qty);  //CID:167917 - printargs
      return;
   }
   uint32_t qty_avail = *qty;

   version_info->name      = "ctrlm-xraudio-hal";
   version_info->version   = CTRLM_XRAUDIO_HAL_VERSION;
   version_info->branch    = CTRLM_XRAUDIO_HAL_BRANCH;
   version_info->commit_id = CTRLM_XRAUDIO_HAL_COMMIT_ID;
   version_info++;
   qty_avail--;

   #ifdef LOCAL_MICS_XRAUDIO_HAL
   uint32_t qty_mic = qty_avail;
   mic_local_version(version_info, &qty_mic);
   version_info += qty_mic;
   qty_avail    -= qty_mic;
   #endif

   *qty -= qty_avail;
}

bool xraudio_hal_init(json_t *obj_config) {
   g_ctrlm_hal_obj.obj_config_mic = NULL;

   // Parse config file
   if(obj_config == NULL || !json_is_object(obj_config)) {
      XLOGD_INFO("using default config");
   } else {
      json_t *obj_mic = json_object_get(obj_config, JSON_OBJ_NAME_MIC);
      if(obj_mic != NULL && json_is_object(obj_mic)) {
         g_ctrlm_hal_obj.obj_config_mic = obj_mic;
      }
   }

   // Init input devices
   for (uint32_t index = 0; index < (sizeof(ctrlm_hal_input_objs)/sizeof(ctrlm_hal_input_obj_get_t)); index++) {
      const ctrlm_hal_input_obj_t *obj = ctrlm_hal_input_objs[index]();
      if (obj->xraudio_input_init != NULL) {
         switch (obj->device) {
            case CTRLM_HAL_INPUT_PTT:
            case CTRLM_HAL_INPUT_FF:
               break;
            case CTRLM_HAL_INPUT_MICS:
               obj->xraudio_input_init(g_ctrlm_hal_obj.obj_config_mic);
               break;
            case CTRLM_HAL_INPUT_INVALID:
               break;
         }
      }
   }

   return(true);
}

void xraudio_hal_capabilities_get(xraudio_hal_capabilities *caps) {
   memset(caps, 0, sizeof(xraudio_hal_capabilities));
   for(caps->input_qty = 0; caps->input_qty < (sizeof(ctrlm_hal_input_objs) / sizeof(ctrlm_hal_input_obj_get_t)); caps->input_qty++) {
      const ctrlm_hal_input_obj_t* obj = ctrlm_hal_input_objs[caps->input_qty]();
      caps->input_caps[caps->input_qty] = obj->input_capabilities;
      XLOGD_INFO("input <%u> capabilities <%s>", caps->input_qty, xraudio_capabilities_input_str(obj->input_capabilities));
   }
}

bool xraudio_hal_dsp_config_get(xraudio_hal_dsp_config_t *dsp_config) {
   for (uint32_t index = 0; index < (sizeof(ctrlm_hal_input_objs)/sizeof(ctrlm_hal_input_obj_get_t)); index++) {
      const ctrlm_hal_input_obj_t *obj = ctrlm_hal_input_objs[index]();
      if (obj->device == CTRLM_HAL_INPUT_MICS) {
         *dsp_config = obj->dsp_config;
         return(true);
      }
   }
   return(false);
}

bool xraudio_hal_available_devices_get(xraudio_devices_input_t *inputs, uint32_t input_qty_max, xraudio_devices_output_t *outputs, size_t output_qty_max) {
   if (input_qty_max < sizeof(ctrlm_hal_input_objs)/sizeof(ctrlm_hal_input_obj_get_t)) {
      XLOGD_ERROR("not enough space allocated for available inputs");
      return(false);
   }

   for(uint32_t index = 0; index < (sizeof(ctrlm_hal_input_objs) / sizeof(ctrlm_hal_input_obj_get_t)); index++) {
      const ctrlm_hal_input_obj_t *obj = ctrlm_hal_input_objs[index]();
      inputs[index] = obj->xraudio_device_type;
   }
   /* No output devices yet */
   return(true);
}

xraudio_hal_obj_t xraudio_hal_open(bool debug, xraudio_power_mode_t power_mode, bool privacy_mode, xraudio_hal_msg_callback_t callback) {
   if(g_ctrlm_hal_obj.async_callback != NULL) {
      XLOGD_ERROR("already open");
      return(NULL);
   }
   g_ctrlm_hal_obj.debug          = debug;
   g_ctrlm_hal_obj.power_mode     = power_mode;
   g_ctrlm_hal_obj.privacy_mode   = privacy_mode;
   g_ctrlm_hal_obj.async_callback = callback;

   sem_init(&rf4ce_ptt_obj_get()->semaphore,   0, 1);
   sem_init(&rf4ce_ff_obj_get()->semaphore,    0, 1);

   return(&g_ctrlm_hal_obj);
}

bool xraudio_hal_power_mode(xraudio_hal_obj_t object, xraudio_power_mode_t power_mode) {
   ctrlm_hal_global_obj_t *obj = (ctrlm_hal_global_obj_t *)object;
   if(!ctrlm_xraudio_hal_global_obj_is_valid(obj)) {
      XLOGD_ERROR("invalid object");
      return(false);
   }
   bool result = true;

   if(g_ctrlm_hal_obj.power_mode != power_mode) {
      for(uint32_t index = 0; index < (sizeof(ctrlm_hal_input_objs) / sizeof(ctrlm_hal_input_obj_get_t)); index++) {
         const ctrlm_hal_input_obj_t *obj = ctrlm_hal_input_objs[index]();
         if(obj->xraudio_input_power_mode != NULL) {
            if(!obj->xraudio_input_power_mode(power_mode)) {
               result = false;
            }
         }
      }
   }
   if(result) {
      g_ctrlm_hal_obj.power_mode = power_mode;
   }

   return(result);
}

bool xraudio_hal_privacy_mode(xraudio_hal_obj_t object, bool enable) {
   ctrlm_hal_global_obj_t *obj = (ctrlm_hal_global_obj_t *)object;
   if(!ctrlm_xraudio_hal_global_obj_is_valid(obj)) {
      XLOGD_ERROR("invalid object");
      return(false);
   }
   bool result = true;

   if(g_ctrlm_hal_obj.privacy_mode != enable) {
      for(uint32_t index = 0; index < (sizeof(ctrlm_hal_input_objs) / sizeof(ctrlm_hal_input_obj_get_t)); index++) {
         const ctrlm_hal_input_obj_t *obj = ctrlm_hal_input_objs[index]();
         if(obj->xraudio_input_privacy_mode != NULL) {
            if(!obj->xraudio_input_privacy_mode(enable)) {
               result = false;
            }
         }
      }
   }
   if(result) {
      g_ctrlm_hal_obj.privacy_mode = enable;
   }

   return(result);
}

bool xraudio_hal_privacy_mode_get(xraudio_hal_obj_t object, bool *enabled) {
   ctrlm_hal_global_obj_t *obj = (ctrlm_hal_global_obj_t *)object;
   if(!ctrlm_xraudio_hal_global_obj_is_valid(obj)) {
      XLOGD_ERROR("invalid object");
      return(false);
   }
   bool result = true;

   for(uint32_t index = 0; index < (sizeof(ctrlm_hal_input_objs) / sizeof(ctrlm_hal_input_obj_get_t)); index++) {
      const ctrlm_hal_input_obj_t *obj = ctrlm_hal_input_objs[index]();
      if(obj->xraudio_input_privacy_mode_get != NULL) {
         if(!obj->xraudio_input_privacy_mode_get(enabled)) {
            result = false;
         }
      }
   }

   return result;
}

void xraudio_hal_close(xraudio_hal_obj_t object) {
   ctrlm_hal_global_obj_t *obj = (ctrlm_hal_global_obj_t *)object;
   if(!ctrlm_xraudio_hal_global_obj_is_valid(obj)) {
      XLOGD_ERROR("invalid object");
      return;
   }

   sem_destroy(&rf4ce_ptt_obj_get()->semaphore);
   sem_destroy(&rf4ce_ff_obj_get()->semaphore);

   obj->debug          = false;
   obj->power_mode     = XRAUDIO_POWER_MODE_INVALID;
   obj->async_callback = NULL;
}

bool xraudio_hal_input_stream_start_set(xraudio_hal_input_obj_t obj, uint32_t start_sample) {
   if(obj == NULL) {
      XLOGD_ERROR("NULL obj");
      return(false);
   }
   ctrlm_hal_input_obj_t *input_obj = (ctrlm_hal_input_obj_t *)obj;
   if(!ctrlm_xraudio_hal_input_obj_is_valid(input_obj)) {
      XLOGD_ERROR("invalid obj");
      return(false);
   }
   if(input_obj->xraudio_input_stream_start_set == NULL) {
      return(false);
   }

   return input_obj->xraudio_input_stream_start_set(start_sample);
}

bool xraudio_hal_input_keyword_detector_reset(xraudio_hal_input_obj_t obj) {
   if(obj == NULL) {
      XLOGD_ERROR("NULL obj");
      return(false);
   }
   ctrlm_hal_input_obj_t *input_obj = (ctrlm_hal_input_obj_t *)obj;
   if(!ctrlm_xraudio_hal_input_obj_is_valid(input_obj)) {
      XLOGD_ERROR("invalid obj");
      return(false);
   }
   if(input_obj->xraudio_input_keyword_detector_reset == NULL) {
      return(false);
   }
   return input_obj->xraudio_input_keyword_detector_reset();
}

bool xraudio_hal_thread_poll(void) {
   for(uint32_t index = 0; index < (sizeof(ctrlm_hal_input_objs) / sizeof(ctrlm_hal_input_obj_get_t)); index++) {
      const ctrlm_hal_input_obj_t *obj = ctrlm_hal_input_objs[index]();
      if(obj->xraudio_input_thread_poll != NULL) {
         if(!obj->xraudio_input_thread_poll()) {
            return(false);
         }
      }
   }
   return(true);
}

xraudio_hal_input_obj_t xraudio_hal_input_open(xraudio_hal_obj_t hal_obj, xraudio_devices_input_t device, xraudio_input_format_t format, xraudio_device_input_configuration_t *configuration) {
   XLOGD_INFO("Opening input device <%s>", xraudio_devices_input_str(device));
   for(uint32_t index = 0; index < (sizeof(ctrlm_hal_input_objs) / sizeof(ctrlm_hal_input_obj_get_t)); index++) {
      const ctrlm_hal_input_obj_t *obj = ctrlm_hal_input_objs[index]();
      XLOGD_DEBUG("input device <%s> open %p",  xraudio_devices_input_str(obj->xraudio_device_type), obj->xraudio_input_open);
      if((obj->xraudio_device_type & device) && obj->xraudio_input_open != NULL) {
         return obj->xraudio_input_open(format, configuration, XRAUDIO_DEVICE_INPUT_LOCAL_GET(device) ? g_ctrlm_hal_obj.obj_config_mic : NULL);
      }
   }
   XLOGD_ERROR("Cannot open unknown input device <%s>",  xraudio_devices_input_str(device));
   return NULL;
}

void xraudio_hal_input_close(xraudio_hal_input_obj_t obj) {
   XLOGD_INFO("Closing input device");
   if(obj == NULL) {
      XLOGD_ERROR("NULL obj");
      return;
   }
   ctrlm_hal_input_obj_t *input_obj = (ctrlm_hal_input_obj_t *)obj;
   if(!ctrlm_xraudio_hal_input_obj_is_valid(input_obj)) {
      XLOGD_ERROR("invalid obj");
      return;
   }
   if(input_obj->xraudio_input_close == NULL) {
      return;
   }
   input_obj->xraudio_input_close();
}

uint32_t xraudio_hal_input_buffer_size_get(xraudio_hal_input_obj_t obj) {
   if(obj == NULL) {
      XLOGD_ERROR("NULL obj");
      return(0);
   }
   ctrlm_hal_input_obj_t *input_obj = (ctrlm_hal_input_obj_t *)obj;
   if(!ctrlm_xraudio_hal_input_obj_is_valid(input_obj)) {
      XLOGD_ERROR("invalid obj");
      return(0);
   }
   if(input_obj->xraudio_input_buffer_size_get == NULL) {
      return(0);
   }
   return input_obj->xraudio_input_buffer_size_get();
}

int32_t xraudio_hal_input_read(xraudio_hal_input_obj_t obj, uint8_t *data, uint32_t size, xraudio_eos_event_t *eos_event) {
   if(obj == NULL) {
      XLOGD_ERROR("NULL obj");
      return(-1);
   }
   ctrlm_hal_input_obj_t *input_obj = (ctrlm_hal_input_obj_t *)obj;
   if(!ctrlm_xraudio_hal_input_obj_is_valid(input_obj)) {
      XLOGD_ERROR("invalid obj");
      return(-1);
   }
   if(input_obj->xraudio_input_read == NULL) {
      return(-1);
   }
   int32_t bytes_read = input_obj->xraudio_input_read(data, size, eos_event);

   if(input_obj->ctrlm_call_data_read_cb != NULL) {
      input_obj->ctrlm_call_data_read_cb(bytes_read);
   }
   return bytes_read;
}

bool xraudio_hal_input_mute(xraudio_hal_input_obj_t obj, xraudio_devices_input_t device, bool enable) {
   if(obj == NULL) {
      XLOGD_ERROR("NULL obj");
      return(false);
   }
   ctrlm_hal_input_obj_t *input_obj = (ctrlm_hal_input_obj_t *)obj;
   if(!ctrlm_xraudio_hal_input_obj_is_valid(input_obj)) {
      XLOGD_ERROR("invalid obj");
      return(false);
   }
   if(input_obj->xraudio_input_mute == NULL) {
      return(false);
   }
   return input_obj->xraudio_input_mute(enable);
}

bool xraudio_hal_input_test_mode(xraudio_hal_input_obj_t obj, bool enable) {
   if(obj == NULL) {
      XLOGD_ERROR("NULL obj");
      return(false);
   }
   ctrlm_hal_input_obj_t *input_obj = (ctrlm_hal_input_obj_t *)obj;
   if(!ctrlm_xraudio_hal_input_obj_is_valid(input_obj)) {
      XLOGD_ERROR("invalid obj");
      return(false);
   }
   if(input_obj->xraudio_input_test_mode == NULL) {
      return(true);
   }
   return input_obj->xraudio_input_test_mode(enable);
}

bool xraudio_hal_input_focus(xraudio_hal_input_obj_t obj, xraudio_sdf_mode_t mode) {
   if(obj == NULL) {
      XLOGD_ERROR("NULL obj");
      return(false);
   }
   ctrlm_hal_input_obj_t *input_obj = (ctrlm_hal_input_obj_t *)obj;
   if(!ctrlm_xraudio_hal_input_obj_is_valid(input_obj)) {
      XLOGD_ERROR("invalid obj");
      return(false);
   }
   if(input_obj->xraudio_input_focus == NULL) {
      return(false);
   }
   return input_obj->xraudio_input_focus(mode);
}

bool xraudio_hal_input_stats(xraudio_hal_input_obj_t obj, xraudio_hal_input_stats_t *input_stats, bool reset) {
   if(obj == NULL) {
      XLOGD_ERROR("NULL obj");
      return(false);
   }
   ctrlm_hal_input_obj_t *input_obj = (ctrlm_hal_input_obj_t *)obj;
   if(!ctrlm_xraudio_hal_input_obj_is_valid(input_obj)) {
      XLOGD_ERROR("invalid obj");
      return(false);
   }
   if(input_obj->xraudio_input_stats == NULL) {
      return(false);
   }
   return input_obj->xraudio_input_stats(input_stats, reset);
}

bool xraudio_hal_input_detection(xraudio_hal_input_obj_t obj, uint32_t chan, bool *ignore) {
   if(obj == NULL) {
      XLOGD_ERROR("NULL obj");
      return(false);
   }
   ctrlm_hal_input_obj_t *input_obj = (ctrlm_hal_input_obj_t *)obj;
   if(!ctrlm_xraudio_hal_input_obj_is_valid(input_obj)) {
      XLOGD_ERROR("invalid obj");
      return(false);
   }
   if(input_obj->xraudio_input_detection == NULL) {
      return(false);
   }
   return input_obj->xraudio_input_detection(chan, ignore);
}

bool xraudio_hal_input_eos_cmd(xraudio_hal_input_obj_t obj, xraudio_eos_cmd_t cmd, uint32_t chan) {
   if(obj == NULL) {
      XLOGD_ERROR("NULL obj");
      return(false);
   }
   ctrlm_hal_input_obj_t *input_obj = (ctrlm_hal_input_obj_t *)obj;
   if(!ctrlm_xraudio_hal_input_obj_is_valid(input_obj)) {
      XLOGD_ERROR("invalid obj");
      return(false);
   }
   if(input_obj->xraudio_input_eos_cmd == NULL) {
      return(false);
   }
   return input_obj->xraudio_input_eos_cmd(cmd, chan);
}

xraudio_hal_output_obj_t xraudio_hal_output_open(xraudio_hal_obj_t hal_obj, xraudio_devices_output_t device, xraudio_resource_id_output_t resource, uint8_t user_id, xraudio_output_format_t *format, xraudio_volume_step_t left, xraudio_volume_step_t right) {
   XLOGD_DEBUG("not implemented");
   return NULL;
}

void xraudio_hal_output_close(xraudio_hal_output_obj_t obj, xraudio_devices_output_t device) {
   XLOGD_DEBUG("not implemented");
}

uint32_t xraudio_hal_output_buffer_size_get(xraudio_hal_output_obj_t obj) {
   XLOGD_DEBUG("not implemented");
   return 0;
}

int32_t  xraudio_hal_output_write(xraudio_hal_output_obj_t obj, uint8_t *data, uint32_t size) {
   XLOGD_DEBUG("not implemented");
   return -1;
}

bool xraudio_hal_output_volume_set_int(xraudio_hal_output_obj_t obj, xraudio_devices_output_t device, xraudio_volume_step_t left, xraudio_volume_step_t right) {
   XLOGD_DEBUG("not implemented");
   return false;
}

bool xraudio_hal_output_volume_set_float(xraudio_hal_output_obj_t obj, xraudio_devices_output_t device, float left, float right) {
   XLOGD_DEBUG("not implemented");
   return false;
}

uint32_t xraudio_hal_output_latency_get(xraudio_hal_output_obj_t obj) {
   XLOGD_DEBUG("not implemented");
   return 0;
}

bool ctrlm_xraudio_hal_global_obj_is_valid(ctrlm_hal_global_obj_t *obj) {
   if(obj != NULL && obj->identifier == CTRLM_XRAUDIO_HAL_GLOBAL_IDENTIFIER) {
      return(true);
   }
   return(false);
}

bool ctrlm_xraudio_hal_input_obj_is_valid(ctrlm_hal_input_obj_t *obj) {
   if(obj != NULL && obj->identifier == CTRLM_XRAUDIO_HAL_SESSION_IDENTIFIER) {
      return(true);
   }
   return(false);
}

ctrlm_hal_input_object_t ctrlm_xraudio_hal_input_open(const ctrlm_hal_input_params_t *input_params) {
   XLOGD_DEBUG("Enter...");
   ctrlm_hal_input_object_t object = NULL;
   if (NULL != (object = ctrlm_xraudio_hal_input_session_req(input_params))) {
      if (false == ctrlm_xraudio_hal_input_session_begin(object, input_params)) {
         XLOGD_ERROR("Unable to begin session, closing");
         ((ctrlm_hal_input_obj_t *)object)->ctrlm_close(false);
         object = NULL;
      }
   }
   return object;
}
ctrlm_hal_input_object_t ctrlm_xraudio_hal_input_session_req(const ctrlm_hal_input_params_t *input_params) {
   XLOGD_DEBUG("Enter...");
   if(NULL == g_ctrlm_hal_obj.async_callback) {
      XLOGD_ERROR("xraudio hal is not open");
      return(NULL);
   }
   for(uint32_t index = 0; index < (sizeof(ctrlm_hal_input_objs) / sizeof(ctrlm_hal_input_obj_get_t)); index++) {
      const ctrlm_hal_input_obj_t *obj = ctrlm_hal_input_objs[index]();
      if(obj->device != input_params->device) {
         continue;
      }
      if(obj->ctrlm_open == NULL || obj->ctrlm_close == NULL) {
         continue;
      }
      xraudio_hal_msg_session_request_t session_request;
      ctrlm_hal_input_object_t object = obj->ctrlm_open(&input_params->input_format, input_params->fd);
      if(object == NULL) {
         XLOGD_ERROR("Unable to open input");
         continue;
      }

      session_request.header.type   = XRAUDIO_MSG_TYPE_SESSION_REQUEST;
      session_request.header.source = obj->xraudio_device_type;
      if(!g_ctrlm_hal_obj.async_callback((void *)&session_request)) {
         XLOGD_ERROR("Unable to acquire session");
         obj->ctrlm_close(false);
         return(NULL);
      }
      return(object);
   }
   XLOGD_ERROR("invalid device <%s>", xraudio_devices_input_str(input_params->device));
   return(NULL);
}

bool ctrlm_xraudio_hal_input_session_begin(ctrlm_hal_input_object_t object, const ctrlm_hal_input_params_t *input_params) {
   XLOGD_DEBUG("Enter...");
   if(NULL == g_ctrlm_hal_obj.async_callback) {
      XLOGD_ERROR("xraudio hal is not open");
      return(false);
   }
   ctrlm_hal_input_obj_t *obj  = (ctrlm_hal_input_obj_t *)object;
   if(obj) {
      xraudio_hal_msg_session_begin_t   session_begin;
      session_begin.header.type   = XRAUDIO_MSG_TYPE_SESSION_BEGIN;
      session_begin.header.source = obj->xraudio_device_type;
      session_begin.format        = input_params->input_format;
      if(obj->xraudio_input_stream_params_get != NULL) {
         if(!obj->xraudio_input_stream_params_get(&session_begin.stream_params)) {
            XLOGD_ERROR("failed to get stream params");
            return(false);
         }
      }

      if(!g_ctrlm_hal_obj.async_callback((void *)&session_begin)) {
         XLOGD_ERROR("Unable to begin session");
         return(false);
      }
      return(true);
   }
   return(false);
}

bool ctrlm_xraudio_hal_input_set_ctrlm_data_read_cb(ctrlm_hal_input_object_t object, ctrlm_data_read_cb_t callback, void *user_data) {
   XLOGD_DEBUG("Enter...");   
   ctrlm_hal_input_obj_t *obj = (ctrlm_hal_input_obj_t *)object;
   if(!ctrlm_xraudio_hal_input_obj_is_valid(obj)) {
      XLOGD_ERROR("invalid object");
      return false;
   }

   if(obj->ctrlm_set_data_read_cb == NULL) {
      XLOGD_ERROR("ctrlm_set_data_read_cb() is NULL");
      return false;
   } else {
      obj->ctrlm_set_data_read_cb(callback, user_data);
   }
   return true;
}


bool ctrlm_xraudio_hal_input_use_external_pipe(ctrlm_hal_input_object_t object, int fd) {
   XLOGD_DEBUG("Enter...");
   ctrlm_hal_input_obj_t *obj = (ctrlm_hal_input_obj_t *)object;
   if(!ctrlm_xraudio_hal_input_obj_is_valid(obj)) {
      XLOGD_ERROR("invalid object");
      return false;
   }

   if(obj->ctrlm_update_fd == NULL) {
      XLOGD_ERROR("ctrlm_update_fd() is NULL");
      return false;
   } else {
      obj->ctrlm_update_fd(fd);
   }
   return true;
}

void ctrlm_xraudio_hal_input_close(ctrlm_hal_input_object_t object) {
   XLOGD_DEBUG("Enter...");
   ctrlm_hal_input_obj_t *obj = (ctrlm_hal_input_obj_t *)object;
   if(!ctrlm_xraudio_hal_input_obj_is_valid(obj)) {
      XLOGD_ERROR("invalid object");
      return;
   }
   if(obj->ctrlm_close != NULL) {
      obj->ctrlm_close(true);
   }
}

xraudio_hal_msg_callback_t xraudio_hal_async_callback_get() {
   return(g_ctrlm_hal_obj.async_callback);
}
