// Copyright 2019-2022 Varjo Technologies Oy. All rights reserved.

#ifndef VARJO_EVENTS_H
#define VARJO_EVENTS_H

#include "Varjo_types.h"
#include "Varjo_types_mr.h"
#include "Varjo_types_datastream.h"

#if defined __cplusplus
extern "C" {
#endif

typedef int64_t varjo_EventType;
static const varjo_EventType varjo_EventType_Visibility = 0x1;      //!< Visibility event received when application being shown / hidden.
static const varjo_EventType varjo_EventType_Button = 0x2;          //!< Button event that is received when a button is pressed or released.
static const varjo_EventType varjo_EventType_TrackingStatus = 0x3;  //!< Event that is received when there's a change in positional tracking state.
static const varjo_EventType varjo_EventType_HeadsetStatus = 0x4;   //!< Event for headset connection and disconnection.
static const varjo_EventType varjo_EventType_DisplayStatus = 0x5;   //!< Event for changes in headset display connections. Eg. if cables are unplugged.
VARJORUNTIME_DEPRECATED static const varjo_EventType varjo_EventType_HeadsetStandbyStatus = 0x6;  //!< Event for changes in standby status. @deprecated, varjo_EventType_StandbyStatus should be used.
static const varjo_EventType varjo_EventType_StandbyStatus = 0x6;            //!< Event for changes in standby status.
static const varjo_EventType varjo_EventType_Foreground = 0x7;               //!< Event for when application goes to background or foreground.
static const varjo_EventType varjo_EventType_MRDeviceStatus = 0x8;           //!< Event for determining whether mixed reality capable device is connected.
static const varjo_EventType varjo_EventType_MRCameraPropertyChange = 0x9;   //!< Event sent right after a camera property (e.g. exposure mode) change.
static const varjo_EventType varjo_EventType_DataStreamStart = 0xA;          //!< Event emitted when a data stream has started.
static const varjo_EventType varjo_EventType_DataStreamStop = 0xB;           //!< Event emitted when a data stream has stopped.
static const varjo_EventType varjo_EventType_TextureSizeChange = 0xC;        //!< Event emitted when a texture size has changed.
static const varjo_EventType varjo_EventType_MRChromaKeyConfigChange = 0xD;  //!< Event emitted when chroma key configuration has changed.

#define VARJO_LAST_EVENT varjo_EventType_MRChromaKeyConfigChange

typedef int8_t varjo_ButtonId;
static const varjo_ButtonId varjo_ButtonId_Application = 0;  //!< Button ID for application button on the headset.

typedef int64_t varjo_TrackingStatus;
static const varjo_TrackingStatus varjo_TrackingStatus_Ok = 0x0;            //!< Tracking is ok.
static const varjo_TrackingStatus varjo_TrackingStatus_Lost = 0x1;          //!< No tracking data available.
static const varjo_TrackingStatus varjo_TrackingStatus_Disconnected = 0x2;  //!< Tracking disconnected.

typedef int64_t varjo_HeadsetStatus;
static const varjo_HeadsetStatus varjo_HeadsetStatus_Connected = 0x1;     //!< Headset is connected.
static const varjo_HeadsetStatus varjo_HeadsetStatus_Disconnected = 0x2;  //!< Headset is disconnected.

typedef int64_t varjo_DisplayStatus;
static const varjo_DisplayStatus varjo_DisplayStatus_ContextNotFound = 0x1;   //!< Context display is not found.
static const varjo_DisplayStatus varjo_DisplayStatus_FocusNotFound = 0x2;     //!< Focus display is not found.
static const varjo_DisplayStatus varjo_DisplayStatus_AllDisplaysFound = 0x4;  //!< All displays found.

typedef int64_t varjo_MRDeviceStatus;
static const varjo_MRDeviceStatus varjo_MRDeviceStatus_Connected = 0x1;     //!< Mixed reality capable device is connected.
static const varjo_MRDeviceStatus varjo_MRDeviceStatus_Disconnected = 0x2;  //!< Mixed reality capable device is disconnected.

struct varjo_EventHeader {
    varjo_EventType type;  //!< Type of the event.
    int64_t timestamp;     //!< Timestamp of the time when the event was issued.
};

struct varjo_EventVisibility {
    varjo_Bool visible;  //!< Current visibility.
};

struct varjo_EventButton {
    varjo_Bool pressed;       //!< Is the button pressed down.
    varjo_ButtonId buttonId;  //!< Id of the button.
};

struct varjo_EventTrackingStatus {
    varjo_TrackingStatus status;  //!< Current tracking status.
};

struct varjo_EventHeadsetStatus {
    varjo_HeadsetStatus status;  //!< Current headset status.
};

struct varjo_EventForeground {
    varjo_Bool isForeground;  //!< Is the session on foreground.
};

struct VARJORUNTIME_DEPRECATED varjo_EventHeadsetStandbyStatus {
    varjo_Bool onStandby;  //!< True if there are no consumers e.g. headset, analytics window and mirror view in VarjoBase of rendered frames.
};

struct varjo_EventStandbyStatus {
    varjo_Bool onStandby;  //!< True if there are no consumers e.g. headset, analytics window and mirror view in VarjoBase of rendered frames.
};

struct varjo_EventDisplayStatus {
    varjo_DisplayStatus status;  //!< Current display status.
};

struct varjo_EventMRDeviceStatus {
    varjo_MRDeviceStatus status;  //!< Is there a mixed reality capable device connected.
};

struct varjo_EventMRCameraPropertyChange {
    varjo_CameraPropertyType type;  //!< Changed property. Value can be queried with #varjo_MRGetCameraPropertyMode and #varjo_MRGetCameraPropertyValue.
};

struct varjo_EventDataStreamStart {
    varjo_StreamId streamId;  //!< Stream id.
};

struct varjo_EventDataStreamStop {
    varjo_StreamId streamId;  //!< Stream id.
};

struct varjo_EventTextureSizeChange {
    varjo_TextureSize_Type_Mask typeMask;  //!< Bitmask of changed texture size types.
};

union varjo_EventData {
    struct varjo_EventVisibility visibility;          //!< Visibility event data.
    struct varjo_EventButton button;                  //!< Button event data.
    struct varjo_EventTrackingStatus trackingStatus;  //!< Tracking status event data.
    struct varjo_EventHeadsetStatus headsetStatus;    //!< Headset status event data.
    struct varjo_EventDisplayStatus displayStatus;    //!< Display status event data.
    VARJORUNTIME_DEPRECATED struct varjo_EventHeadsetStandbyStatus headsetStandbyStatus;      //!< Headset standby status event data. @deprecated, varjo_EventStandbyStatus should be used,
    struct varjo_EventStandbyStatus standbyStatus;    //!< Standby status event data.
    struct varjo_EventForeground foreground;          //!< Foreground status event data.
    struct varjo_EventMRDeviceStatus mrDeviceStatus;  //!< Mixed reality device status event data.
    struct varjo_EventMRCameraPropertyChange mrCameraPropertyChange;  //!< Camera property change event data.
    struct varjo_EventDataStreamStart dataStreamStart;                //!< Data stream start event data.
    struct varjo_EventDataStreamStop dataStreamStop;                  //!< Data stream stop event data.
    struct varjo_EventTextureSizeChange textureSizeChange;            //!< Texture size change data
};

/**
 * Event struct used for all the events by the event system.
 */
struct varjo_Event {
    struct varjo_EventHeader header;  //!< Event header
    union varjo_EventData data;       //!< Event data
};


#if defined __cplusplus
}
#endif

#endif  // VARJO_EVENTS_H
