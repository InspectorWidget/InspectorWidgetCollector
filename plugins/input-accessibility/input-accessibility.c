#include <obs-module.h>

#include "platform.h"

#ifdef __APPLE__
#include "AXRecord.h"
#endif

#include <stdio.h>

#define UIO_LOG(level, format, ...) \
    blog(level, "[accessibility]: " format, ##__VA_ARGS__)
#define UIO_LOG_S(source, level, format, ...) \
    blog(level, "[accessibility '%s']: " format, \
    obs_source_get_name(source), ##__VA_ARGS__)
#define UIO_BLOG(level, format, ...) \
    UIO_LOG(level, format, ##__VA_ARGS__)
//UIO_LOG_S(s->source, level, format, ##__VA_ARGS__)

FILE * event_file = 0;

const char* start_logging_accessibility(char* event_file_name){
    if (!event_file_name || (event_file_name && !event_file_name[0])) {
        return "event file name empty";
    }
    /*event_file = fopen (event_file_name, "w+");
    if (event_file == NULL){
        return "event file not found";
    }*/

#ifdef __APPLE__
    int status = start_ax(event_file_name);
    return (status == 0 ? "" : "Could not start accessibility recording");
#endif

    //setbuf(event_file, NULL);
    return "Could not start accessibility recording";
}

int stop_logging_accessibility(void){
    // Close the file
    //fflush(event_file);
    //fclose(event_file);
#ifdef __APPLE__
    int status = stop_ax();
    return (status == 0 ? "" : "Could not stop accessibility recording");
#endif

    return "Could not stop accessibility recording";
}

struct input_accessibility {
	obs_source_t *source;
	char         *folder;
	char         *file;
	bool         recording;
	bool         persistent;
};

/// GenerateTimeDateFilename from obs/obs-app.cpp
char* GenerateEventsFilename(const char* path)
{
    time_t    now = time(0);
    struct tm *cur_time;
    int bufSize = 1024;
    char *file = (char*)malloc(bufSize);

#ifdef _WIN32
    char slash = '\\';
#else
    char slash = '/';
#endif

    cur_time = localtime(&now);
    snprintf(file,
             bufSize,
             "%s%c%d-%02d-%02d-%02d-%02d-%02d%c%d-%02d-%02d-%02d-%02d-%02d.xml",
             path,
             slash,
             cur_time->tm_year+1900,
             cur_time->tm_mon+1,
             cur_time->tm_mday,
             cur_time->tm_hour,
             cur_time->tm_min,
             cur_time->tm_sec,
             slash,
             cur_time->tm_year+1900,
             cur_time->tm_mon+1,
             cur_time->tm_mday,
             cur_time->tm_hour,
             cur_time->tm_min,
             cur_time->tm_sec);
    printf("file '%s'\n",file);
    return file;
}

static const char *input_accessibility_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
    return obs_module_text("Accessibility");
}

static void input_accessibility_unload(struct input_accessibility *context)
{
	UNUSED_PARAMETER(context);
}

static void input_accessibility_update(void *data, obs_data_t *settings)
{
	UNUSED_PARAMETER(data);
	const char *folder = obs_data_get_string(settings, "folder");
    printf("input_accessibility_update folder %s\n",folder);
}

static void input_accessibility_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, "recording", false);
	const char* default_path = GetDefaultEventsSavePath();
	obs_data_set_default_string(settings, "folder", default_path);
	printf("path %s\n",default_path);
}

static void input_accessibility_show(void *data)
{
	UNUSED_PARAMETER(data);
}

static void input_accessibility_hide(void *data)
{
	UNUSED_PARAMETER(data);
}

static void *input_accessibility_create(obs_data_t *settings, obs_source_t *source)
{
    struct input_accessibility *context = bzalloc(sizeof(struct input_accessibility));
	context->source = source;

    stop_logging_accessibility();

    input_accessibility_update(context, settings);
	return context;
}

static void input_accessibility_destroy(void *data)
{
    struct input_accessibility *context = data;
    input_accessibility_unload(context);
	bfree(context);
}

static obs_properties_t *input_accessibility_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();

	const char* default_path = GetDefaultEventsSavePath();

	printf("path %s\n",default_path);

	obs_properties_add_path(props,
	                        "folder", obs_module_text("SaveFolder"),
	                        OBS_PATH_DIRECTORY, NULL, default_path);

	return props;
}

static const char * input_accessibility_start(void *data, obs_data_t *settings)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(settings);
	const char *folder = obs_data_get_string(settings, "folder");
	char* filepath = GenerateEventsFilename(folder);
    char* status = start_logging_accessibility(filepath);
    free(filepath);
    return status;
}

static void input_accessibility_stop(void *data, obs_data_t *settings)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(settings);
    stop_logging_accessibility();
}

static struct obs_source_info input_accessibility_info = {
    .id             = "input_accessibility",
	.type           = OBS_SOURCE_TYPE_INPUT,
	.output_flags   = OBS_SOURCE_INTERACTION,
    .get_name       = input_accessibility_get_name,
    .create         = input_accessibility_create,
    .destroy        = input_accessibility_destroy,
    .update         = input_accessibility_update,
    .start          = input_accessibility_start,
    .stop           = input_accessibility_stop,
    .get_defaults   = input_accessibility_defaults,
    .show           = input_accessibility_show,
    .hide           = input_accessibility_hide,
    .get_properties = input_accessibility_properties
};

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("input-accessibility", "en-US")

OBS_MODULE_AUTHOR("Christian Frisson")

bool obs_module_load(void)
{
    obs_register_source(&input_accessibility_info);
	return true;
}
