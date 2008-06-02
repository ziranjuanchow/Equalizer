
/* Copyright (c) 2005-2008, Stefan Eilemann <eile@equalizergraphics.com> 
   All rights reserved. */

#ifndef EQ_COMMANDS_H
#define EQ_COMMANDS_H

#include <eq/net/commands.h>

namespace eq
{
    enum ServerCommand
    {
        CMD_SERVER_CHOOSE_CONFIG        = eqNet::CMD_NODE_CUSTOM,
        CMD_SERVER_USE_CONFIG,
        CMD_SERVER_CHOOSE_CONFIG_REPLY,
        CMD_SERVER_CREATE_CONFIG,
        CMD_SERVER_CREATE_CONFIG_REPLY,
        CMD_SERVER_DESTROY_CONFIG,
        CMD_SERVER_RELEASE_CONFIG,
        CMD_SERVER_RELEASE_CONFIG_REPLY,
        CMD_SERVER_INIT_CONFIG,
        CMD_SERVER_SHUTDOWN,
        CMD_SERVER_SHUTDOWN_REPLY,
        CMD_SERVER_CUSTOM
    };

    enum ClientCommand
    {
        CMD_CLIENT_EXIT               = eqNet::CMD_NODE_CUSTOM,
        CMD_CLIENT_CUSTOM
    };

    enum ConfigCommand
    {
        CMD_CONFIG_START_INIT                 = eqNet::CMD_SESSION_CUSTOM,
        CMD_CONFIG_START_INIT_REPLY,
        CMD_CONFIG_FINISH_INIT,
        CMD_CONFIG_FINISH_INIT_REPLY,
        CMD_CONFIG_EXIT,
        CMD_CONFIG_EXIT_REPLY,
        CMD_CONFIG_CREATE_REPLY,
        CMD_CONFIG_CREATE_NODE,
        CMD_CONFIG_CREATE_NODE_REPLY,
        CMD_CONFIG_DESTROY_NODE,
        CMD_CONFIG_START_FRAME,
        CMD_CONFIG_START_FRAME_REPLY,
        CMD_CONFIG_FRAME_FINISH,
        CMD_CONFIG_FINISH_ALL_FRAMES,
        CMD_CONFIG_EVENT,
        CMD_CONFIG_DATA,
        CMD_CONFIG_START_CLOCK,
        CMD_CONFIG_CUSTOM
    };

    enum NodeCommand
    {
        CMD_NODE_CONFIG_INIT = eqNet::CMD_OBJECT_CUSTOM,
        CMD_NODE_CONFIG_INIT_REPLY,
        CMD_NODE_CONFIG_EXIT,
        CMD_NODE_CONFIG_EXIT_REPLY,
        CMD_NODE_CREATE_PIPE,
        CMD_NODE_DESTROY_PIPE,
        CMD_NODE_FRAME_START, 
        CMD_NODE_FRAME_FINISH,
        CMD_NODE_FRAME_FINISH_EARLY,
        CMD_NODE_FRAME_FINISH_REPLY,
        CMD_NODE_FRAME_DRAW_FINISH,
        CMD_NODE_CUSTOM
    };

    enum PipeCommand
    {
        CMD_PIPE_CONFIG_INIT = eqNet::CMD_OBJECT_CUSTOM,
        CMD_PIPE_CONFIG_INIT_REPLY,
        CMD_PIPE_CONFIG_EXIT,
        CMD_PIPE_CONFIG_EXIT_REPLY, 
        CMD_PIPE_CREATE_WINDOW,
        CMD_PIPE_DESTROY_WINDOW,
        CMD_PIPE_FRAME_START,
        CMD_PIPE_FRAME_FINISH,
        CMD_PIPE_FRAME_FINISH_REPLY,
        CMD_PIPE_FRAME_DRAW_FINISH,
        CMD_PIPE_STOP_THREAD,
        CMD_PIPE_FRAME_START_CLOCK,
        CMD_PIPE_CUSTOM
    };

    enum WindowCommand
    {
        CMD_WINDOW_CONFIG_INIT = eqNet::CMD_OBJECT_CUSTOM,
        CMD_WINDOW_CONFIG_INIT_REPLY,
        CMD_WINDOW_CONFIG_EXIT,
        CMD_WINDOW_CONFIG_EXIT_REPLY,
        CMD_WINDOW_CREATE_CHANNEL,
        CMD_WINDOW_DESTROY_CHANNEL,
        CMD_WINDOW_SET_PVP,
        CMD_WINDOW_FRAME_START,
        CMD_WINDOW_FRAME_FINISH,
        CMD_WINDOW_FINISH,
        CMD_WINDOW_BARRIER,
        CMD_WINDOW_SWAP,
        CMD_WINDOW_FRAME_DRAW_FINISH,
        CMD_WINDOW_CUSTOM
    };

    enum ChannelCommand
    {
        CMD_CHANNEL_CONFIG_INIT = eqNet::CMD_OBJECT_CUSTOM,
        CMD_CHANNEL_CONFIG_INIT_REPLY,
        CMD_CHANNEL_CONFIG_EXIT,
        CMD_CHANNEL_CONFIG_EXIT_REPLY,
        CMD_CHANNEL_SET_NEARFAR,
        CMD_CHANNEL_FRAME_START,
        CMD_CHANNEL_FRAME_FINISH,
        CMD_CHANNEL_FRAME_FINISH_REPLY,
        CMD_CHANNEL_FRAME_CLEAR,
        CMD_CHANNEL_FRAME_DRAW,
        CMD_CHANNEL_FRAME_DRAW_FINISH,
        CMD_CHANNEL_FRAME_ASSEMBLE,
        CMD_CHANNEL_FRAME_READBACK,
        CMD_CHANNEL_FRAME_TRANSMIT,
        CMD_CHANNEL_CUSTOM
    };

    enum FrameDataCommand
    {
        CMD_FRAMEDATA_TRANSMIT = eqNet::CMD_OBJECT_CUSTOM,
        CMD_FRAMEDATA_READY,
        CMD_FRAMEDATA_UPDATE,
        CMD_FRAMEDATA_CUSTOM
    };

    enum GLXEventThreadCommand
    {
        CMD_GLXEVENTTHREAD_REGISTER_PIPE,
        CMD_GLXEVENTTHREAD_DEREGISTER_PIPE,
        CMD_GLXEVENTTHREAD_REGISTER_WINDOW,
        CMD_GLXEVENTTHREAD_DEREGISTER_WINDOW,
        CMD_GLXEVENTTHREAD_ALL
    };
};

#endif // EQ_COMMANDS_H

