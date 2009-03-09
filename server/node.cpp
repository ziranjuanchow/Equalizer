
/* Copyright (c) 2005-2009, Stefan Eilemann <eile@equalizergraphics.com> 
   All rights reserved. */

#include <pthread.h>
#include "node.h"

#include "channel.h"
#include "config.h"
#include "global.h"
#include "log.h"
#include "paths.h"
#include "pipe.h"
#include "server.h"
#include "window.h"

#include <eq/net/barrier.h>
#include <eq/net/command.h>
#include <eq/client/packets.h>

using namespace eq::base;
using namespace std;

namespace eq
{
namespace server
{
typedef net::CommandFunc<Node> NodeFunc;

void Node::_construct()
{
    _active         = 0;
    _config         = 0;
    _tasks          = eq::TASK_NONE;
    _lastDrawPipe   = 0;
    _flushedFrame   = 0;
    _finishedFrame  = 0;
    EQINFO << "New node @" << (void*)this << endl;
}

Node::Node()
{
    _construct();

    const Global* global = Global::instance();    
    for( int i=0; i < eq::Node::IATTR_ALL; ++i )
        _iAttributes[i] =global->getNodeIAttribute((eq::Node::IAttribute)i);
}

Node::Node( const Node& from, Config* config )
        : net::Object()
{
    _construct();

    _name = from._name;
    _node = from._node;

    config->addNode( this );

    memcpy( _iAttributes, from._iAttributes, 
            eq::Node::IATTR_ALL * sizeof( int32_t ));

    const ConnectionDescriptionVector& descriptions =
        from.getConnectionDescriptions();
    for( ConnectionDescriptionVector::const_iterator i = descriptions.begin();
         i != descriptions.end(); ++i )
    {
        const ConnectionDescriptionPtr desc = *i;
        addConnectionDescription( 
            new ConnectionDescription( desc.getReference( )));
    }

    const PipeVector& pipes = from.getPipes();
    for( PipeVector::const_iterator i = pipes.begin(); i != pipes.end(); ++i )
        new Pipe( **i, this );
}

Node::~Node()
{
    EQINFO << "Delete node @" << (void*)this << endl;

    if( _config )
        _config->removeNode( this );
    
    for( PipeVector::const_iterator i = _pipes.begin(); i != _pipes.end(); ++i )
    {
        Pipe* pipe = *i;

        pipe->_node = 0;
        delete pipe;
    }
    _pipes.clear();
}

void Node::attachToSession( const uint32_t id, const uint32_t instanceID, 
                               net::Session* session )
{
    net::Object::attachToSession( id, instanceID, session );
    
    net::CommandQueue* queue = getCommandThreadQueue();

    registerCommand( eq::CMD_NODE_CONFIG_INIT_REPLY, 
                     NodeFunc( this, &Node::_cmdConfigInitReply ), queue );
    registerCommand( eq::CMD_NODE_CONFIG_EXIT_REPLY, 
                     NodeFunc( this, &Node::_cmdConfigExitReply ), queue );
    registerCommand( eq::CMD_NODE_FRAME_FINISH_REPLY,
                     NodeFunc( this, &Node::_cmdFrameFinishReply ), queue );
}

void Node::addPipe( Pipe* pipe )
{
    EQASSERT( pipe->getWindows().empty( ));

    _pipes.push_back( pipe ); 
    pipe->_node = this;
}

bool Node::removePipe( Pipe* pipe )
{
    EQASSERT( pipe->getWindows().empty( ));

    PipeVector::iterator i = find( _pipes.begin(), _pipes.end(), pipe );
    if( i == _pipes.end( ))
        return false;

    _pipes.erase( i );
    pipe->_node = 0; 

    return true;
}

NodePath Node::getPath() const
{
    EQASSERT( _config );
    
    const NodeVector&      nodes = _config->getNodes();
    NodeVector::const_iterator i = std::find( nodes.begin(), nodes.end(),
                                              this );
    EQASSERT( i != nodes.end( ));

    NodePath path;
    path.nodeIndex = std::distance( nodes.begin(), i );
    return path;
}

Channel* Node::getChannel( const ChannelPath& path )
{
    EQASSERT( _pipes.size() > path.pipeIndex );

    if( _pipes.size() <= path.pipeIndex )
        return 0;

    return _pipes[ path.pipeIndex ]->getChannel( path );
}

namespace
{
template< class C, class V >
VisitorResult _accept( C* node, V& visitor )
{ 
    VisitorResult result = visitor.visitPre( node );
    if( result != TRAVERSE_CONTINUE )
        return result;

    const PipeVector& pipes = node->getPipes();
    for( PipeVector::const_iterator i = pipes.begin(); i != pipes.end(); ++i )
    {
        switch( (*i)->accept( visitor ))
        {
            case TRAVERSE_TERMINATE:
                return TRAVERSE_TERMINATE;

            case TRAVERSE_PRUNE:
                result = TRAVERSE_PRUNE;
                break;
                
            case TRAVERSE_CONTINUE:
            default:
                break;
        }
    }

    switch( visitor.visitPost( node ))
    {
        case TRAVERSE_TERMINATE:
            return TRAVERSE_TERMINATE;

        case TRAVERSE_PRUNE:
            return TRAVERSE_PRUNE;
                
        case TRAVERSE_CONTINUE:
        default:
            break;
    }

    return result;
}
}

VisitorResult Node::accept( NodeVisitor& visitor )
{
    return _accept( this, visitor );
}

VisitorResult Node::accept( ConstNodeVisitor& visitor ) const
{
    return _accept( this, visitor );
}

void Node::activate()
{   
    ++_active;
    EQLOG( LOG_VIEW ) << "activate: " << _active << std::endl;
}

void Node::deactivate()
{ 
    EQASSERT( _active != 0 );
    --_active; 
    EQLOG( LOG_VIEW ) << "deactivate: " << _active << std::endl;
};

//===========================================================================
// Operations
//===========================================================================

//---------------------------------------------------------------------------
// update running entities (init/exit)
//---------------------------------------------------------------------------

void Node::updateRunning( const uint32_t initID )
{
    if( !isActive() && _state == STATE_STOPPED ) // inactive
        return;

    if( isActive() && _state != STATE_RUNNING ) // becoming active
    {
        EQASSERT( _state == STATE_STOPPED );
        _configInit( initID );
    }

    // Let all running pipes update their running state (incl. children)
    for( PipeVector::const_iterator i = _pipes.begin(); i != _pipes.end(); ++i )
        (*i)->updateRunning( initID );

    if( !isActive( )) // becoming inactive
    {
        EQASSERT( _state == STATE_RUNNING );
        _configExit();
    }

    flushSendBuffer();
}

bool Node::syncRunning()
{
    if( !isActive() && _state == STATE_STOPPED ) // inactive
        return true;

    // Sync state updates
    bool success = true;
    for( PipeVector::const_iterator i = _pipes.begin(); i != _pipes.end(); ++i )
    {
        Pipe* pipe = *i;
        if( !pipe->syncRunning( ))
        {
            _error += "pipe " + pipe->getName() + ": '" + 
                      pipe->getErrorMessage() + '\'';
            success = false;
        }
    }

    flushSendBuffer();

    if( isActive() && _state != STATE_RUNNING && !_syncConfigInit( ))
        // becoming active
        success = false;

    if( !isActive() && !_syncConfigExit( ))
        // becoming inactive
        success = false;

    EQASSERT( _state == STATE_RUNNING || _state == STATE_STOPPED );
    return success;
}

//---------------------------------------------------------------------------
// init
//---------------------------------------------------------------------------
void Node::_configInit( const uint32_t initID )
{
    EQASSERT( _state == STATE_STOPPED );
    _state = STATE_INITIALIZING;

    _flushedFrame  = 0;
    _finishedFrame = 0;
    _frameIDs.clear();

    _config->registerObject( this );

    EQLOG( LOG_INIT ) << "Create node" << std::endl;
    eq::ConfigCreateNodePacket createNodePacket;
    createNodePacket.nodeID = getID();
    _config->send( _node, createNodePacket );

    EQLOG( LOG_INIT ) << "Init node" << std::endl;
    eq::NodeConfigInitPacket packet;
    packet.initID = initID;
    packet.tasks  = _tasks;

    memcpy( packet.iAttributes, _iAttributes, 
            eq::Node::IATTR_ALL * sizeof( int32_t ));

    _send( packet, _name );
}

bool Node::_syncConfigInit()
{
    EQASSERT( _state == STATE_INITIALIZING || _state == STATE_INIT_SUCCESS ||
              _state == STATE_INIT_FAILED );

    _state.waitNE( STATE_INITIALIZING );

    const bool success = ( _state == STATE_INIT_SUCCESS );
    if( success )
        _state = STATE_RUNNING;
    else
        EQWARN << "Node initialization failed: " << _error << endl;

    return success;
}

//---------------------------------------------------------------------------
// exit
//---------------------------------------------------------------------------
void Node::_configExit()
{
    EQASSERT( _state == STATE_RUNNING || _state == STATE_INIT_FAILED );
    _state = STATE_EXITING;

    EQLOG( LOG_INIT ) << "Exit node" << std::endl;
    eq::NodeConfigExitPacket packet;
    _send( packet );
    flushSendBuffer();

    EQLOG( LOG_INIT ) << "Destroy node" << std::endl;
    eq::ConfigDestroyNodePacket destroyNodePacket;
    destroyNodePacket.nodeID = getID();
    _config->send( _node, destroyNodePacket );
}

bool Node::_syncConfigExit()
{
    EQASSERT( _state == STATE_EXITING || _state == STATE_EXIT_SUCCESS || 
              _state == STATE_EXIT_FAILED );
    
    _state.waitNE( STATE_EXITING );
    const bool success = ( _state == STATE_EXIT_SUCCESS );
    EQASSERT( success || _state == STATE_EXIT_FAILED );

    _config->deregisterObject( this );

    _state = STATE_STOPPED; // EXIT_FAILED -> STOPPED transition
    _tasks = eq::TASK_NONE;
    _frameIDs.clear();
    _flushBarriers();
    return success;
}

//---------------------------------------------------------------------------
// update
//---------------------------------------------------------------------------
void Node::update( const uint32_t frameID, const uint32_t frameNumber )
{
    EQVERB << "Start frame " << frameNumber << endl;
    EQASSERT( _state == STATE_RUNNING );
    EQASSERT( _active > 0 );

    _frameIDs[ frameNumber ] = frameID;
    
    if( !_lastDrawPipe ) // happens when used channels skip a frame (DPlex, LB)
        _lastDrawPipe = _pipes[0];

    eq::NodeFrameStartPacket startPacket;
    startPacket.frameID     = frameID;
    startPacket.frameNumber = frameNumber;
    _send( startPacket );
    EQLOG( eq::LOG_TASKS ) << "TASK node start frame " << &startPacket << endl;

    for( PipeVector::const_iterator i = _pipes.begin(); i != _pipes.end(); ++i )
    {
        Pipe* pipe = *i;
        if( pipe->isActive( ))
            pipe->update( frameID, frameNumber );
    }

    const Config*  config  = getConfig();
    const uint32_t latency = config->getLatency();

    eq::NodeFrameTasksFinishPacket finishPacket;
    finishPacket.frameID     = frameID;
    finishPacket.frameNumber = frameNumber;
    _send( finishPacket );
    EQLOG( eq::LOG_TASKS ) << "TASK node tasks finish " << &finishPacket <<endl;

    if( frameNumber > latency )
        flushFrames( frameNumber - latency );

    flushSendBuffer();
    _lastDrawPipe = 0;
}

void Node::updateFrameFinishNT( const uint32_t currentFrame )
{
    const Config*  config  = getConfig();
    const uint32_t latency = config->getLatency();
    if( latency == 0 || currentFrame <= latency )
        return;

    const uint32_t            frameNumber = currentFrame - latency;
    if( _frameIDs.find( frameNumber ) == _frameIDs.end( ))
        return; // finish already send by previous updateFrameFinishNT

    eq::NodeFrameFinishPacket packet;
    packet.frameID          = _frameIDs[ frameNumber ];
    packet.frameNumber      = frameNumber;
    packet.syncGlobalFinish = isApplicationNode();

    _send( packet );
    _frameIDs.erase( frameNumber );

    EQLOG( eq::LOG_TASKS ) << "TASK node finish frame non-threaded " << &packet
                           << endl;
}

void Node::flushFrames( const uint32_t frameNumber )
{
    EQVERB << "Flush frames including " << frameNumber << endl;

    while( _flushedFrame < frameNumber )
    {
        ++_flushedFrame;
        _sendFrameFinish( _flushedFrame );
    }

    flushSendBuffer();
}

void Node::_sendFrameFinish( const uint32_t frameNumber )
{
    if( _frameIDs.find( frameNumber ) == _frameIDs.end( ))
        return; // finish already send by updateFrameFinishNT

    eq::NodeFrameFinishPacket packet;
    packet.frameID     = _frameIDs[ frameNumber ];
    packet.frameNumber = frameNumber;

    _send( packet );
    _frameIDs.erase( frameNumber );
    EQLOG( eq::LOG_TASKS ) << "TASK node finish frame  " << &packet << endl;
}

//---------------------------------------------------------------------------
// Barrier cache
//---------------------------------------------------------------------------
net::Barrier* Node::getBarrier()
{
    if( _barriers.empty() )
    {
        net::Barrier* barrier = new net::Barrier( _node );
        _config->registerObject( barrier );
        barrier->setAutoObsolete( getConfig()->getLatency()+1, 
                                  Object::AUTO_OBSOLETE_COUNT_VERSIONS );
        
        return barrier;
    }

    net::Barrier* barrier = _barriers.back();
    _barriers.pop_back();
    barrier->setHeight(0);
    return barrier;
}

void Node::releaseBarrier( net::Barrier* barrier )
{
    _barriers.push_back( barrier );
}

void Node::_flushBarriers()
{
    for( vector< net::Barrier* >::const_iterator i =_barriers.begin(); 
         i != _barriers.end(); ++ i )
    {
        net::Barrier* barrier = *i;
        _config->deregisterObject( barrier );
        delete barrier;
    }
    _barriers.clear();
}

void Node::flushSendBuffer()
{
    _bufferedTasks.sendBuffer( _node->getConnection( ));
}

//===========================================================================
// command handling
//===========================================================================
net::CommandResult Node::_cmdConfigInitReply( net::Command& command )
{
    const eq::NodeConfigInitReplyPacket* packet = 
        command.getPacket<eq::NodeConfigInitReplyPacket>();
    EQVERB << "handle configInit reply " << packet << endl;
    EQASSERT( _state == STATE_INITIALIZING );

    _error = packet->error;
    _state = packet->result ? STATE_INIT_SUCCESS : STATE_INIT_FAILED;
    return net::COMMAND_HANDLED;
}

net::CommandResult Node::_cmdConfigExitReply( net::Command& command )
{
    const eq::NodeConfigExitReplyPacket* packet =
        command.getPacket<eq::NodeConfigExitReplyPacket>();
    EQVERB << "handle configExit reply " << packet << endl;
    EQASSERT( _state == STATE_EXITING );

    _state = packet->result ? STATE_EXIT_SUCCESS : STATE_EXIT_FAILED;
    return net::COMMAND_HANDLED;
}

net::CommandResult Node::_cmdFrameFinishReply( net::Command& command )
{
    const eq::NodeFrameFinishReplyPacket* packet = 
        command.getPacket<eq::NodeFrameFinishReplyPacket>();
    EQVERB << "handle frame finish reply " << packet << endl;
    
    _finishedFrame = packet->frameNumber;
    _config->notifyNodeFrameFinished( packet->frameNumber );

    return net::COMMAND_HANDLED;
}

ostream& operator << ( ostream& os, const Node* node )
{
    if( !node )
        return os;
    
    os << disableFlush;
    const Config* config = node->getConfig();
    if( config && config->isApplicationNode( node ))
        os << "appNode" << endl;
    else
        os << "node" << endl;

    os << "{" << endl << indent;

    const std::string& name = node->getName();
    if( !name.empty( ))
        os << "name     \"" << name << "\"" << endl;

    const ConnectionDescriptionVector& descriptions = 
        node->getConnectionDescriptions();
    for( ConnectionDescriptionVector::const_iterator i = descriptions.begin();
         i != descriptions.end(); ++i )

        os << (*i).get();

    bool attrPrinted   = false;
    
    for( eq::Node::IAttribute i = static_cast<eq::Node::IAttribute>( 0 );
         i<eq::Node::IATTR_ALL; 
         i = static_cast<eq::Node::IAttribute>( static_cast<uint32_t>( i )+1))
    {
        const int value = node->getIAttribute( i );
        if( value == Global::instance()->getNodeIAttribute( i ))
            continue;

        if( !attrPrinted )
        {
            os << endl << "attributes" << endl;
            os << "{" << endl << indent;
            attrPrinted = true;
        }
        
        os << ( i==eq::Node::IATTR_THREAD_MODEL ?
                    "thread_model       " :
                i==eq::Node::IATTR_HINT_STATISTICS ?
                    "hint_statistics    " :
                "ERROR" )
           << static_cast<eq::IAttrValue>( value ) << endl;
    }
    
    if( attrPrinted )
        os << exdent << "}" << endl << endl;

    const PipeVector& pipes = node->getPipes();
    for( PipeVector::const_iterator i = pipes.begin(); i != pipes.end(); ++i )
        os << *i;

    os << exdent << "}" << enableFlush << endl;
    return os;
}

}
}
