/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2015 - Daniel De Matteis
 *  Copyright (C) 2012-2015 - Michael Lelli
 * 
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _PLATFORM_ANDROID_H
#define _PLATFORM_ANDROID_H

#include <jni.h>
#include <poll.h>
#include <rthreads/rthreads.h>
#include <sched.h>

#include <android/configuration.h>
#include <android/looper.h>
#include <android/native_activity.h>
#include <android/window.h>
#include <android/sensor.h>


#ifndef MAX_AXIS
#define MAX_AXIS 10
#endif

#ifndef MAX_PADS
#define MAX_PADS 8
#endif

#ifndef MAX_TOUCH
#define MAX_TOUCH 16
#endif

#ifndef AKEYCODE_ASSIST
#define AKEYCODE_ASSIST 219
#endif

#define LAST_KEYCODE AKEYCODE_ASSIST

typedef struct state_device
{
   int id;
   int port;
   char name[256];
} state_device_t;

struct input_pointer
{
   int16_t x, y;
   int16_t full_x, full_y;
};

typedef struct
{
   float x;
   float y;
   float z;
} sensor_t;

typedef struct android_input_state
{
   int16_t analog_state[MAX_PADS][MAX_AXIS];
   int8_t hat_state[MAX_PADS][2];
   uint8_t pad_state[MAX_PADS][(LAST_KEYCODE + 7) / 8];
   unsigned pads_connected;
   state_device_t pad_states[MAX_PADS];
   struct input_pointer pointer[MAX_TOUCH];
   sensor_t accelerometer_state;
   unsigned pointer_count;
} android_input_state_t;

struct android_poll_source
{
   /* The identifier of this source. May be LOOPER_ID_MAIN or
    * LOOPER_ID_INPUT. */
   int32_t id;

   /* The android_app this ident is associated with. */
   struct android_app *app;

   /* Function to call to perform the standard processing of data from
    * this source. */
   void (*process)(struct android_app *app, struct android_poll_source *source);
};

/**
 * This is the interface for the standard glue code of a threaded
 * application.  In this model, the application's code is running
 * in its own thread separate from the main thread of the process.
 * It is not required that this thread be associated with the Java
 * VM, although it will need to be in order to make JNI calls any
 * Java objects.
 */
struct android_app
{
   /* The application can place a pointer to its own state object
    * here if it likes. */
   void *userData;

   /* Fill this in with the function to process main app commands (APP_CMD_*) */
   void (*onAppCmd)(struct android_app *app, int32_t cmd);

   /* Fill this in with the function to process input events. At this point
    * the event has already been pre-dispatched, and it will be finished upon
    * return. Return 1 if you have handled the event, 0 for any default
    * dispatching. */
   int32_t (*onInputEvent)(struct android_app *app, AInputEvent *event);


   /* The ANativeActivity object instance that this app is running in. */
   ANativeActivity* activity;

   /* The current configuration the app is running in. */
   AConfiguration *config;

    /* This is the last instance's saved state, as provided at creation time.
     * It is NULL if there was no state.  You can use this as you need; the
     * memory will remain around until you call android_app_exec_cmd() for
     * APP_CMD_RESUME, at which point it will be freed and savedState set to NULL.
     * These variables should only be changed when processing a APP_CMD_SAVE_STATE,
     * at which point they will be initialized to NULL and you can malloc your
     * state and place the information here.  In that case the memory will be
     * freed for you later. */
    void* savedState;
    size_t savedStateSize;

   /* The ALooper associated with the app's thread. */
   ALooper* looper;

   /* When non-NULL, this is the input queue from which the app will
    * receive user input events. */
   AInputQueue* inputQueue;

   /* When non-NULL, this is the window surface that the app can draw in. */
   ANativeWindow* window;

   /* Current content rectangle of the window; this is the area where the
    * window's content should be placed to be seen by the user. */
   ARect contentRect;

   /* Current state of the app's activity. May be either APP_CMD_START,
    * APP_CMD_RESUME, APP_CMD_PAUSE, or APP_CMD_STOP; see below. */
   int activityState;

   /* This is non-zero when the application's NativeActivity is being
    * destroyed and waiting for the app thread to complete. */
   int destroyRequested;

   /* Below are "private" implementation variables of the glue code. */

   slock_t *mutex;
   scond_t *cond;

   int msgread;
   int msgwrite;

   sthread_t *thread;

   struct android_poll_source cmdPollSource;
   struct android_poll_source inputPollSource;

   int running;
   int stateSaved;
   int destroyed;
   int redrawNeeded;
   AInputQueue* pendingInputQueue;
   ANativeWindow* pendingWindow;
   ARect pendingContentRect;

   unsigned accelerometer_event_rate;
   const ASensor* accelerometerSensor;
   uint64_t sensor_state_mask;
   char current_ime[PATH_MAX_LENGTH];
   jmethodID getIntent;
   jmethodID onRetroArchExit;
   jmethodID getStringExtra;
   jmethodID clearPendingIntent;
   jmethodID hasPendingIntent;
   jmethodID getPendingIntentConfigPath;
   jmethodID getPendingIntentLibretroPath;
   jmethodID getPendingIntentFullPath;
   jmethodID getPendingIntentIME;
   android_input_state_t thread_state;

   ASensorManager *sensorManager;
   ASensorEventQueue *sensorEventQueue;
};

enum
{
   /**
    * Looper data ID of commands coming from the app's main thread, which
    * is returned as an identifier from ALooper_pollOnce().  The data for this
    * identifier is a pointer to an android_poll_source structure.
    * These can be retrieved and processed with android_app_read_cmd()
    * and android_app_exec_cmd().
    */
   LOOPER_ID_MAIN   = 1,

   /**
    * Looper data ID of events coming from the AInputQueue of the
    * application's window, which is returned as an identifier from
    * ALooper_pollOnce().  The data for this identifier is a pointer to an
    * android_poll_source structure.  These can be read via the inputQueue
    * object of android_app.
    */
   LOOPER_ID_INPUT  = 2,

   /**
    * Start of user-defined ALooper identifiers.
    */
   LOOPER_ID_USER   = 3,
};

enum
{
   /**
    * Command from main thread: the AInputQueue has changed.  Upon processing
    * this command, android_app->inputQueue will be updated to the new queue
    * (or NULL).
    */
   APP_CMD_INPUT_CHANGED,

   /**
    * Command from main thread: a new ANativeWindow is ready for use.  Upon
    * receiving this command, android_app->window will contain the new window
    * surface.
    */
   APP_CMD_INIT_WINDOW,

   /**
    * Command from main thread: the existing ANativeWindow needs to be
    * terminated.  Upon receiving this command, android_app->window still
    * contains the existing window; after calling android_app_exec_cmd
    * it will be set to NULL.
    */
   APP_CMD_TERM_WINDOW,

   /**
    * Command from main thread: the current ANativeWindow has been resized.
    * Please redraw with its new size.
    */
   APP_CMD_WINDOW_RESIZED,

   /**
    * Command from main thread: the system needs that the current ANativeWindow
    * be redrawn.  You should redraw the window before handing this to
    * android_app_exec_cmd() in order to avoid transient drawing glitches.
    */
   APP_CMD_WINDOW_REDRAW_NEEDED,

   /**
    * Command from main thread: the content area of the window has changed,
    * such as from the soft input window being shown or hidden.  You can
    * find the new content rect in android_app::contentRect.
    */
   APP_CMD_CONTENT_RECT_CHANGED,

   /**
    * Command from main thread: the app's activity window has gained
    * input focus.
    */
   APP_CMD_GAINED_FOCUS,

   /**
    * Command from main thread: the app's activity window has lost
    * input focus.
    */
   APP_CMD_LOST_FOCUS,

   /**
    * Command from main thread: the current device configuration has changed.
    */
   APP_CMD_CONFIG_CHANGED,

   /**
    * Command from main thread: the system is running low on memory.
    * Try to reduce your memory use.
    */
   APP_CMD_LOW_MEMORY,

   /**
    * Command from main thread: the app's activity has been started.
    */
   APP_CMD_START,

   /**
    * Command from main thread: the app's activity has been resumed.
    */
   APP_CMD_RESUME,

   /**
    * Command from main thread: the app should generate a new saved state
    * for itself, to restore from later if needed.  
    */
   APP_CMD_SAVE_STATE,

   /**
    * Command from main thread: the app's activity has been paused.
    */
   APP_CMD_PAUSE,

   /**
    * Command from main thread: the app's activity has been stopped.
    */
   APP_CMD_STOP,

   /**
    * Command from main thread: the app's activity is being destroyed,
    * and waiting for the app thread to clean up and exit before proceeding.
    */
   APP_CMD_DESTROY,

   // Set by thread when it will no longer reply to commands.
   APP_CMD_DEAD,
};

#define JNI_EXCEPTION(env) \
   if ((*env)->ExceptionOccurred(env)) \
   { \
      (*env)->ExceptionDescribe(env); \
      (*env)->ExceptionClear(env); \
   }

#define FIND_CLASS(env, var, classname) \
   var = (*env)->FindClass(env, classname); \
   JNI_EXCEPTION(env)

#define GET_OBJECT_CLASS(env, var, clazz_obj) \
   var = (*env)->GetObjectClass(env, clazz_obj); \
   JNI_EXCEPTION(env)

#define GET_FIELD_ID(env, var, clazz, fieldName, fieldDescriptor) \
   var = (*env)->GetFieldID(env, clazz, fieldName, fieldDescriptor); \
   JNI_EXCEPTION(env)

#define GET_METHOD_ID(env, var, clazz, methodName, fieldDescriptor) \
   var = (*env)->GetMethodID(env, clazz, methodName, fieldDescriptor); \
   JNI_EXCEPTION(env)

#define GET_STATIC_METHOD_ID(env, var, clazz, methodName, fieldDescriptor) \
   var = (*env)->GetStaticMethodID(env, clazz, methodName, fieldDescriptor); \
   JNI_EXCEPTION(env)

#define CALL_OBJ_METHOD(env, var, clazz_obj, methodId) \
   var = (*env)->CallObjectMethod(env, clazz_obj, methodId); \
   JNI_EXCEPTION(env)

#define CALL_OBJ_STATIC_METHOD(env, var, clazz, methodId) \
   var = (*env)->CallStaticObjectMethod(env, clazz, methodId); \
   JNI_EXCEPTION(env)

#define CALL_OBJ_STATIC_METHOD_PARAM(env, var, clazz, methodId, ...) \
   var = (*env)->CallStaticObjectMethod(env, clazz, methodId, __VA_ARGS__); \
   JNI_EXCEPTION(env)

#define CALL_OBJ_METHOD_PARAM(env, var, clazz_obj, methodId, ...) \
   var = (*env)->CallObjectMethod(env, clazz_obj, methodId, __VA_ARGS__); \
   JNI_EXCEPTION(env)

#define CALL_VOID_METHOD(env, clazz_obj, methodId) \
   (*env)->CallVoidMethod(env, clazz_obj, methodId); \
   JNI_EXCEPTION(env)

#define CALL_VOID_METHOD_PARAM(env, clazz_obj, methodId, ...) \
   (*env)->CallVoidMethod(env, clazz_obj, methodId, __VA_ARGS__); \
   JNI_EXCEPTION(env)

#define CALL_BOOLEAN_METHOD(env, var, clazz_obj, methodId) \
   var = (*env)->CallBooleanMethod(env, clazz_obj, methodId); \
   JNI_EXCEPTION(env)

#define CALL_DOUBLE_METHOD(env, var, clazz_obj, methodId) \
   var = (*env)->CallDoubleMethod(env, clazz_obj, methodId); \
   JNI_EXCEPTION(env)

#define CALL_INT_METHOD(env, var, clazz_obj, methodId) \
   var = (*env)->CallIntMethod(env, clazz_obj, methodId); \
   JNI_EXCEPTION(env)

bool (*engine_lookup_name)(char *buf,
      int *vendorId, int *productId, size_t size, int id);

JNIEnv *jni_thread_getenv(void);

void android_app_write_cmd(struct android_app *android_app, int8_t cmd);

extern struct android_app *g_android;

#endif /* _PLATFORM_ANDROID_H */
