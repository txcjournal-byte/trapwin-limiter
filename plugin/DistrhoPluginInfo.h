#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

#define DISTRHO_PLUGIN_BRAND    "TrapWin"
#define DISTRHO_PLUGIN_NAME     "TrapWin Limiter"
#define DISTRHO_PLUGIN_URI      "https://trapvst.local/plugins/traplimit"
#define DISTRHO_PLUGIN_CLAP_ID  "com.trapvst.traplimit"

#define DISTRHO_PLUGIN_BRAND_ID   TrVs
#define DISTRHO_PLUGIN_UNIQUE_ID  TrLm

#define DISTRHO_PLUGIN_HAS_UI           1
#define DISTRHO_PLUGIN_IS_RT_SAFE       1
#define DISTRHO_PLUGIN_NUM_INPUTS       2
#define DISTRHO_PLUGIN_NUM_OUTPUTS      2
#define DISTRHO_PLUGIN_WANT_LATENCY     1
#define DISTRHO_PLUGIN_WANT_PROGRAMS    1
#define DISTRHO_PLUGIN_WANT_DIRECT_ACCESS 1
#define DISTRHO_PLUGIN_VST3_CATEGORIES  "Fx|TrapWin"
#define DISTRHO_PLUGIN_CLAP_FEATURES    "audio-effect", "limiter", "mastering", "stereo"

#define DISTRHO_UI_USE_NANOVG       1
#define DISTRHO_UI_USER_RESIZABLE   1
#define DISTRHO_UI_DEFAULT_WIDTH    1108
#define DISTRHO_UI_DEFAULT_HEIGHT   199

#endif
