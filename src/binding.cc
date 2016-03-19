#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include <node.h>
#include <node_buffer.h>
#include <nan.h>
#include <string.h>
#include <stdlib.h>

#include <picoapi.h>

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

// This should be enough for 1 voice
#define PICO_MEM_SIZE 2500000

const pico_Char* PICO_VOICE_NAME =
  reinterpret_cast<const pico_Char*>("PicoVoice");
char ERR_MSG_TA_LOAD[] = "ta resource load failed";
char ERR_MSG_SG_LOAD[] = "sg resource load failed";
char ERR_MSG_CREATE_VOICE[] = "createVoiceDefinition() failed";
char ERR_MSG_TA_ADD[] = "ta resource add failed";
char ERR_MSG_SG_ADD[] = "sg resource add failed";
char ERR_MSG_TA_NAME[] = "ta getResourceName() failed";
char ERR_MSG_SG_NAME[] = "sg getResourceName() failed";
char ERR_MSG_NEW_ENGINE[] = "newEngine() failed";
char ERR_MSG_GET_DATA[] = "getData() failed";
char ERR_MSG_UNKNOWN[] = "Unknown error";
char ERR_MSG_MALLOC[] = "malloc() failed";

#define PICO_MALLOC_FAIL -9000
#define PICO_TEXT_REQUIRED -9001

using namespace node;
using namespace v8;

class Speaky : public ObjectWrap {
  public:
    void* tts_mem;
    pico_System tts_system;
    pico_Engine tts_engine;
    pico_Resource tts_ta_resource;
    pico_Resource tts_sg_resource;
    Nan::Persistent<String> tts_text;
    size_t tts_text_offset;
    size_t tts_text_len;

    Speaky() :
      tts_mem(nullptr),
      tts_system(nullptr),
      tts_engine(nullptr),
      tts_ta_resource(nullptr),
      tts_sg_resource(nullptr),
      tts_text_offset(0),
      tts_text_len(0) {
    }

    ~Speaky() {
      clearResources();
      if (tts_system) {
        pico_terminate(&tts_system);
        tts_system = nullptr;
      }
      free(tts_mem);
      tts_mem = nullptr;
    }

    void clearResources() {
      if (tts_engine) {
        if (tts_system)
          pico_disposeEngine(tts_system, &tts_engine);
        tts_engine = nullptr;
      }
      if (tts_system)
        pico_releaseVoiceDefinition(tts_system, PICO_VOICE_NAME);
      if (tts_ta_resource) {
        if (tts_system)
          pico_unloadResource(tts_system, &tts_ta_resource);
        tts_ta_resource = nullptr;
      }
      if (tts_sg_resource) {
        if (tts_system)
          pico_unloadResource(tts_system, &tts_sg_resource);
        tts_sg_resource = nullptr;
      }
    }

    pico_Status init(char** errorMsg) {
      tts_mem = malloc(PICO_MEM_SIZE);
      if (!tts_mem) {
        *errorMsg = ERR_MSG_MALLOC;
        return PICO_MALLOC_FAIL;
      }
      pico_Status r = pico_initialize(tts_mem, PICO_MEM_SIZE, &tts_system);
      if (r == PICO_OK) {
        *errorMsg = nullptr;
      } else {
        if (pico_getSystemStatusMessage(tts_system, r, *errorMsg) != PICO_OK)
          *errorMsg = ERR_MSG_UNKNOWN;
      }
      return r;
    }

    pico_Status setVoice(const pico_Char* tts_ta_path,
                         const pico_Char* tts_sg_path,
                         char** errorMsg) {
      clearResources();

      pico_Status r;
      pico_Char ta_resource_name[PICO_MAX_RESOURCE_NAME_SIZE];
      pico_Char sg_resource_name[PICO_MAX_RESOURCE_NAME_SIZE];

      r = pico_loadResource(tts_system, tts_ta_path, &tts_ta_resource);
      if (r != PICO_OK) {
        if (pico_getSystemStatusMessage(tts_system, r, *errorMsg) != PICO_OK)
          *errorMsg = ERR_MSG_TA_LOAD;
        return r;
      }

      r = pico_loadResource(tts_system, tts_sg_path, &tts_sg_resource);
      if (r != PICO_OK) {
        if (pico_getSystemStatusMessage(tts_system, r, *errorMsg) != PICO_OK)
          *errorMsg = ERR_MSG_SG_LOAD;
        return r;
      }

      r = pico_createVoiceDefinition(tts_system, PICO_VOICE_NAME);
      if (r != PICO_OK) {
        if (pico_getSystemStatusMessage(tts_system, r, *errorMsg) != PICO_OK)
          *errorMsg = ERR_MSG_CREATE_VOICE;
        return r;
      }

      r = pico_getResourceName(tts_system,
                               tts_ta_resource,
                               reinterpret_cast<char*>(ta_resource_name));
      if (r != PICO_OK) {
        if (pico_getSystemStatusMessage(tts_system, r, *errorMsg) != PICO_OK)
          *errorMsg = ERR_MSG_TA_NAME;
        return r;
      }

      r = pico_addResourceToVoiceDefinition(tts_system,
                                            PICO_VOICE_NAME,
                                            ta_resource_name);
      if (r != PICO_OK) {
        if (pico_getSystemStatusMessage(tts_system, r, *errorMsg) != PICO_OK)
          *errorMsg = ERR_MSG_TA_ADD;
        return r;
      }

      r = pico_getResourceName(tts_system,
                               tts_sg_resource,
                               reinterpret_cast<char*>(sg_resource_name));
      if (r != PICO_OK) {
        if (pico_getSystemStatusMessage(tts_system, r, *errorMsg) != PICO_OK)
          *errorMsg = ERR_MSG_SG_NAME;
        return r;
      }

      r = pico_addResourceToVoiceDefinition(tts_system,
                                            PICO_VOICE_NAME,
                                            sg_resource_name);
      if (r != PICO_OK) {
        if (pico_getSystemStatusMessage(tts_system, r, *errorMsg) != PICO_OK)
          *errorMsg = ERR_MSG_SG_ADD;
        return r;
      }

      r = pico_newEngine(tts_system, PICO_VOICE_NAME, &tts_engine);
      if (r != PICO_OK) {
        if (pico_getSystemStatusMessage(tts_system, r, *errorMsg) != PICO_OK)
          *errorMsg = ERR_MSG_NEW_ENGINE;
        return r;
      }
      
      *errorMsg = nullptr;
      return r;
    }

    pico_Status speak(char* text, char** errorMsg) {
      pico_Status r;
      pico_Int16 text_size;
      pico_Int16 consumed;

      if (text == nullptr) {
        if (tts_text.IsEmpty())
          return PICO_TEXT_REQUIRED;
        Local<String> tts_orig_text = Nan::New(tts_text);
        Nan::Utf8String temp(tts_orig_text);
        text_size = MIN(tts_text_len - tts_text_offset, PICO_INT16_MAX);
        r = pico_putTextUtf8(tts_engine,
                             reinterpret_cast<pico_Char*>(*temp),
                             text_size,
                             &consumed);
        tts_text_offset += consumed;
        if (tts_text_offset == tts_text_len) {
          // We finished processing the text
          tts_text.Reset();
        }
      } else {
        if (!tts_text.IsEmpty()) {
          tts_text.Reset();
          // Reset engine first
          r = pico_resetEngine(tts_engine, PICO_RESET_SOFT);
          if (r != PICO_OK)
            return r;
        }
        size_t text_len = strlen(text) + 1;
        text_size = MIN(text_len, PICO_INT16_MAX);
        r = pico_putTextUtf8(tts_engine,
                             reinterpret_cast<pico_Char*>(text),
                             text_size,
                             &consumed);
        if (r == PICO_OK) {
          if (consumed != text_size || text_len != text_size) {
            // We need to store state for future invocations since we did not
            // process the entirety of the input text (or input text chunk)
            tts_text.Reset(Nan::New<String>(text).ToLocalChecked());
            tts_text_offset = consumed;
            tts_text_len = text_len;
          }
        }
      }

      if (r == PICO_OK)
        *errorMsg = nullptr;
      else if (pico_getEngineStatusMessage(tts_engine, r, *errorMsg) != PICO_OK)
        *errorMsg = ERR_MSG_UNKNOWN;

      return r;
    }

    pico_Status pullAudio(char* audio_buffer,
                          int audio_buffer_size,
                          int16_t* out_total_bytes,
                          char** errorMsg) {
      pico_Int16 out_data_type;
      pico_Status r;
      int16_t nb = *out_total_bytes = 0;

      do {
        r = pico_getData(tts_engine,
                         audio_buffer,
                         audio_buffer_size,
                         &nb,
                         &out_data_type);
        if (nb > 0) {
          assert(out_data_type == PICO_DATA_PCM_16BIT);
          audio_buffer += nb;
          audio_buffer_size -= nb;
          *out_total_bytes += nb;
          if (audio_buffer_size == 0)
            return r;
          continue;
        }
      } while (r == PICO_STEP_BUSY);
      /*if (r == PICO_STEP_ERROR) {
        if (pico_getEngineStatusMessage(tts_engine, r, *errorMsg) != PICO_OK)
          *errorMsg = ERR_MSG_GET_DATA;
      } else {
        *errorMsg = nullptr;
      }*/
      return r;
    }

    static void New(const Nan::FunctionCallbackInfo<v8::Value>& args) {
      if (!args.IsConstructCall()) {
        return Nan::ThrowTypeError(
          "Use `new` to create instances of this object."
        );
      }

      Speaky* obj = new Speaky();
      char* err = nullptr;
      obj->init(&err);
      if (err) {
        delete obj;
        return Nan::ThrowError(err);
      }
      obj->Wrap(args.This());

      return args.GetReturnValue().Set(args.This());
    }

    static void SetVoice(const Nan::FunctionCallbackInfo<v8::Value>& args) {
      Speaky* obj = ObjectWrap::Unwrap<Speaky>(args.This());

      pico_Status r;
      char* err = nullptr;

      if (args.Length() < 2)
        return Nan::ThrowError("Missing path arguments");
      if (!args[0]->IsString())
        return Nan::ThrowTypeError("taPath argument must be a string");
      if (!args[1]->IsString())
        return Nan::ThrowTypeError("sgPath argument must be a string");

      Nan::Utf8String ta_path(args[0]);
      Nan::Utf8String sg_path(args[1]);
      r = obj->setVoice(reinterpret_cast<const pico_Char*>(*ta_path),
                        reinterpret_cast<const pico_Char*>(*sg_path),
                        &err);

      if (err)
        return Nan::ThrowError(err);

      args.GetReturnValue().Set(Nan::New<Int32>(r));
    }

    static void Speak(const Nan::FunctionCallbackInfo<v8::Value>& args) {
      Speaky* obj = ObjectWrap::Unwrap<Speaky>(args.This());

      pico_Status r;
      char* err = nullptr;

      if (args.Length() > 0 && args[0]->IsString()) {
        Nan::Utf8String temp(args[0]);
        r = obj->speak(*temp, &err);
      } else {
        r = obj->speak(nullptr, &err);
      }

      if (err)
        return Nan::ThrowError(err);

      if (r == PICO_OK && obj->tts_text.IsEmpty())
        args.GetReturnValue().Set(Nan::New<Boolean>(true));
      else 
        args.GetReturnValue().Set(Nan::New<Int32>(r));
    }

    static void GetAudio(const Nan::FunctionCallbackInfo<v8::Value>& args) {
      Speaky* obj = ObjectWrap::Unwrap<Speaky>(args.This());

      pico_Status r;
      char* err = nullptr;

      if (args.Length() < 1)
        return Nan::ThrowTypeError("Missing audio buffer argument");
      if (!Buffer::HasInstance(args[0]))
        return Nan::ThrowTypeError("audio buffer argument must be a Buffer");

      Local<Value> buffer_obj = args[0];
      char* buffer = Buffer::Data(buffer_obj);
      size_t buffer_len = Buffer::Length(buffer_obj);
      int16_t written = 0;
      r = obj->pullAudio(buffer, MIN(buffer_len, INT32_MAX), &written, &err);

      if (err)
        return Nan::ThrowError(err);

      if (r == PICO_STEP_IDLE) {
        // XXX: hack to be able to return both "finished" status and number of
        // bytes written
        written *= -1;
      }

      args.GetReturnValue().Set(Nan::New<Number>(written));
    }

    static void Initialize(Handle<Object> target) {
      Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);

      tpl->InstanceTemplate()->SetInternalFieldCount(1);
      tpl->SetClassName(Nan::New<String>("Speaky").ToLocalChecked());
      Nan::SetPrototypeMethod(tpl, "setVoice", SetVoice);
      Nan::SetPrototypeMethod(tpl, "speak", Speak);
      Nan::SetPrototypeMethod(tpl, "getAudio", GetAudio);

      target->Set(Nan::New<String>("Speaky").ToLocalChecked(),
                  tpl->GetFunction());
    }
};

extern "C" {
  void init(Handle<Object> target) {
    Nan::HandleScope();
    Speaky::Initialize(target);
  }

  NODE_MODULE(speaky, init);
}
