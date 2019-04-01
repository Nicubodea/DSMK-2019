#ifndef _TRACE_H_
#define _TRACE_H_

#define WPP_CONTROL_GUIDS \
WPP_DEFINE_CONTROL_GUID( \
AppSpecificFlags, (bb6301c6, 4e62, 4bbf, b869, 33b533287483), \
WPP_DEFINE_BIT(ComponentDriver1)                            \
)
//
// Because we are using our custom logging function DrvLogMessage(Flags, Level, Message, ...) we need to define the flowing macros
#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) WPP_LEVEL_LOGGER(flags)
#define WPP_LEVEL_FLAGS_ENABLED(lvl,flags) (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

//
// Here we define our logging functions
//
// begin_wpp config
//
// Functions for logging driver related events
//
// FUNC Drv1LogTrace{LEVEL=TRACE_LEVEL_VERBOSE, FLAGS=ComponentDriver1}(MSG, ...);
// FUNC Drv1LogInfo{LEVEL=TRACE_LEVEL_INFORMATION, FLAGS=ComponentDriver1}(MSG, ...);
// FUNC Drv1LogWarning{LEVEL=TRACE_LEVEL_WARNING, FLAGS=ComponentDriver1}(MSG, ...);
// FUNC Drv1LogError{LEVEL=TRACE_LEVEL_ERROR, FLAGS=ComponentDriver1}(MSG, ...);
// FUNC Drv1LogCritical{LEVEL=TRACE_LEVEL_CRITICAL, FLAGS=ComponentDriver1}(MSG, ...);
//
// end_wpp
//
#endif//_TRACE_H